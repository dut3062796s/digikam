/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-05-04
 * Description : Various operations on images
 *
 * Copyright (C) 2009 by Marcel Wiesweg <marcel.wiesweg@gmx.de>
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

#ifndef IMAGEVIEWUTILITIES_H
#define IMAGEVIEWUTILITIES_H

// Qt includes

#include <QList>
#include <QWidget>

// KDE includes

#include <kurl.h>

// Local includes

#include "imageinfo.h"

class KJob;
namespace KIO { class Job; }

namespace Digikam
{

class Album;

class ImageViewUtilities : public QObject
{
    Q_OBJECT

public:

    ImageViewUtilities(QWidget *parentWidget);

public:

    void addIsCutSelection(QMimeData* mime, bool cut);
    bool decodeIsCutSelection(const QMimeData *mimeData);

public Q_SLOTS:

    void createNewAlbumForInfos(const QList<ImageInfo>& infos, Album *currentAlbum);
    void deleteImages(const QList<ImageInfo>& infos, bool deletePermanently);
    void deleteImagesDirectly(const QList<ImageInfo>& infos, bool useTrash);

    void insertToLightTable(const QList<ImageInfo>& list, const ImageInfo& current, bool emptyBeforeAdding);
    void insertToQueueManager(const QList<ImageInfo>& list, const ImageInfo& currentInfo, bool newQueue);
    void insertSilentToQueueManager(const QList<ImageInfo>& list, const ImageInfo& currentInfo, int queueid);

    void notifyFileContentChanged(const KUrl::List& urls);

    void openInEditor(const ImageInfo& info, const QList<ImageInfo>& allInfosToOpen, Album *currentAlbum);
    void rename(const ImageInfo& renameInfo);
    void rename(const ImageInfo& renameInfo, const QString& newName);
    void setAsAlbumThumbnail(Album *album, const ImageInfo& imageInfo);

Q_SIGNALS:

    void editorCurrentUrlChanged(const KUrl& url);
    void imageRenamed();

protected Q_SLOTS:

    void slotDIOResult(KJob* kjob);
    void slotRenamed(KIO::Job*, const KUrl &, const KUrl& newURL);

protected:

    QWidget *m_widget;
};

} // namespace Digikam

#endif /* IMAGEVIEWUTILITIES_H */

