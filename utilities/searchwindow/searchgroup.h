/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-01-20
 * Description : User interface for searches
 *
 * Copyright (C) 2008 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef SEARCHGROUP_H
#define SEARCHGROUP_H

// Qt includes

#include <QWidget>
#include <QList>

// Local includes

#include "searchview.h"
#include "searchxml.h"

class QComboBox;
class QGridLayout;
class QLabel;
class QRadioButton;
class QStackedLayout;
class QVBoxLayout;

namespace Digikam
{

class SearchGroupLabel;
class SearchFieldGroup;
class SearchFieldGroupLabel;
class ClickLabel;

class SearchGroup : public AbstractSearchGroupContainer
{
    Q_OBJECT

public:

    enum Type
    {
        FirstGroup,
        ChainGroup
    };

    SearchGroup(SearchView *parent);

    void setup(Type type = FirstGroup);

    void read(SearchXmlCachingReader& reader);
    void write(SearchXmlWriter& writer);
    void reset();

    Type groupType() const;

    QList<QRect> startupAnimationArea() const;

Q_SIGNALS:

    void removeRequested();

protected:

    virtual SearchGroup *createSearchGroup();
    virtual void addGroupToLayout(SearchGroup *group);

    SearchView                   *m_view;

    QList<SearchFieldGroup*>      m_fieldGroups;
    QList<SearchFieldGroupLabel*> m_fieldLabels;

    QVBoxLayout                  *m_layout;
    SearchGroupLabel             *m_label;
    QVBoxLayout                  *m_subgroupLayout;

    Type                          m_groupType;
};

// -------------------------------------------------------------------------

class SearchGroupLabel : public QWidget
{
    Q_OBJECT

public:

    SearchGroupLabel(SearchViewThemedPartsCache *cache, SearchGroup::Type type, QWidget *parent = 0);

    void setGroupOperator(SearchXml::Operator op);
    void setDefaultFieldOperator(SearchXml::Operator op);

    SearchXml::Operator groupOperator() const;
    SearchXml::Operator defaultFieldOperator() const;

Q_SIGNALS:

    void removeClicked();

protected Q_SLOTS:

    void toggleShowOptions();
    void toggleGroupOperator();
    void boxesToggled();

protected:

    virtual void paintEvent(QPaintEvent *ev);

    void setExtended(bool extended);
    void adjustOperatorOptions();
    void updateGroupLabel();

private:

    bool                           m_extended;
    SearchXml::Operator            m_groupOp;
    SearchXml::Operator            m_fieldOp;
    QGridLayout                   *m_layout;
    //QComboBox                     *m_groupOpBox;
    ClickLabel                    *m_groupOpLabel;
    QRadioButton                  *m_allBox;
    QRadioButton                  *m_anyBox;
    QRadioButton                  *m_noneBox;
    QRadioButton                  *m_oneNotBox;
    ClickLabel                    *m_optionsLabel;
    ClickLabel                    *m_removeLabel;
    QStackedLayout                *m_stackedLayout;
    SearchViewThemedPartsCache    *m_themeCache;
};

} // namespace Digikam

#endif // SEARCHGROUP_H
