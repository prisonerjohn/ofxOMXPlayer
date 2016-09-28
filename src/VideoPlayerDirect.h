#pragma once


#include "BaseVideoPlayer.h"
#include "VideoDecoderDirect.h"

class VideoPlayerDirect : public BaseVideoPlayer
{
public:
    VideoPlayerDirect();
    ~VideoPlayerDirect();
            
    VideoDecoderDirect* directDecoder;
    
    bool open(StreamInfo& hints, Component* clockComponent_, ofxOMXPlayerSettings& settings_);
    bool openDecoder();
    void close();
    
    bool doUpdate;
};
