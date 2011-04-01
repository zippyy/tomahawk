/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCANMANAGER_H
#define SCANMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMap>

#include "dllmacro.h"

class MusicScanner;
class QThread;
class QFileSystemWatcher;

class ScanManager : public QObject
{
Q_OBJECT

public:
    static ScanManager* instance();

    explicit ScanManager( QObject* parent = 0 );
    virtual ~ScanManager();
    
    void runManualScan( const QStringList& path );

signals:
    void finished();
    
public slots:
    void handleChangedDir( const QString& path );
    void addWatchedDirs( const QStringList& paths );
    void removeWatchedDir( const QString& path );
    void setInitialPaths( QMap<QString, unsigned int> pathMap );

private slots:
    void scannerQuit();
    void scannerFinished();
    void scannerDestroyed( QObject* scanner );
    
    void startupWatchPaths();

    void onSettingsChanged();
    
private:
    static ScanManager* s_instance;
    
    MusicScanner* m_scanner;
    QThread* m_musicScannerThreadController;
    QStringList m_currScannerPath;
    QFileSystemWatcher* m_dirWatcher;
};

#endif
