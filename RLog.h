/*
 * RLog.h
 *
 *  Created on: Oct 1, 2015
 *      Author: markus
 */

#ifndef RCAST_RLOG_H_
#define RCAST_RLOG_H_

#include <ostream>

namespace rlog
{
    enum LogLevel
    {
        Off = 0,
        Critical,
        Important,
        Normal,
        Verbose,
        Debug,
        All
    };

    extern LogLevel logLevel;
    extern bool networkLogEnabled;

    // Point this to any ostream to get logging there. Default std::cout
    extern std::ostream* logStream;

}

#define RLOG(_logLevel, _message)\
if( rlog::logLevel >= _logLevel )\
{\
    *rlog::logStream << _message << std::endl;\
}

#define RLOG_N(_message) RLOG(rlog::Normal, _message)

#define RLOG_NETWORK(_message)\
if( rlog::networkLogEnabled )\
{\
    *rlog::logStream << _message << std::endl;\
}


#endif /* RCAST_RLOG_H_ */
