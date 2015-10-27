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

#ifndef RCAST_CHROMECASTINPUT_H_
#define RCAST_CHROMECASTINPUT_H_

#include <vector>
#include <thread>

enum class ChromecastButtonPress
{
    Short,
    Long
};

class ChromecastButtonCallback
{
public:
    virtual ~ChromecastButtonCallback(){};

    // Callback with one event consisting of multiple button presses
    virtual void onEvent(std::vector<ChromecastButtonPress> inputEvent) = 0;
};

class ChromecastButton
{
public:
    ChromecastButton( ChromecastButtonCallback* callback );
    ~ChromecastButton();

private:

    void readLoop();

    std::shared_ptr<std::thread> mReadThread;
    ChromecastButtonCallback* mCallback;
    bool mContinueReading;

    uint32_t mButtonSequenceTimeout;    // microseconds
    uint32_t mLongPressTime;            // microseconds
};

#endif /* RCAST_CHROMECASTINPUT_H_ */
