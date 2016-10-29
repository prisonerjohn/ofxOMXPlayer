#pragma once
#include "ofMain.h"
#include "ofxOMXPlayerSettings.h"

#include "LIBAV_INCLUDES.h"


#include "OMXReader.h"
#include "OMXClock.h"
#include "StreamInfo.h"
#include "OMXThread.h"
#include "BaseVideoDecoder.h"

#define MAX_DATA_SIZE 10 * 1024 * 1024


class BaseVideoPlayer: public ofThread
{
public:
    BaseVideoPlayer();
    BaseVideoDecoder* decoder;
    std::deque<OMXPacket *> packets;
    
    bool isOpen;
    StreamInfo omxStreamInfo;
    double currentPTS;
    
    
    float fps;
    bool doAbort;
    bool doFlush;
    int speed;
    unsigned int cachedSize;
    
    
    void setSpeed(int speed);
    int getSpeed();
    
    void applyFilter(OMX_IMAGEFILTERTYPE filter);
    bool decode(OMXPacket *pkt);
    void threadedFunction();
    void flush();
    
    bool addPacket(OMXPacket*);
    
    double getCurrentPTS();
    double getFPS();
    
    unsigned int getCached();
    
    void submitEOS();
    bool EOS();
    /*
    void lock();
    void unlock();
    void lockDecoder();
    void unlockDecoder();
    bool isDecoderLocked;
    */
    uint32_t validHistoryPTS;
    
    virtual bool openDecoder() =0;
    virtual void close() = 0;
    
    ofxOMXPlayerSettings settings;
    Component* clockComponent;
    OMXClock* omxClock;
    OMXReader* omxReader;
    void adjustFPS();
    
};