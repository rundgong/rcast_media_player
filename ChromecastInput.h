/*
 * ChromecastInput.h
 *
 *  Created on: Sep 28, 2015
 *      Author: markus
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
