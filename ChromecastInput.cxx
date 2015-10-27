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

#include "ChromecastInput.h"

#include <stdio.h>
#include <linux/input.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "RLog.h"

static const uint32_t BUTTON_RELEASED = 0;
static const uint32_t BUTTON_PRESSED = 1;

// Times in microseconds
static const uint32_t DEFAULT_TIMEOUT = 1000000; // 1 second
static const uint32_t READ_SLEEP = 10000;        // 0.01 second


ChromecastButton::ChromecastButton( ChromecastButtonCallback* callback )
  : mCallback(callback),
    mContinueReading(true),
    mButtonSequenceTimeout(DEFAULT_TIMEOUT),
    mLongPressTime(DEFAULT_TIMEOUT)
{
    mReadThread.reset( new std::thread(
            [this]()
            {
                this->readLoop();
            }
        ));
}

ChromecastButton::~ChromecastButton()
{
    mContinueReading = false;
    mReadThread->join();
}

void
ChromecastButton::readLoop()
{
    int inputFD;
    struct input_event input;
    size_t readCount;

    uint32_t timeoutTickCount = 0;
    uint32_t timeoutTickLimit = mButtonSequenceTimeout/READ_SLEEP;

    std::vector<ChromecastButtonPress> inputEvent;

    bool buttonIsPressed = false;
    uint32_t pressTime; // Time in micro sec

    // The Chromecast button triggers events for /dev/input/event0
    inputFD = open("/dev/input/event0", O_RDONLY | O_NONBLOCK );

    while(mContinueReading)
    {
        readCount = read(inputFD, &input, sizeof(struct input_event) );

        if( readCount != sizeof(struct input_event) )   // There were nothing to read from inputFD
        {
            usleep(READ_SLEEP);

            // Only count ticks when button is not pressed
            // and there is an event waiting to be posted
            if( !buttonIsPressed && inputEvent.size()>0)
            {
                ++timeoutTickCount;
            }

            if( timeoutTickCount > timeoutTickLimit )
            {
                RLOG(rlog::Debug, "ChromecastButton timeout" )
                mCallback->onEvent(inputEvent);
                inputEvent.clear();
                timeoutTickCount = 0;
            }

            continue;
        }

        // Key event for Misc Button is what we are interested in
        if(input.type != EV_KEY) continue;
        if(input.code != BTN_MISC) continue;

        if( input.value == BUTTON_PRESSED )
        {
            buttonIsPressed = true;
            pressTime = input.time.tv_sec * 1000000 + input.time.tv_usec;
            RLOG(rlog::Debug, "ChromecastButton pressed" )
        }

        if( input.value == BUTTON_RELEASED )
        {
            buttonIsPressed = false;
            uint32_t releaseTime = input.time.tv_sec * 1000000 + input.time.tv_usec;
            uint32_t timeDiff = releaseTime - pressTime;

            RLOG(rlog::Debug, "ChromecastButton released" )
            if( timeDiff > mLongPressTime )
            {
                inputEvent.push_back(ChromecastButtonPress::Long);
                RLOG(rlog::Verbose, "ChromecastButton Long press" )
            }
            else
            {
                inputEvent.push_back(ChromecastButtonPress::Short);
                RLOG(rlog::Verbose, "ChromecastButton Short press" )
            }

        }
    }


}


