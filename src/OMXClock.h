#pragma once

#include "Component.h"


static inline OMX_TICKS ToOMXTime(int64_t pts)
{
    OMX_TICKS ticks;
    ticks.nLowPart = pts;
    ticks.nHighPart = pts >> 32;
    return ticks;
}
static inline int64_t FromOMXTime(OMX_TICKS ticks)
{
    int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
    return pts;
}

class OMXClock
{

public:
    OMXClock();
    ~OMXClock();
    void lock();
    void unlock();
    bool init(bool has_video, bool has_audio);
    void deinit();
    bool isPaused()
    {
        return pauseState;
    };
    bool stop();
    bool start(double pts, int fps_);
    bool step(int steps = 1);
    //bool reset();
    double getMediaTime();
    bool setMediaTime(double pts);
    bool pause();
    bool resume();
    bool setSpeed(int speed_, bool doResume = false);
    int  getSpeed()
    {
        return currentSpeed;
    };
    Component* getComponent();
    bool OMXStateExecute();
    bool enableHDMISync();
    int64_t getAbsoluteClock();
    void sleep(unsigned int dwMilliSeconds);
    int getFrameCounter();
    
private:
    Component clockComponent;
    int fps;
    int startFrame;
    bool              pauseState;
    bool              hasVideo;
    bool              hasAudio;
    int               currentSpeed;
    int               previousSpeed;
    int frameCounter;
    pthread_mutex_t   m_lock;
};