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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509.h>
#include <openssl/buffer.h>
#include <openssl/x509v3.h>
#include <openssl/opensslconf.h>

#include <string>
#include <iostream>
#include <arpa/inet.h>
#include "cast_channel.pb.h"
#include "SslWrapper.h"
#include "CastLink.h"
#include "CastMediaPlayer.h"
#include "json/json.h"
#include "ChromecastInput.h"
#include "RLog.h"
#include <fstream>


class ButtonEventHandler : public ChromecastButtonCallback
{
public:
    ButtonEventHandler(CastMediaPlayer* castMediaPlayer)
     : mCastMediaPlayer(castMediaPlayer)
    {
    }

    virtual void onEvent(std::vector<ChromecastButtonPress> inputEvent) override
    {
        static const std::vector<ChromecastButtonPress> playOrPauseEvent = { ChromecastButtonPress::Short };
        static const std::vector<ChromecastButtonPress> stopEvent = { ChromecastButtonPress::Long };
        static const std::vector<ChromecastButtonPress> nextEvent = { ChromecastButtonPress::Short, ChromecastButtonPress::Short };
        static const std::vector<ChromecastButtonPress> previousEvent = { ChromecastButtonPress::Short, ChromecastButtonPress::Short, ChromecastButtonPress::Short };

        std::ostringstream oss;
        for( auto buttonPress : inputEvent )
        {
            oss << (int)buttonPress << " ";
        }
        RLOG_N("Button Event: " << oss.str())

        if( inputEvent == playOrPauseEvent ) { mCastMediaPlayer->playOrPause(); }
        if( inputEvent == stopEvent )        { mCastMediaPlayer->stop(); }
        if( inputEvent == nextEvent )        { mCastMediaPlayer->next(); }
        if( inputEvent == previousEvent )    { mCastMediaPlayer->previous(); }
    }

private:
    CastMediaPlayer* mCastMediaPlayer;
};

static std::ofstream* sFileLogStream = 0;
static std::string sChromecastHost = "127.0.0.1";

int handleArguments(int argc, char* argv[])
{
    int i;
    for( i=1; i<argc; ++i )
    {
        if(      std::string(argv[i]) == "--log=on" )   { rlog::logLevel = rlog::Normal; }
        else if( std::string(argv[i]) == "--log=off" )  { rlog::logLevel = rlog::Off;}
        else if( std::string(argv[i]) == "--log=file" )
        {
#ifdef RCAST_NATIVE
            sFileLogStream = new std::ofstream("./log.txt");
#else
            sFileLogStream = new std::ofstream("/data/rcast/log.txt", std::ios_base::app);
#endif
            rlog::logStream = sFileLogStream;
        }
        else if( std::string(argv[i]) == "--log-level=off" )        { rlog::logLevel = rlog::Off; }
        else if( std::string(argv[i]) == "--log-level=critical" )   { rlog::logLevel = rlog::Critical; }
        else if( std::string(argv[i]) == "--log-level=important" )  { rlog::logLevel = rlog::Important; }
        else if( std::string(argv[i]) == "--log-level=normal" )     { rlog::logLevel = rlog::Normal; }
        else if( std::string(argv[i]) == "--log-level=verbose" )    { rlog::logLevel = rlog::Verbose; }
        else if( std::string(argv[i]) == "--log-level=debug" )      { rlog::logLevel = rlog::Debug; }
        else if( std::string(argv[i]) == "--log-level=all" )        { rlog::logLevel = rlog::All; }
        else if( std::string(argv[i]) == "--log-network" )          { rlog::networkLogEnabled = true; }
        else if( std::string(argv[i]).substr(0,7) == "--host=" )
        {
            sChromecastHost = std::string(argv[i]).substr(7);
        }
        else
        {
            std::cout << "Unknown argument: " << argv[i] << std::endl;
            return 1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    if( handleArguments(argc,argv)  ) return 1;

    RLOG(rlog::Important, "rCast Player started" )
    RLOG(rlog::Important, "Chromecast host: " << sChromecastHost )

    CastMediaPlayer castMediaPlayer(sChromecastHost, 8009);
    bool run = true;

#ifdef RCAST_NATIVE

    while(run)
    {
        int c = getchar();

        if( c == ' ' ){ castMediaPlayer.playOrPause(); }
        if( c == 's' ){ castMediaPlayer.stop(); }
        if( c == 'n' ){ castMediaPlayer.next(); }
        if( c == 'p' ){ castMediaPlayer.previous(); }
        if( c == 'q' ){ run = false; }
    }

#else

    ButtonEventHandler buttonEventHandler(&castMediaPlayer);
    ChromecastButton chromecastButton(&buttonEventHandler);

    while(run)
    {
        usleep(1000000);
    }

#endif

    RLOG(rlog::Important, "rCast Player exit" )

    if( sFileLogStream != 0 )
    {
        rlog::logStream = &std::cout;   // Make sure nothing accidentally logs to file after it is deleted
        delete sFileLogStream;
    }
    return 0;
}

