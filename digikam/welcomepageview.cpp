/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-12-20
 * Description : a widget to display a welcome page
 *               on root album.
 *
 * Copyright (C) 2006-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

// Qt includes.

#include <QWidget>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

// KDE includes.

#include <klocale.h>
#include <kcursor.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kapplication.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>
#include <kglobalsettings.h>
#include <ktemporaryfile.h>

// Local includes.

#include "ddebug.h"
#include "version.h"
#include "themeengine.h"
#include "welcomepageview.h"
#include "welcomepageview.moc"

namespace Digikam
{

WelcomePageView::WelcomePageView(QWidget* parent)
               : KHTMLPart(parent)
{
    widget()->setFocusPolicy(Qt::WheelFocus);
    // Let's better be paranoid and disable plugins (it defaults to enabled):
    setPluginsEnabled(false);
    setJScriptEnabled(false); // just make this explicit.
    setJavaEnabled(false);    // just make this explicit.
    setMetaRefreshEnabled(false);
    setURLCursor(Qt::PointingHandCursor);

    m_digikamCssFile = new KTemporaryFile;
    m_digikamCssFile->setSuffix(".css");
    m_digikamCssFile->setAutoRemove(false);
    m_digikamCssFile->open();

    m_infoPageCssFile = new KTemporaryFile;
    m_infoPageCssFile->setSuffix(".css");
    m_infoPageCssFile->setAutoRemove(false);
    m_infoPageCssFile->open();

    slotThemeChanged();

    // ------------------------------------------------------------

    connect(browserExtension(), SIGNAL(openUrlRequest(const KUrl&, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&)),
            this, SLOT(slotUrlOpen(const KUrl&)));

    connect(ThemeEngine::instance(), SIGNAL(signalThemeChanged()),
            this, SLOT(slotThemeChanged()));
}

WelcomePageView::~WelcomePageView()
{
    m_digikamCssFile->remove();
    m_infoPageCssFile->remove();
}

void WelcomePageView::slotUrlOpen(const KUrl &url)
{
   KToolInvocation::invokeBrowser(url.url());
}

QString WelcomePageView::infoPage()
{
    QStringList newFeatures;
    newFeatures << i18n("Designed for KDE4");
    newFeatures << i18n("Hardware handling using KDE4 Solid interface");
    newFeatures << i18n("XMP metadata support");
    newFeatures << i18n("A new camera interface");
    newFeatures << i18n("A new tool to capture photographs from Camera");
    newFeatures << i18n("The database file can be stored on a customized place to support remote album library paths");
    newFeatures << i18n("Support of multiple roots album paths");
    newFeatures << i18n("Thumbnails-bar integration with preview mode and in editor for easy navigation between images");
    newFeatures << i18n("Supports the latest camera RAW files");
    newFeatures << i18n("Geolocation performed using KDE4 Marble interface");
    newFeatures << i18n("New tool based on LensFun library to fix lens defaults automatically");

    QString featureItems;
    for ( int i = 0 ; i < newFeatures.count() ; i++ )
        featureItems += i18n("<li>%1</li>\n", newFeatures[i] );

    QString info =
        i18nc(
        "%1: current digiKam version; "
        "%2: digiKam help:// Url; "
        "%3: digiKam homepage Url; "
        "%4: prior digiKam version; "
        "%5: prior KDE version; "
        "%6: generated list of new features; "
        "%7: generated list of important changes; "
        "--- end of comment ---",

        "<h2 style='margin-top: 0px;'>"
        "Welcome to digiKam %1"
        "</h2><p>"
        "digiKam is an open source photo management program. "
        "It is designed to import, organize, enhance, search and export your digital photographs on your computer.</p>"
        "<p>You are currently in the Album view mode of digiKam. The Albums are the real "
        "containers where your files are stored, they are identical with the folders "
        "on disk.</p>\n<ul><li>"
        "digiKam has many powerful features which are described in the "
        "<a href=\"%2\">documentation</a></li>\n"
        "<li>The <a href=\"%3\">digiKam homepage</A> provides information about "
        "new versions of digiKam</li></ul>\n"
        "%7\n<p>"
        "Some of the new features in this release of digiKam include "
        "(compared to digiKam %4):</p>\n"
        "<ul>\n%5</ul>\n"
        "%6\n"
        "<p>We hope that you will enjoy digiKam.</p>\n"
        "<p>Thank you,</p>\n"
        "<p style='margin-bottom: 0px'>&nbsp; &nbsp; The digiKam team</p>",

    QString(digikam_version),            // %1 : current digiKam version
    "help:/digikam/index.html",          // %2 : digiKam help:// Url
    "http://www.digikam.org",            // %3 : digiKam homepage Url
    "0.9.3",                             // %4 : prior digiKam version
    featureItems,                        // %5 : prior KDE version
    QString(),                           // %6 : generated list of new features
    QString());                          // %7 : previous digiKam release.

    return info;
}

QByteArray WelcomePageView::fileToString(const QString &aFileName)
{
    QByteArray   result;
    QFileInfo    info(aFileName);
    unsigned int readLen;
    unsigned int len = info.size();
    QFile        file(aFileName);

    if (aFileName.isEmpty() || len <= 0 ||
        !info.exists() || info.isDir() || !info.isReadable() ||
        !file.open(QIODevice::Unbuffered|QIODevice::ReadOnly))
        return QByteArray();

    result.resize(len + 2);
    readLen = file.read(result.data(), len);
    if (1 && result[len-1]!='\n')
    {
        result[len++] = '\n';
        readLen++;
    }
    result[len] = '\0';

    if (readLen < len)
        return QByteArray();

    return result;
}

QString WelcomePageView::digikamCssFilePath()
{
    QColor background = ThemeEngine::instance()->baseColor();

    QString locationLogo = KStandardDirs::locate("data", "digikam/about/top-left-digikam.png");
    QString digikamCss   = fileToString(KStandardDirs::locate("data", "digikam/about/digikam.css"));
    digikamCss           = digikamCss.arg(locationLogo)             // %1
                                     .arg(background.name())        // %2
                                     .arg(background.name());       // %3

    QFile file(m_digikamCssFile->fileName());
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file); 
    stream << digikamCss;
    file.close();

    return m_digikamCssFile->fileName();
}

QString WelcomePageView::infoPageCssFilePath()
{
    QColor background = ThemeEngine::instance()->baseColor();

    QString infoPageCss  = fileToString(KStandardDirs::locate("data", "digikam/about/infopage.css"));
    infoPageCss          = infoPageCss.arg(background.name())                                                    // %1
                                      .arg(KStandardDirs::locate("data", "digikam/about/top-middle.png"))        // %2
                                      .arg(KStandardDirs::locate("data", "digikam/about/top-left.png"))          // %3
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-top-left.png"))      // %4
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-top-right.png"))     // %5
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-top-middle.png"))    // %6
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-middle-left.png"))   // %7
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-middle-right.png"))  // %8
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-bottom-left.png"))   // %9
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-bottom-right.png"))  // %10
                                      .arg(KStandardDirs::locate("data", "digikam/about/box-bottom-middle.png")) // %11
                                      .arg(KStandardDirs::locate("data", "digikam/about/bottom-middle.png"))     // %12
                                      .arg(KStandardDirs::locate("data", "digikam/about/bottom-left.png"))       // %13
                                      .arg(KStandardDirs::locate("data", "digikam/about/bottom-right.png"));     // %14

    QFile file(m_infoPageCssFile->fileName());
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file); 
    stream << infoPageCss;
    file.close();

    return m_infoPageCssFile->fileName();
}

void WelcomePageView::slotThemeChanged()
{
    QString fontSize         = QString::number(12);
    QString appTitle         = i18n("digiKam");
    QString catchPhrase      = QString();      // Not enough space for a catch phrase at default window size.
    QString quickDescription = i18n("Manage your photographs like a professional "
                                    "with the power of open source");
    QString locationHtml     = KStandardDirs::locate("data", "digikam/about/main.html");
    QString locationCss      = infoPageCssFilePath();
    QString locationRtl      = KStandardDirs::locate("data", "digikam/about/infopage_rtl.css" );
    QString rtl              = kapp->isRightToLeft() ? QString("@import \"%1\";" ).arg(locationRtl)
                                                     : QString();
    QString digikamCss       = digikamCssFilePath();

    begin(KUrl(locationHtml));

    QString content = fileToString(locationHtml);
    content         = content.arg(locationCss)        // %1
                             .arg(rtl)                // %2
                             .arg(digikamCss)         // %3
                             .arg(fontSize)           // %4
                             .arg(appTitle)           // %5
                             .arg(catchPhrase)        // %6
                             .arg(quickDescription)   // %7
                             .arg(infoPage());        // %8

    write(content);
    end();
    show();
}

}  // namespace Digikam
