/*
 * RLog.cxx
 *
 *  Created on: Oct 1, 2015
 *      Author: markus
 */

#include "RLog.h"
#include <iostream>

namespace rlog
{
    LogLevel logLevel = Normal;
    bool networkLogEnabled = false;

    std::ostream* logStream = &std::cout;
}

