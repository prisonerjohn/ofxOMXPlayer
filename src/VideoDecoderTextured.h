#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"
#include "BaseVideoDecoder.h"

class VideoDecoderTextured : public BaseVideoDecoder, public ComponentListener
{
public:
    VideoDecoderTextured();
    ~VideoDecoderTextured(){};
    
    bool open(StreamInfo, OMXClock* omxClock_, ofxOMXPlayerSettings&, EGLImageKHR);

    OMX_ERRORTYPE onFillBuffer(Component*, OMX_BUFFERHEADERTYPE*);
    int frameCounter;
};
