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


class BaseVideoPlayer: public OMXThread
{
public:
    BaseVideoPlayer();
    virtual ~BaseVideoPlayer();
    BaseVideoDecoder* decoder;
    std::deque<OMXPacket *> packets;
    
    bool isOpen;
    StreamInfo omxStreamInfo;
    double currentPTS;
    
    pthread_cond_t m_packet_cond;
    //pthread_cond_t m_picture_cond;
    pthread_mutex_t m_lock;
    pthread_mutex_t m_lock_decoder; 
    
    float fps;
    bool doAbort;
    bool doFlush;
    int speed;
    unsigned int cachedSize;
    
    
    void setSpeed(int speed);
    int getSpeed();
    
    void applyFilter(OMX_IMAGEFILTERTYPE filter);
    bool decode(OMXPacket *pkt);
    void process();
    void flush();
    
    bool addPacket(OMXPacket*);
    

    bool closeDecoder();
    double getCurrentPTS();
    double getFPS();
    
    unsigned int getCached();
    
    void submitEOS();
    bool EOS();
    
    void lock();
    void unlock();
    void lockDecoder();
    void unlockDecoder();
    
    
    uint32_t validHistoryPTS;
        
    int getCurrentFrame();
    void resetFrameCounter();
    
    
    virtual bool openDecoder() =0;
    virtual void close() = 0;
    
    ofxOMXPlayerSettings settings;
    Component* clockComponent;
    OMXClock* omxClock;
    
    void adjustFPS();
};