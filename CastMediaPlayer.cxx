/*
    Copyright 2015 rundgong

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "CastMediaPlayer.h"
#include "RLog.h"
#include <iostream>
#include <fstream>
#include "Utils.h"
#include "json/json.h"

static const std::string rCastReceiverApp = "rcast_player";


ReceiverHandler::ReceiverHandler()
  :  mLatestRequestId(0)
{
}

int
ReceiverHandler::onCastMessage( CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "ReceiverHandler::onCastMessage" )

    Json::Reader jsonReader;
    Json::Value castPayloadJson;
    jsonReader.parse( castMessage.payload_utf8(), castPayloadJson );

    uint32_t requestId = castPayloadJson["requestId"].asUInt();
    std::string type = castPayloadJson["type"].asString();

    if( type != "RECEIVER_STATUS" ){ return 0; }    // Unknown message type

    std::string appId = castPayloadJson["status"]["applications"][0]["appId"].asString();

    if( appId == rCastReceiverApp )
    {
        mSessionId = castPayloadJson["status"]["applications"][0]["sessionId"].asString();
        mTransportId = castPayloadJson["status"]["applications"][0]["transportId"].asString();
    }
    else
    {
        // When the idle app, or other app, starts we are not interested in it's session
        mSessionId = "";
        mTransportId = "";
    }


    RLOG(rlog::Verbose,
            "  requestId = " << requestId << std::endl
         << "  type = " << type << std::endl
         << "  sessionId = " << mSessionId << std::endl
         << "  transportId = " << mTransportId << std::endl )

    if( requestId != 0 )
    {
        mLatestRequestId = requestId;
    }

    return 0;
}

void ReceiverHandler::reset()
{
    mLatestRequestId = 0;
    mSessionId = "";
    mTransportId = "";
}

MediaHandler::MediaHandler()
{
    reset();
}

void
MediaHandler::reset()
{
    mLatestRequestId = 0;
    mMediaSessionId = 0;
    mPlayerState = IDLE;
}

void MediaHandler::setFinishedCallback(std::function<void()> finishedCallback)
{
    mFinishedCallback = finishedCallback;
}

int
MediaHandler::onCastMessage( CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "MediaHandler::onCastMessage" )

    Json::Reader jsonReader;
    Json::Value castPayloadJson;
    jsonReader.parse( castMessage.payload_utf8(), castPayloadJson );

    uint32_t requestId = castPayloadJson["requestId"].asUInt();
    std::string type = castPayloadJson["type"].asString();

    if( type != "MEDIA_STATUS" ){ return 0; }    // Unknown message type

    std::string newPlayerState = castPayloadJson["status"][0]["playerState"].asString();
    mMediaSessionId = castPayloadJson["status"][0]["mediaSessionId"].asUInt();

    if( newPlayerState == "IDLE" || newPlayerState == "" )
    {
        mPlayerState = IDLE;

        std::string idleReason = castPayloadJson["status"][0]["idleReason"].asString();
        if( idleReason == "FINISHED" )
        {
            if(mFinishedCallback) mFinishedCallback();
        }
    }

    if( newPlayerState == "BUFFERING"  )
    {
        mPlayerState = BUFFERING;
    }

    if( newPlayerState == "PLAYING"  )
    {
        mPlayerState = PLAYING;
    }

    if( newPlayerState == "PAUSED"  )
    {
        mPlayerState = PAUSED;
    }

    RLOG(rlog::Verbose, "Media Status: mediaSessionId=" << mMediaSessionId
                      << ", playerState=" << newPlayerState)

    if( requestId != 0 )
    {
        mLatestRequestId = requestId;
    }

    return 0;
}



CastMediaPlayer::CastMediaPlayer(const std::string& host, uint16_t port)
 :  mPlayListIndex(0), mHost(host), mPort(port), mRequestId(0)
{
    loadPlayList();
    mMediaHandler.setFinishedCallback(
        [this]()
        {
            RLOG_N("Video #" << mPlayListIndex << " finished. Auto play next")
            next();
        }
    );
}

CastMediaPlayer::~CastMediaPlayer()
{
    stop(); // Stop any playing videos before disconnect
}



void
CastMediaPlayer::loadPlayList()
{
    RLOG_N( "Load playlist" )
#ifdef RCAST_NATIVE
    std::ifstream playListStream("./playlist.txt");
#else
    std::ifstream playListStream("/data/rcast/playlist.txt");
#endif

    std::string line;

    while( !playListStream.eof() && !playListStream.fail() )
    {
        std::getline(playListStream,line);
        line = trim(line);  // Remove leading and trailing white space

        if( line.size() == 0 )
        {
            continue;
        }

        RLOG_N( "Load playlist #" << mPlayList.size() << " - " << line << ".")

        mPlayList.push_back(line);
    }
    RLOG(rlog::Verbose, "Load playlist finished" )
}

void
CastMediaPlayer::loadMediaFromPlaylist()
{
    if( ! verifyPlaylist() ) return;

    RLOG_N( "Load Media #" << mPlayListIndex << " - " << mPlayList[mPlayListIndex]  )

    mediaLoad( mPlayList[mPlayListIndex] );
}

static std::string getLaunchReceiverPayload(uint32_t requestId, const std::string& appId )
{
    Json::Value loadPayload;
    loadPayload["requestId"] = requestId;
    loadPayload["type"] = "LAUNCH";
    loadPayload["appId"] = appId;

    return getJsonString(loadPayload);
}

static std::string getMediaLoadPayload(uint32_t requestId, const std::string& videoUrl )
{
    Json::Value loadPayload;
    loadPayload["requestId"] = requestId;
    loadPayload["type"] = "LOAD";
    loadPayload["media"] = Json::Value();
    loadPayload["media"]["contentId"] = videoUrl;
    loadPayload["media"]["streamType"] = "NONE";
    loadPayload["media"]["contentType"] = "video/mp4";

    return getJsonString(loadPayload);
}

static std::string getMediaPlayPayload(uint32_t requestId, uint32_t mediaSessionId )
{
    Json::Value loadPayload;
    loadPayload["requestId"] = requestId;
    loadPayload["mediaSessionId"] = mediaSessionId;
    loadPayload["type"] = "PLAY";

    return getJsonString(loadPayload);
}

static std::string getMediaPausePayload(uint32_t requestId, uint32_t mediaSessionId )
{
    Json::Value loadPayload;
    loadPayload["requestId"] = requestId;
    loadPayload["mediaSessionId"] = mediaSessionId;
    loadPayload["type"] = "PAUSE";

    return getJsonString(loadPayload);
}

void
CastMediaPlayer::verifyMediaConnection()
{
    if( !mCastLink || !mCastLink->isConnected() )
    {
        //mCastLink = std::shared_ptr<CastLink>( new CastLink(mHost, mPort) );
        mCastLink.reset( new CastLink(mHost, mPort) );
        mCastLink->addCallback(&mReceiverHandler);
        mCastLink->addCallback(&mMediaHandler);

    }

    if( mReceiverHandler.sessionId() == "" )
    {
        receiverLaunch(rCastReceiverApp);
    }
}

uint32_t
CastMediaPlayer::getNextRequestId()
{
    return ++mRequestId;
}

void
CastMediaPlayer::playOrPause()
{
    RLOG_N( "PLAY/PAUSE" )
    if( ! verifyPlaylist() ) return;

    verifyMediaConnection();

    if( mMediaHandler.playerState() == MediaHandler::BUFFERING
        || mMediaHandler.playerState() == MediaHandler::PLAYING     )
    {
        mediaPause();
    }
    else if( mMediaHandler.playerState() == MediaHandler::PAUSED )
    {
        mediaPlay();
    }
    else    // IDLE
    {
        loadMediaFromPlaylist();
    }
}

void
CastMediaPlayer::stop()
{
    RLOG_N( "STOP" )

    if( mReceiverHandler.sessionId() != "" )
    {
        receiverStop();
        mReceiverHandler.reset();
        mMediaHandler.reset();
    }

    mCastLink.reset();  // Drop connection to CC when we stop, and connect again when needed

}

void
CastMediaPlayer::next()
{
    RLOG_N( "NEXT" )
    if( ! verifyPlaylist() ) return;

    mPlayListIndex = (mPlayListIndex + 1) % mPlayList.size();

    loadMediaFromPlaylist();
}

void
CastMediaPlayer::previous()
{
    RLOG_N( "PREVIOUS" )
    if( ! verifyPlaylist() ) return;

    if( mPlayListIndex == 0 )
    {
        mPlayListIndex = mPlayList.size() -1;
    }
    else
    {
        --mPlayListIndex;
    }

    loadMediaFromPlaylist();
}

void CastMediaPlayer::getStatus()
{
    RLOG(rlog::Debug, "CastMediaPlayer::getStatus" )

    auto castMessage = getMediaCastMessage();

    Json::Value statusPayload;
    statusPayload["requestId"] = getNextRequestId();
    statusPayload["type"] = "GET_STATUS";

    castMessage.set_payload_utf8( getJsonString(statusPayload) );
    mCastLink->send(castMessage);
}

extensions::api::cast_channel::CastMessage
CastMediaPlayer::getMediaCastMessage()
{
    extensions::api::cast_channel::CastMessage mediaCastMessage =
        getCastMessage( CastLink::sDefaultSender,
                        mReceiverHandler.transportId().c_str(),
                        MediaHandler::sNameSpace);

    return mediaCastMessage;
}

extensions::api::cast_channel::CastMessage
CastMediaPlayer::getReceiverCastMessage()
{
    extensions::api::cast_channel::CastMessage mediaCastMessage =
        getCastMessage( CastLink::sDefaultSender,
                        CastLink::sDefaultReceiver,
                        ReceiverHandler::sNameSpace);

    return mediaCastMessage;
}

void
CastMediaPlayer::receiverLaunch(const std::string& receiverApp)
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverLaunch " << receiverApp )

    auto castMessage = getReceiverCastMessage();
    std::string payload = getLaunchReceiverPayload(getNextRequestId(), receiverApp);
    castMessage.set_payload_utf8(payload);

    mCastLink->send(castMessage);

    while( mReceiverHandler.latestRequestId() != mRequestId ){ ; }  // Wait for response

    mCastLink->addDestination( mReceiverHandler.transportId() );

}


void
CastMediaPlayer::mediaLoad( const std::string& mediaUrl )
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaLoad " << mediaUrl )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaLoadPayload(getNextRequestId(), mediaUrl);
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::mediaPlay()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaPlay " )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaPlayPayload(getNextRequestId(), mMediaHandler.mediaSessionId());
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::mediaPause()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaPause " )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaPausePayload(getNextRequestId(), mMediaHandler.mediaSessionId());
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::receiverStop()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverStop " )

    auto castMessage = getReceiverCastMessage();

    Json::Value statusPayload;
    statusPayload["requestId"] = getNextRequestId();
    statusPayload["type"] = "STOP";
    statusPayload["sessionId"] = mReceiverHandler.sessionId();

    castMessage.set_payload_utf8( getJsonString(statusPayload) );
    mCastLink->send(castMessage);

    // TODO wait for response? remove transportId from cast link
    mReceiverHandler.reset();
}

bool
CastMediaPlayer::verifyPlaylist()
{
    if(mPlayList.size() == 0)
    {
        RLOG(rlog::Critical, "Playlist Error: Playlist size=0" )
        return false;
    }
    if( mPlayListIndex >= mPlayList.size())
    {
        RLOG(rlog::Critical, "Playlist Error: Playlist size=" << mPlayList.size()
                << ", index=" << mPlayListIndex )
        return false;
    }

    return true;
}

