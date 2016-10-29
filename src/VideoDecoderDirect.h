#pragma once

#include "ofMain.h"

#include "BaseVideoDecoder.h"
#include "OMXDisplay.h"


class VideoDecoderDirect : public BaseVideoDecoder
{
public:
  
    VideoDecoderDirect();
    bool open(StreamInfo&, OMXClock* omxClock_, ofxOMXPlayerSettings&);
      
    void updateFrameCount();
    void onUpdate(ofEventArgs& args);
    
    bool doDeinterlace;
    bool doHDMISync;    
    OMXDisplay display;
    OMXDisplay* getOMXDisplay()
    {
        return &display;
    }
    
};