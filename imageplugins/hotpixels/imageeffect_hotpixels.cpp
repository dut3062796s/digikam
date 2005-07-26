/* ============================================================
* File  : imageeffect_hotpixels.cpp
* Author: Unai Garro <ugarro at users dot sourceforge dot net>
* Date  : 2005-03-27
* Description :  a digiKam image plugin for fixing dots produced by
*           hot/stuck/dead pixels from a CCD
*
* Copyright 2005 by Unai Garro
*
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

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwhatsthis.h>

// KDE includes.

#include <kdebug.h>
#include <klocale.h>
#include <kimageio.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>

// Digikam includes.

#include <digikamheaders.h>

// Local includes.

#include "version.h"
#include "imageeffect_hotpixels.h"
#include "hotpixelviewwidget.h"
#include "blackframelistview.h"
#include "blackframeparser.h"

//Temporally added includes. Please remove
#include <iostream>

namespace DigikamHotPixelsImagesPlugin
{

ImageEffect_HotPixels::ImageEffect_HotPixels(QWidget* parent)
                   : CtrlPanelDialog(parent, i18n("Hot Pixels Correction"), "hotpixels", 
                                     false, false, false, 
                                     Digikam::ImagePannelWidget::SeparateViewDuplicate)
{
    // No need Abort button action.
    showButton(User1, false); 
    
    showButton(Apply, true);
    setButtonText( Apply, i18n("Add Black Frame..."));
    setButtonWhatsThis( Apply, i18n("<p>Use this button to add a new black frame file witch will "
                                    "be used by the hot pixels removal filter.") );  
    QString whatsThis;

    KAboutData* about = new KAboutData("digikamimageplugins",
                                       I18N_NOOP("Hot Pixels Correction"), 
                                       digikamimageplugins_version,
                                       I18N_NOOP("A digiKam image plugin for fixing dots produced by "
                                                 "hot/stuck/dead pixels from a CCD."),
                                       KAboutData::License_GPL,
                                       "(c) 2005, Unai Garro", 
                                       0,
                                       "http://extragear.kde.org/apps/digikamimageplugins");
                
    about->addAuthor("Unai Garro", I18N_NOOP("Author and maintainer"),
                     "ugarro at sourceforge dot net");
    
    about->addAuthor("Gilles Caulier", I18N_NOOP("Developper"),
                     "caulier dot gilles at free.fr");
        
    setAboutData(about);
    
    // -------------------------------------------------------------
    // Settings area.
    
    QWidget *gboxSettings = new QWidget(m_imagePreviewWidget);
    QGridLayout* gridSettings = new QGridLayout( gboxSettings, 2, 2, marginHint(), spacingHint());
    
    QLabel *filterMethodLabel = new QLabel(i18n("Filter method:"), gboxSettings);
    m_filterMethodCombo = new QComboBox(gboxSettings);
    
    m_filterMethodCombo->insertItem(i18n("Average"));
    m_filterMethodCombo->insertItem(i18n("Linear"));
    m_filterMethodCombo->insertItem(i18n("Quadratic"));
    m_filterMethodCombo->insertItem(i18n("Cubic"));
    
    gridSettings->addMultiCellWidget(filterMethodLabel, 0, 0, 0, 0);
    gridSettings->addMultiCellWidget(m_filterMethodCombo, 0, 0, 1, 2);
        
    //TODO: load the default setting from kconfig
    
    m_blackFrameListView = new BlackFrameListView(gboxSettings);
    m_blackFrameListView->addColumn(i18n("Preview"));
    m_blackFrameListView->addColumn(i18n("Size"));
    m_blackFrameListView->addColumn(i18n("Enabled"));
    gridSettings->addMultiCellWidget(m_blackFrameListView, 1, 2, 0, 2);
    
    m_imagePreviewWidget->setUserAreaWidget(gboxSettings);
        
    // -------------------------------------------------------------
    // Black frames parser instance.

    m_parser = new BlackFrameParser;
    
    // -------------------------------------------------------------
    // Main window's signal & slots
    
    connect(m_filterMethodCombo, SIGNAL(activated(int)),
            this, SLOT(slotEffect()));
                          
    connect(m_parser, SIGNAL(parsed(QValueList<HotPixel>)),
            this, SLOT(blackFrameParsed(QValueList<HotPixel>))); 
}

ImageEffect_HotPixels::~ImageEffect_HotPixels()
{
    delete m_parser;
}

// Select Black frame file.
void ImageEffect_HotPixels::slotApply()
{
    //Does one need to do this if digikam did so already?
    KImageIO::registerFormats(); 
    
    KFileDialog *fileSelectDialog = new KFileDialog(QString::null, KImageIO::pattern(), this, "", true);
    fileSelectDialog->setCaption(i18n("Select a black frame image"));
    fileSelectDialog->exec();
    
    //Load the selected file and insert into the list
    
    KURL url = fileSelectDialog->selectedURL();
    new BlackFrameListViewItem(m_blackFrameListView, url);
    delete fileSelectDialog;
}

void ImageEffect_HotPixels::renderingFinished(void)
{
    m_filterMethodCombo->setEnabled(true);
    enableButton(Apply, true);     
}

void ImageEffect_HotPixels::resetValues(void)
{
    m_filterMethodCombo->blockSignals(true);
    m_filterMethodCombo->setCurrentItem(2);       //FIXME
    m_filterMethodCombo->blockSignals(false);
}  

void ImageEffect_HotPixels::prepareEffect()
{
    m_filterMethodCombo->setEnabled(false);
    enableButton(Apply, false);     

    QImage image = m_imagePreviewWidget->getOriginalClipImage();
    
    //FIXME
    int index = m_filterMethodCombo->currentItem();
    InterpolationMethod method;
    
    if (index==0)
        method=average;
    else if (index==1)
        method=linear;
    else if (index==2)
        method=quadratic;
    else if (index==3)
        method=cubic;
    else method=quadratic; //Default, it shouldn't happen

    QValueList<HotPixel> hotPixelsRegion;
    QRect area = m_imagePreviewWidget->getOriginalImageRegionToRender();
    
    for (QValueList<HotPixel>::Iterator it = m_hotPixelsList.begin() ; it != m_hotPixelsList.end() ; ++it )
        {
        HotPixel hp = (*it);
        
        if ( area.contains( hp.rect ) )
           {
           hp.rect.moveTopLeft(QPoint::QPoint( hp.rect.x()-area.x(), hp.rect.y()-area.y() ));
           hotPixelsRegion.append(hp);
           }
        }

    m_threadedFilter = dynamic_cast<Digikam::ThreadedFilter *>(new HotPixelFixer(
                       &image, this, hotPixelsRegion, method));
}

void ImageEffect_HotPixels::prepareFinal()
{
    m_filterMethodCombo->setEnabled(false);
    enableButton(Apply, false);     
    
    Digikam::ImageIface iface(0, 0);
    QImage orgImage(iface.originalWidth(), iface.originalHeight(), 32);
    uint *data = iface.getOriginalData();
    memcpy( orgImage.bits(), data, orgImage.numBytes() );
    
    //FIXME
    int index = m_filterMethodCombo->currentItem();
    InterpolationMethod method;
    
    if (index==0)
        method=average;
    else if (index==1)
        method=linear;
    else if (index==2)
        method=quadratic;
    else if (index==3)
        method=cubic;
    else method=quadratic; //Default, it shouldn't happen

    m_threadedFilter = dynamic_cast<Digikam::ThreadedFilter *>(new HotPixelFixer(
                       &orgImage, this, m_hotPixelsList, method));
    delete [] data;
}

void ImageEffect_HotPixels::putPreviewData(void)
{
    QImage imDest = m_threadedFilter->getTargetImage();
    m_imagePreviewWidget->setPreviewImageData(imDest);
}

void ImageEffect_HotPixels::putFinalData(void)
{
    Digikam::ImageIface iface(0, 0);

    iface.putOriginalData(i18n("Hot Pixels Correction"), 
                        (uint*)m_threadedFilter->getTargetImage().bits());
}

void ImageEffect_HotPixels::blackFrameParsed(QValueList<HotPixel> hpList)
{
    m_hotPixelsList = hpList;
    slotEffect();
}

}  // NameSpace DigikamHotPixelsImagesPlugin

#include "imageeffect_hotpixels.moc"
