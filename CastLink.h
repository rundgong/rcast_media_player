/*
 * CastLink.h
 *
 *  Created on: 12 aug 2015
 *      Author: markus
 */

#ifndef CASTLINK_H_
#define CASTLINK_H_

#include <thread>
#include <memory>
#include <list>
#include "cast_channel.pb.h"
#include "SslWrapper.h"

using namespace extensions::api::cast_channel;

extensions::api::cast_channel::CastMessage
getCastMessage(const char* sourceId, const char* destinationId, const char* nameSpace);

class CastLink;

class CastMessageHandler
{
public:
    virtual ~CastMessageHandler(){}

    virtual std::string nameSpace() = 0;
    virtual int onCastMessage( CastLink* castLink, const CastMessage& castMessage ) = 0;
};


class HeartBeatHandler : public CastMessageHandler
{
public:
    static constexpr auto sNameSpace = "urn:x-cast:com.google.cast.tp.heartbeat";

    std::string nameSpace(){ return sNameSpace; }
    int onCastMessage( CastLink* castLink, const CastMessage& castMessage );
};

class ConnectionHandler : public CastMessageHandler
{
public:
    static constexpr auto sNameSpace = "urn:x-cast:com.google.cast.tp.connection";

    std::string nameSpace(){ return sNameSpace; }
    int onCastMessage( CastLink* castLink, const CastMessage& castMessage );
};


struct CastCallback
{
    std::string nameSpace;
    CastMessageHandler* receiverPtr;
};

class CastLink
{
public:
    static constexpr auto sDefaultSender = "sender-0";
    static constexpr auto sDefaultReceiver = "receiver-0";

    CastLink(const std::string& host, uint16_t port = 8009);
    ~CastLink();

    void send( const CastMessage& castMessage );
    void addCallback( CastMessageHandler* receiverPtr );

    // Must connect to destination before sending messages there
    void addDestination(const std::string& destination );

    bool isConnected();

private:

    void init();

    void receiverLoop();
    uint32_t readMessageLength();
    void readPayload( std::vector<uint8_t>&  messageBuffer );
    void dispatchCastMessage(const CastMessage& castMessage);

    std::list<CastCallback> mCastCallbacks;
    std::shared_ptr<SslWrapper> mSslWrapper;
    std::shared_ptr<std::thread> mReceiverThread;
    bool mIsConnected;

    HeartBeatHandler mHeartBeatHandler;
    ConnectionHandler mConnectionHandler;

    const static int sCastHeaderLength = 4;

};


#endif /* CASTLINK_H_ */
