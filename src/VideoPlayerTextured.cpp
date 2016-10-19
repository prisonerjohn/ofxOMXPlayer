#include "VideoPlayerTextured.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#include "XMemUtils.h"


VideoPlayerTextured::VideoPlayerTextured()
{
	textureDecoder = NULL;
}

void VideoPlayerTextured::resetFrameCounter()
{
    lockDecoder();
    textureDecoder->frameCounter = 0;
    ofLogVerbose(__func__)  << "";
    unlockDecoder();
}
VideoPlayerTextured::~VideoPlayerTextured()
{
    close();
}

bool VideoPlayerTextured::open(StreamInfo hints, OMXClock* omxClock_, OMXReader* omxReader_, ofxOMXPlayerSettings& settings_, EGLImageKHR eglImage_)
{

	eglImage = eglImage_;
    settings = settings_;
    

	if(ThreadHandle())
	{
		close();
	}


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

	Create();


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
	doAbort  = true;
	doFlush   = true;
	
	flush();
	
	if(ThreadHandle())
	{
		lock();
            pthread_cond_broadcast(&m_packet_cond);
		unlock();

		StopThread("VideoPlayerTextured");
	}

	if (textureDecoder)
	{
		delete textureDecoder;
		textureDecoder = NULL;
	};

	isOpen      = false;
	currentPTS  = DVD_NOPTS_VALUE;
	speed       = DVD_PLAYSPEED_NORMAL;

}
