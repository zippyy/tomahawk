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

#include "whatshotwidget.h"
#include "ui_whatshotwidget.h"

#include <QPainter>

#include "viewmanager.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "RecentPlaylistsModel.h"

#include "audio/audioengine.h"
#include "playlist/playlistmodel.h"
#include "playlist/treeproxymodel.h"
#include "widgets/overlaywidget.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include <dynamic/GeneratorInterface.h>

#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;

static QString s_whatsHotIdentifier = QString( "WhatsHotWidget" );

WhatsHotWidget::WhatsHotWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WhatsHotWidget )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout_3->layout() );


    m_tracksModel = new PlaylistModel( ui->tracksView );
    m_tracksModel->setStyle( TrackModel::Short );

    ui->tracksView->setFrameShape( QFrame::NoFrame );
    ui->tracksView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->tracksView->overlay()->setEnabled( false );
    ui->tracksView->setTrackModel( m_tracksModel );
    ui->tracksView->setHeaderHidden( true );
    ui->tracksView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );


    m_artistsModel = new TreeModel( ui->artistsView );
    m_artistsModel->setColumnStyle( TreeModel::TrackOnly );

    m_artistsProxy = new TreeProxyModel( ui->artistsView );
    m_artistsProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_artistsProxy->setDynamicSortFilter( true );

    ui->artistsView->setProxyModel( m_artistsProxy );
    ui->artistsView->setTreeModel( m_artistsModel );
    ui->artistsView->setFrameShape( QFrame::NoFrame );
    ui->artistsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );


    m_artistsProxy->sort( -1 ); // disable sorting, must be called after artistsView->setTreeModel

    ui->artistsView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->artistsView->header()->setVisible( false );


    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), SLOT( checkQueries() ) );


    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );


    QTimer::singleShot( 0, this, SLOT( fetchData() ) );
}


WhatsHotWidget::~WhatsHotWidget()
{
    delete ui;
}

void
WhatsHotWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoCriteriaHash artistInfo;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoChartArtists;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    requestData.type = Tomahawk::InfoSystem::InfoChartTracks;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    tDebug() << "WhatsHot: requested InfoChartArtists+Tracks";
}


void
WhatsHotWidget::checkQueries()
{
    m_timer->stop();
    m_tracksModel->ensureResolved();
}


void
WhatsHotWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_whatsHotIdentifier )
    {
//        tDebug() << "Info of wrong type or not with our identifier";
        return;
    }

    tDebug() << "WhatsHot: got something...";
    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoChartArtists:
        {
            const QStringList artists = returnedData["artists"].toStringList();
            tDebug() << "WhatsHot: got artists! " << artists.size();
            tDebug() << artists;
            foreach ( const QString& artist, artists )
            {
                m_artistsModel->addArtists( Artist::get( artist ) );
            }
            break;
        }
        case InfoSystem::InfoChartTracks:
        {
            const QList<Tomahawk::InfoSystem::ArtistTrackPair> tracks = returnedData["tracks"].value<QList<Tomahawk::InfoSystem::ArtistTrackPair> >();
            tDebug() << "WhatsHot: got tracks! " << tracks.size();
            foreach ( const Tomahawk::InfoSystem::ArtistTrackPair& track, tracks )
            {
                query_ptr query = Query::get( track.artist, track.track, QString(), uuid() );
                m_tracksModel->append( query );
            }
            break;
        }
        default:
            return;
    }
}


void
WhatsHotWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
WhatsHotWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


