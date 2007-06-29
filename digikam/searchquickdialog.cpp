/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-05-19
 * Description : a dialog to perform simple search in albums
 *
 * Copyright (C) 2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2006-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

/** @file searchquickdialog.cpp */

// Qt includes.

#include <qtimer.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qlabel.h>

//Added by qt3to4:
#include <Q3GridLayout>
#include <QHideEvent>

// KDE includes.

#include <klineedit.h>
#include <klocale.h>
#include <kurl.h>

// Local includes.

#include "ddebug.h"
#include "searchresultsview.h"
#include "searchquickdialog.h"
#include "searchquickdialog.moc"

namespace Digikam
{

class SearchQuickDialogPriv
{
public:

    SearchQuickDialogPriv()
    {
        timer       = 0;
        searchEdit  = 0;
        nameEdit    = 0;
        resultsView = 0;
    }

    QTimer            *timer;

    KLineEdit         *searchEdit;
    KLineEdit         *nameEdit;

    SearchResultsView *resultsView;
};

SearchQuickDialog::SearchQuickDialog(QWidget* parent, KUrl& url)
                 : KDialog(parent)
		 , m_url(url)
{
    setDefaultButton(Ok);
    setButtons(Help|Ok|Cancel);
    setModal(true);
    setCaption(i18n("Quick Search"));

    QWidget *w = new QWidget(this);
    setMainWidget(w);
    d = new SearchQuickDialogPriv;
    d->timer = new QTimer(this);
    setHelp("quicksearchtool.anchor", "digikam");
    
    Q3GridLayout* grid = new Q3GridLayout(w, 2, 2, 0, spacingHint());
    
    QLabel *label1 = new QLabel("<b>" + i18n("Search:") + "</b>", w);
    d->searchEdit  = new KLineEdit(w);
    d->searchEdit->setWhatsThis( i18n("<p>Enter your search criteria to find items in the album library"));
    
    d->resultsView = new SearchResultsView(w);
    d->resultsView->setMinimumSize(320, 200);
    d->resultsView->setWhatsThis( i18n("<p>Here you can see the items found in album library "
                                          "using the current search criteria"));
    
    QLabel *label2 = new QLabel(i18n("Save search as:"), w);
    d->nameEdit    = new KLineEdit(w);
    d->nameEdit->setText(i18n("Last Search"));
    d->nameEdit->setWhatsThis( i18n("<p>Enter the name of the current search to save in the "
                                       "\"My Searches\" view"));

    grid->addMultiCellWidget(label1, 0, 0, 0, 0);
    grid->addMultiCellWidget(d->searchEdit, 0, 0, 1, 2);
    grid->addMultiCellWidget(d->resultsView, 1, 1, 0, 2);
    grid->addMultiCellWidget(label2, 2, 2, 0, 1);
    grid->addMultiCellWidget(d->nameEdit, 2, 2, 2, 2);
    
    connect(d->searchEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotSearchChanged(const QString&)));

    connect(d->timer, SIGNAL(timeout()),
            this, SLOT(slotTimeOut()));

    enableButtonOk(false);
    //resize(configDialogSize("QuickSearch Dialog"));

    // check if we are being passed a valid url
    if (m_url.isValid())
    {
        int count = m_url.queryItem("count").toInt();
        if (count > 0)
        {
            QStringList strList;

            for (int i=1; i<=count; i++)
            {
                QString val = m_url.queryItem(QString::number(i) + ".val");
                if (!strList.contains(val))
                {
                    strList.append(val);
                }
            }
            
            d->searchEdit->setText(strList.join(" "));
            d->nameEdit->setText(url.queryItem("name"));
            d->timer->start(0, true);
        }
    }
}

SearchQuickDialog::~SearchQuickDialog()
{
    //saveDialogSize(("QuickSearch Dialog"));
    delete d->timer;    
    delete d;
}

void SearchQuickDialog::slotTimeOut()
{
    if (d->searchEdit->text().isEmpty())
    {
        d->resultsView->clear();
        enableButtonOk(false);
        return;
    }

    enableButtonOk(true);
    
    KUrl url;
    url.setProtocol("digikamsearch");

    QString path, num;
    int     count = 0;
    
    QStringList textList = QStringList::split(' ', d->searchEdit->text());
    for (QStringList::iterator it = textList.begin(); it != textList.end(); ++it)
    {
        if (count != 0)
            path += " AND ";

        path += QString(" %1 ").arg(count + 1);

        num = QString::number(++count);
        url.addQueryItem(num + ".key", "keyword");
        url.addQueryItem(num + ".op", "like");
        url.addQueryItem(num + ".val", *it);
    }

    url.setPath(path);
    url.addQueryItem("name", "Live Search");
    url.addQueryItem("count", num);

    m_url = url;
    d->resultsView->openURL(url);
}

void SearchQuickDialog::slotSearchChanged(const QString&)
{
    d->timer->start(500, true);    
}

void SearchQuickDialog::hideEvent(QHideEvent* e)
{
    m_url.removeQueryItem("name");
    m_url.addQueryItem("name", d->nameEdit->text().isEmpty() ?
                       i18n("Last Search") : d->nameEdit->text());
    KDialog::hideEvent(e);
}

}  // namespace Digikam

