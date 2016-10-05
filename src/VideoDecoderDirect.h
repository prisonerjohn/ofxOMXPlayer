#pragma once

#include "ofMain.h"

#include "BaseVideoDecoder.h"
#include "OMXDisplay.h"


class VideoDecoderDirect : public BaseVideoDecoder
{
public:
  
    VideoDecoderDirect();
    ~VideoDecoderDirect();
    bool open(StreamInfo&, Component*, ofxOMXPlayerSettings&);
      
    void updateFrameCount();
    void onUpdate(ofEventArgs& args);
    
    int getCurrentFrame();
    void resetFrameCounter();
    

    bool doDeinterlace;
    bool doHDMISync;    
    OMXDisplay display;
    OMXDisplay* getOMXDisplay()
    {
        return &display;
    }
    bool doUpdate;
    
    static OMX_ERRORTYPE onRenderFillBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onRenderEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onDecoderEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);


private:
    int frameCounter;
    int frameOffset;
    
    
};