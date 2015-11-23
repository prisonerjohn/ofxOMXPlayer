/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */



#include "VideoPlayerNonTextured.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#include "XMemUtils.h"


VideoPlayerNonTextured::VideoPlayerNonTextured()
{
	doHDMISync = false;
	nonTextureDecoder = NULL;


}
VideoPlayerNonTextured::~VideoPlayerNonTextured()
{
	//ofLogVerbose(__func__) << "START";

	close();

	pthread_cond_destroy(&m_packet_cond);
	pthread_mutex_destroy(&m_lock);
	pthread_mutex_destroy(&m_lock_decoder);
	//ofLogVerbose(__func__) << "END";
}

bool VideoPlayerNonTextured::open(OMXStreamInfo& hints, OMXClock *av_clock, bool deinterlace, bool hdmi_clock_sync)
{
	//ofLogVerbose(__func__) << "VideoPlayerNonTextured Open";

	if (!av_clock)
	{
		return false;
	}

	if(ThreadHandle())
	{
		close();
	}


	omxStreamInfo       = hints;
	omxClock    = av_clock;
	fps         = 25.0f;
	frameTime   = 0;
	doDeinterlace = deinterlace;
	currentPTS = DVD_NOPTS_VALUE;
	doAbort      = false;
	doFlush       = false;
	cachedSize = 0;
	doHDMISync = hdmi_clock_sync;
	speed       = DVD_PLAYSPEED_NORMAL;
	

	timeStampAdjustment = omxClock->getAbsoluteClock();

	if(!openDecoder())
	{
		close();
		return false;
	}

	Create();

	isOpen        = true;

	return true;
}


bool VideoPlayerNonTextured::openDecoder()
{

	if (omxStreamInfo.fpsrate && omxStreamInfo.fpsscale)
	{
		fps = DVD_TIME_BASE / OMXReader::normalizeFrameduration((double)DVD_TIME_BASE * omxStreamInfo.fpsscale / omxStreamInfo.fpsrate);
	}
	else
	{
		fps = 25;
	}

	if( fps > 100 || fps < 5 )
	{
		ofLog(OF_LOG_VERBOSE, "Invalid framerate %d, using forced 25fps and just trust timestamps\n", (int)fps);
		fps = 25;
	}

	frameTime = (double)DVD_TIME_BASE / fps;

	nonTextureDecoder = new VideoDecoderNonTextured();
	
	nonTextureDecoder->setDisplayRect(displayRect);

	decoder = (VideoDecoderBase*)nonTextureDecoder;
	if(!nonTextureDecoder->open(omxStreamInfo, omxClock, doDeinterlace, doHDMISync))
	{

		closeDecoder();
		return false;
	}
	
	stringstream info;
	info << "Video width: "	<<	omxStreamInfo.width					<< "\n";
	info << "Video height: "	<<	omxStreamInfo.height					<< "\n";
	info << "Video profile: "	<<	omxStreamInfo.profile					<< "\n";
	info << "Video fps: "		<<	fps							<< "\n";
	//ofLogVerbose(__func__) << "\n" << info;

	/*ofLog(OF_LOG_VERBOSE, "Video codec %s width %d height %d profile %d fps %f\n",
		decoder->GetDecoderName().c_str() , omxStreamInfo.width, omxStreamInfo.height, omxStreamInfo.profile, fps);*/




	return true;
}

bool VideoPlayerNonTextured::close()
{
	//ofLogVerbose(__func__) << " START, isExiting:" << isExiting;
	doAbort  = true;
	doFlush   = true;

	flush();

	if(ThreadHandle())
	{
		lock();
		//ofLogVerbose(__func__) << "WE ARE STILL THREADED";
		pthread_cond_broadcast(&m_packet_cond);
		unlock();

		StopThread("VideoPlayerNonTextured");
	}
	
	if (nonTextureDecoder && !isExiting)
	{
		//ofLogVerbose(__func__) << "PRE DELETE nonTextureDecoder";
		delete nonTextureDecoder;
		nonTextureDecoder = NULL;
		//ofLogVerbose(__func__) << "POST DELETE nonTextureDecoder";
	}

	isOpen          = false;
	currentPTS   = DVD_NOPTS_VALUE;
	speed         = DVD_PLAYSPEED_NORMAL;

	//ofLogVerbose(__func__) << " END";
	return true;
}

bool VideoPlayerNonTextured::validateDisplayRect(ofRectangle& rectangle)
{
	//ofLogVerbose(__func__) << "displayRect: " << displayRect;
	//ofLogVerbose(__func__) << "rectangle: " << rectangle;
	if (displayRect == rectangle) 
	{
		return false;
	}
	displayRect = rectangle;
	return true;
}
void VideoPlayerNonTextured::setDisplayRect(ofRectangle& rectangle)
{
	if (ThreadHandle()) 
	{
		lock();
			if(validateDisplayRect(rectangle))
			{
				lockDecoder();
					nonTextureDecoder->setDisplayRect(displayRect);
				unlockDecoder();
			}
		unlock();
	}else 
	{
		validateDisplayRect(rectangle);
	}
}