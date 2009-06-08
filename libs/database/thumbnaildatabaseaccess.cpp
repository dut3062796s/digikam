/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-03-18
 * Description : Database access wrapper.
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

#include "thumbnaildatabaseaccess.h"

// Qt includes

#include <QMutex>
#include <QSqlDatabase>

// KDE includes

#include <kdebug.h>
#include <klocale.h>

// Local includes

#include "databasecorebackend.h"
#include "thumbnaildb.h"
#include "thumbnailschemaupdater.h"

namespace Digikam
{

class ThumbnailDatabaseAccessStaticPriv
{
public:

    ThumbnailDatabaseAccessStaticPriv()
        : backend(0), db(0),
          mutex(QMutex::Recursive), // create a recursive mutex
          initializing(false), lockCount(0)
    {}
    ~ThumbnailDatabaseAccessStaticPriv() {};

    DatabaseCoreBackend*backend;
    ThumbnailDB        *db;
    DatabaseParameters  parameters;
    QMutex              mutex;
    QString             lastError;

    bool                initializing;
    int                 lockCount;
};

class ThumbnailDatabaseAccessMutexLocker : public QMutexLocker
{
public:

    ThumbnailDatabaseAccessMutexLocker(ThumbnailDatabaseAccessStaticPriv *d)
        : QMutexLocker(&d->mutex), d(d)
    {
        d->lockCount++;
    }

    ~ThumbnailDatabaseAccessMutexLocker()
    {
        d->lockCount--;
    }

    ThumbnailDatabaseAccessStaticPriv* const d;
};

ThumbnailDatabaseAccessStaticPriv *ThumbnailDatabaseAccess::d = 0;

ThumbnailDatabaseAccess::ThumbnailDatabaseAccess()
{
    Q_ASSERT(d/*You will want to call setParameters before constructing ThumbnailDatabaseAccess*/);
    d->mutex.lock();
    d->lockCount++;
    if (!d->backend->isOpen() && !d->initializing)
    {
        // avoid endless loops
        d->initializing = true;

        d->backend->open(d->parameters);

        d->initializing = false;
    }
}

ThumbnailDatabaseAccess::~ThumbnailDatabaseAccess()
{
    d->lockCount--;
    d->mutex.unlock();
}

ThumbnailDatabaseAccess::ThumbnailDatabaseAccess(bool)
{
    // private constructor, when mutex is locked and
    // backend should not be checked
    d->mutex.lock();
    d->lockCount++;
}

ThumbnailDB *ThumbnailDatabaseAccess::db() const
{
    return d->db;
}

DatabaseCoreBackend *ThumbnailDatabaseAccess::backend() const
{
    return d->backend;
}

DatabaseParameters ThumbnailDatabaseAccess::parameters()
{
    if (d)
        return d->parameters;
    return DatabaseParameters();
}

void ThumbnailDatabaseAccess::setParameters(const DatabaseParameters& parameters)
{
    if (!d)
    {
        d = new ThumbnailDatabaseAccessStaticPriv();
    }

    ThumbnailDatabaseAccessMutexLocker lock(d);

    if (d->parameters == parameters)
        return;

    if (d->backend && d->backend->isOpen())
        d->backend->close();

    d->parameters = parameters;

    if (!d->backend || !d->backend->isCompatible(parameters))
    {
        delete d->db;
        delete d->backend;
        d->backend = new DatabaseCoreBackend();
        d->db = new ThumbnailDB(d->backend);
    }
}

bool ThumbnailDatabaseAccess::checkReadyForUse(InitializationObserver *observer)
{
    QStringList drivers = QSqlDatabase::drivers();
    if (!drivers.contains("QSQLITE"))
    {
        kError(50003) << "No SQLite3 driver available. List of QSqlDatabase drivers: " << drivers << endl;
        d->lastError = i18n("The driver \"SQLITE\" for SQLite3 databases is not available.\n"
                            "digiKam depends on the drivers provided by the SQL module of Qt4.");
        return false;
    }

    // create an object with private shortcut constructor
    ThumbnailDatabaseAccess access(false);

    if (!d->backend)
    {
        kWarning(50003) << "No database backend available in checkReadyForUse. "
                           "Did you call setParameters before?" << endl;
        return false;
    }
    if (d->backend->isReady())
        return true;
    if (!d->backend->isOpen())
    {
        if (!d->backend->open(d->parameters))
        {
            access.setLastError(i18n("Error opening database backend.\n ")
                                + d->backend->lastError());
            return false;
        }
    }

    // avoid endless loops (if called methods create new ThumbnailDatabaseAccess objects)
    d->initializing = true;

    // update schema
    ThumbnailSchemaUpdater updater(&access);
    updater.setObserver(observer);
    if (!d->backend->initSchema(&updater))
    {
        d->initializing = false;
        return false;
    }

    d->initializing = false;

    return d->backend->isReady();
}

QString ThumbnailDatabaseAccess::lastError()
{
    return d->lastError;
}

void ThumbnailDatabaseAccess::setLastError(const QString& error)
{
    d->lastError = error;
}

void ThumbnailDatabaseAccess::cleanUpDatabase()
{
    if (d)
    {
        ThumbnailDatabaseAccessMutexLocker locker(d);
        d->backend->close();
        delete d->db;
        delete d->backend;
    }
    delete d;
    d = 0;
}

}  // namespace Digikam
