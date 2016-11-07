#pragma once


#include "BaseVideoPlayer.h"
#include "VideoDecoderDirect.h"

class VideoPlayerDirect : public BaseVideoPlayer
{
public:
    VideoPlayerDirect();
            
    VideoDecoderDirect* directDecoder;
    
    bool open(StreamInfo hints, OMXClock* omxClock_, OMXReader*, ofxOMXPlayerSettings& settings_);
    bool openDecoder();
    void close();
    
};
