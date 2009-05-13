/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-12-01
 * Description : Recording changes on the database
 *
 * Copyright (C) 2007-2008 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef DATABASECHANGESETS_H
#define DATABASECHANGESETS_H

// Qt includes

#include <QList>
#include <QMetaType>
#include <QDBusArgument>

// Local includes

#include "dbusutilities.h"
#include "digikam_export.h"
#include "databasefields.h"

namespace Digikam
{

class DIGIKAM_DATABASE_EXPORT ImageChangeset
{
public:

    /**
     * An ImageChangeset covers adding or changing any properties of an image.
     * It is described by a list of affected image ids, and a set of affected database fields.
     * There is no guarantee that information in the database has actually been changed.
     */

    ImageChangeset();
    ImageChangeset(QList<qlonglong> ids, DatabaseFields::Set changes);
    ImageChangeset(qlonglong id, DatabaseFields::Set changes);

    QList<qlonglong> ids() const;
    bool containsImage(qlonglong id) const;
    DatabaseFields::Set changes() const;

    ImageChangeset& operator<<(const QDBusArgument& argument);
    const ImageChangeset& operator>>(QDBusArgument& argument) const;

private:

    QList<qlonglong>    m_ids;
    DatabaseFields::Set m_changes;
};

class DIGIKAM_DATABASE_EXPORT ImageTagChangeset
{
public:

    /**
     * An ImageTagChangeset covers adding and removing the association of a tag with an image.
     * It is described by a list of affected image ids, a list of affected tags,
     * and an operation.
     * There is no guarantee that information in the database has actually been changed.
     * Special case:
     * If all tags have been removed from an item, operation is RemovedAll,
     * and the tags list is empty. containsTag() will always return true in this case.
     */

    enum Operation
    {
        Unknown,
        Added,
        Removed,
        RemovedAll
    };

    ImageTagChangeset();
    ImageTagChangeset(QList<qlonglong> ids, QList<int> tags, Operation operation);
    ImageTagChangeset(qlonglong id, QList<int> tags, Operation operation);
    ImageTagChangeset(qlonglong id, int tag, Operation operation);

    /**
     * Combines two ImageTagChangesets.
     * The operations shall not differ between the two sets;
     * the operation is set to Unknown if it differs.
     * This is especially not suitable for RemovedAll changesets.
     */
    ImageTagChangeset& operator<<(const ImageTagChangeset& other);

    ImageTagChangeset& operator<<(const QDBusArgument& argument);
    const ImageTagChangeset& operator>>(QDBusArgument& argument) const;

    QList<qlonglong> ids() const;
    bool containsImage(qlonglong id) const;
    QList<int> tags() const;
    bool containsTag(int id);
    Operation operation() const;

    bool tagsWereAdded() const
    { return operation() == Added; }
    bool tagsWereRemoved() const
    { return operation() == Removed || operation() == RemovedAll; }

private:

    QList<qlonglong>    m_ids;
    QList<int>          m_tags;
    Operation           m_operation;
};

class DIGIKAM_DATABASE_EXPORT CollectionImageChangeset
{
public:

    enum Operation
    {
        Unknown,

        /** "Added" indicates that images have been added to albums. */
        Added,

        /** "Removed" indicates that an image has been removed from the given album,
         *  and has possibly set a status of Removed and a null Album (though this can
         *  already have changed to valid values), but the image-specific tables have not been removed. */
        Removed,

        /** "RemovedAll" indicates that for all entries in the specified album, the "Removed" operation
         *  has been carried out. This is equivalent to a "Removed" changesets with all image ids in the
         *  list, but for RemovedAll, the list may not be explicitly given (may be empty). */
        RemovedAll,

        /** "Deleted" indicates that the image-specific tables have been removed from the database.
         *  While "Removed" means all data is still there, though possibly not accessible from an album,
         *  this means all data has been irreversibly deleted. */
        Deleted,

        /** Special combination: Images which has the "Removed" status have now been "Delete"d.
         *  A changeset with Removed or RemovedAll is guaranteed to have been sent anytime before.
         *  Image ids nor albums ids may or may be not available in any combination. */
        RemovedDeleted,

        /** Images have been moved. This is extra information; a Removed and then an Added changeset
         *  are guaranteed to be sent subsequently.
         *  Album is the source album.
         */
        Moved,

        /** Images have been copied. This is extra information; an Added changeset
         *  is guaranteed to be sent subsequently.
         *  Album is the source album.
         */
        Copied
    };

    /**
     * An CollectionImageChangeset covers adding and removing an image to/from the collection.
     * It is described by a list of affected image ids, a list of affected albums,
     * and an operation.
     * Special Case "RemovedAll":
     * If all images have been removed from an album, operation is RemovedAll,
     * the album list contains the (now empty) albums, ids() is empty,
     * but containsImage() always returns true.
     * Special Case "RemovedDeleted":
     * Images with the "Removed" status are now irreversibly deleted.
     * ids() and/or albums() may be empty (this means information is not available).
     */

    CollectionImageChangeset();
    CollectionImageChangeset(QList<qlonglong> ids, QList<int> albums, Operation operation);
    CollectionImageChangeset(QList<qlonglong> ids, int album, Operation operation);
    CollectionImageChangeset(qlonglong id, int album, Operation operation);

    /**
     * Combines two CollectionImageChangesets.
     * The operations shall not differ between the two sets;
     * the operation is set to Unknown if it differs.
     * This is especially not suitable for RemovedAll changesets.
     */
    CollectionImageChangeset& operator<<(const CollectionImageChangeset& other);

    CollectionImageChangeset& operator<<(const QDBusArgument& argument);
    const CollectionImageChangeset& operator>>(QDBusArgument& argument) const;

    /** Specification of this changeset.
     *  All special cases where the returned list may be empty are noted above.
     *  The lists are valid unless such a case is explicitly mentioned.
     */
    QList<qlonglong> ids() const;
    bool containsImage(qlonglong id) const;
    QList<int> albums() const;
    bool containsAlbum(int id) const;
    Operation operation() const;

private:

    QList<qlonglong>    m_ids;
    QList<int>          m_albums;
    Operation           m_operation;
};

class DIGIKAM_DATABASE_EXPORT AlbumChangeset
{
public:

    enum Operation
    {
        Unknown,
        Added,
        Deleted,
        Renamed,
        PropertiesChanged
    };

    AlbumChangeset();
    AlbumChangeset(int albumId, Operation operation);

    int albumId() const;
    Operation operation() const;

    AlbumChangeset& operator<<(const QDBusArgument& argument);
    const AlbumChangeset& operator>>(QDBusArgument& argument) const;

private:

    int       m_id;
    Operation m_operation;
};

class DIGIKAM_DATABASE_EXPORT TagChangeset
{
public:

    enum Operation
    {
        Unknown,
        Added,
        Deleted,
        Renamed,
        Reparented,
        IconChanged
    };

    TagChangeset();
    TagChangeset(int albumId, Operation operation);

    int tagId() const;
    Operation operation() const;

    TagChangeset& operator<<(const QDBusArgument& argument);
    const TagChangeset& operator>>(QDBusArgument& argument) const;

private:

    int       m_id;
    Operation m_operation;
};

class DIGIKAM_DATABASE_EXPORT AlbumRootChangeset
{
public:

    enum Operation
    {
        Unknown,
        Added,
        Deleted,
        PropertiesChanged
    };

    AlbumRootChangeset();
    AlbumRootChangeset(int albumRootId, Operation operation);

    int albumRootId() const;
    Operation operation() const;

    AlbumRootChangeset& operator<<(const QDBusArgument& argument);
    const AlbumRootChangeset& operator>>(QDBusArgument& argument) const;

private:

    int       m_id;
    Operation m_operation;
};

class DIGIKAM_DATABASE_EXPORT SearchChangeset
{
public:

    enum Operation
    {
        Unknown,
        Added,
        Deleted,
        Changed
    };

    SearchChangeset();
    SearchChangeset(int searchId, Operation operation);

    int searchId() const;
    Operation operation() const;

    SearchChangeset& operator<<(const QDBusArgument& argument);
    const SearchChangeset& operator>>(QDBusArgument& argument) const;

private:

    int       m_id;
    Operation m_operation;
};

} // namespace Digikam

// custom macro from our dbusutilities.h
DECLARE_METATYPE_FOR_DBUS(Digikam::ImageChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::ImageTagChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::CollectionImageChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::AlbumChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::TagChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::SearchChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::AlbumRootChangeset)
DECLARE_METATYPE_FOR_DBUS(Digikam::DatabaseFields::Set)

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<qlonglong>)

#endif //DATABASECHANGESETS_H
