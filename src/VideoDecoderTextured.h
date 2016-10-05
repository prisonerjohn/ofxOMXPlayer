#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"
#include "BaseVideoDecoder.h"

class VideoDecoderTextured : public BaseVideoDecoder
{
public:
    VideoDecoderTextured();
    ~VideoDecoderTextured(){};
    
    bool open(StreamInfo&, Component* clockComponent, ofxOMXPlayerSettings&, EGLImageKHR);
        
    int getCurrentFrame();
    void resetFrameCounter();
    static OMX_ERRORTYPE onDecoderEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onRenderFillBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);

private:
    int frameCounter;
    int frameOffset;
};
