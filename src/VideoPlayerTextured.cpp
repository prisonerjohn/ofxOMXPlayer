#include "VideoPlayerTextured.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#include "XMemUtils.h"


VideoPlayerTextured::VideoPlayerTextured()
{
	textureDecoder = NULL;
}


bool VideoPlayerTextured::open(StreamInfo hints, OMXClock* omxClock_, OMXReader* omxReader_, ofxOMXPlayerSettings& settings_, EGLImageKHR eglImage_)
{

	eglImage = eglImage_;
    settings = settings_;
    
	omxStreamInfo       = hints;
    omxClock = omxClock_;
	clockComponent    = omxClock->getComponent();
    omxReader = omxReader_;
    
	fps         = 25.0f;
	currentPTS = DVD_NOPTS_VALUE;
	doAbort      = false;
	doFlush       = false;
	cachedSize = 0;
	speed       = DVD_PLAYSPEED_NORMAL;

    adjustFPS();
	if(!openDecoder())
	{
		close();
		return false;
	}

	startThread();


	isOpen        = true;

	return true;
}


bool VideoPlayerTextured::openDecoder()
{



	if (!textureDecoder)
	{
		textureDecoder = new VideoDecoderTextured();

	}

	decoder = (BaseVideoDecoder*)textureDecoder;

	if(!textureDecoder->open(omxStreamInfo, omxClock, settings, eglImage))
	{
        delete textureDecoder;
        textureDecoder = NULL;
        decoder = NULL;
		return false;
	}

	stringstream info;
	info << "Video width: "     <<	omxStreamInfo.width     << "\n";
	info << "Video height: "    <<	omxStreamInfo.height    << "\n";
	info << "Video profile: "   <<	omxStreamInfo.profile   << "\n";
	info << "Video fps: "		<<	fps                     << "\n";
	ofLogVerbose(__func__) << "\n" << info.str();


	return true;
}

void VideoPlayerTextured::close()
{
    if(isThreadRunning())
    {
        stopThread();
    }
    //flush();
    if (textureDecoder)
    {
        textureDecoder->close();
        delete textureDecoder;
        textureDecoder = NULL;
    };
    decoder = NULL;


   
    
	isOpen      = false;
	currentPTS  = DVD_NOPTS_VALUE;
	speed       = DVD_PLAYSPEED_NORMAL;

}
