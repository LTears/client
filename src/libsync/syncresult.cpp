/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "syncresult.h"
#include "progressdispatcher.h"

namespace OCC
{

SyncResult::SyncResult()
    : _status( Undefined )
    , _foundFilesNotSynced(false)
    , _folderStructureWasChanged(false)
    , _numNewItems(0)
    , _numRemovedItems(0)
    , _numUpdatedItems(0)
    , _numRenamedItems(0)
    , _numConflictItems(0)
    , _numErrorItems(0)

{
}

SyncResult::Status SyncResult::status() const
{
    return _status;
}

void SyncResult::reset()
{
    *this = SyncResult();
}

QString SyncResult::statusString() const
{
    QString re;
    Status stat = status();

    switch( stat ){
    case Undefined:
        re = QLatin1String("Undefined");
        break;
    case NotYetStarted:
        re = QLatin1String("Not yet Started");
        break;
    case SyncRunning:
        re = QLatin1String("Sync Running");
        break;
    case Success:
        re = QLatin1String("Success");
        break;
    case Error:
        re = QLatin1String("Error");
        break;
    case SetupError:
        re = QLatin1String("SetupError");
        break;
    case SyncPrepare:
        re = QLatin1String("SyncPrepare");
        break;
    case Problem:
        re = QLatin1String("Success, some files were ignored.");
        break;
    case SyncAbortRequested:
        re = QLatin1String("Sync Request aborted by user");
        break;
    case Paused:
        re = QLatin1String("Sync Paused");
        break;
    }
    return re;
}

void SyncResult::setStatus( Status stat )
{
    _status = stat;
    _syncTime = QDateTime::currentDateTime();
}

QDateTime SyncResult::syncTime() const
{
    return _syncTime;
}

QStringList SyncResult::errorStrings() const
{
    return _errors;
}

void SyncResult::appendErrorString( const QString& err )
{
    _errors.append( err );
}

QString SyncResult::errorString() const
{
    if( _errors.isEmpty() ) return QString::null;
    return _errors.first();
}

void SyncResult::clearErrors()
{
    _errors.clear();
}

void SyncResult::setFolder(const QString& folder)
{
    _folder = folder;
}

QString SyncResult::folder() const
{
    return _folder;
}

void SyncResult::processCompletedItem(const SyncFileItemPtr &item)
{
    if (Progress::isWarningKind(item->_status)) {
        // Count any error conditions, error strings will have priority anyway.
        _foundFilesNotSynced = true;
    }

    if (item->_isDirectory && (item->_instruction == CSYNC_INSTRUCTION_NEW
            || item->_instruction == CSYNC_INSTRUCTION_TYPE_CHANGE
            || item->_instruction == CSYNC_INSTRUCTION_REMOVE
            || item->_instruction == CSYNC_INSTRUCTION_RENAME)) {
        _folderStructureWasChanged = true;
    }

    // Process the item to the gui
    if( item->_status == SyncFileItem::FatalError || item->_status == SyncFileItem::NormalError ) {
        //: this displays an error string (%2) for a file %1
        appendErrorString( QObject::tr("%1: %2").arg(item->_file, item->_errorString) );
        _numErrorItems++;
        if (!_firstItemError) {
            _firstItemError = item;
        }
    } else if( item->_status == SyncFileItem::Conflict ) {
        _numConflictItems++;
        if (!_firstConflictItem) {
            _firstConflictItem = item;
        }
    } else {
        if (!item->hasErrorStatus() && item->_status != SyncFileItem::FileIgnored && item->_direction == SyncFileItem::Down) {
            switch (item->_instruction) {
            case CSYNC_INSTRUCTION_NEW:
            case CSYNC_INSTRUCTION_TYPE_CHANGE:
                _numNewItems++;
                if (!_firstItemNew)
                    _firstItemNew = item;
                break;
            case CSYNC_INSTRUCTION_REMOVE:
                _numRemovedItems++;
                if (!_firstItemDeleted)
                    _firstItemDeleted = item;
                break;
            case CSYNC_INSTRUCTION_SYNC:
                _numUpdatedItems++;
                if (!_firstItemUpdated)
                    _firstItemUpdated = item;
                break;
            case CSYNC_INSTRUCTION_RENAME:
                if (!_firstItemRenamed) {
                    _firstItemRenamed = item;
                }
                _numRenamedItems++;
                break;
            default:
                // nothing.
                break;
            }
        } else if( item->_direction == SyncFileItem::None ) {
            if( item->_instruction == CSYNC_INSTRUCTION_IGNORE ) {
                _foundFilesNotSynced = true;
            }
        }
    }

}

} // ns mirall
