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



#include "VideoPlayerDirect.h"


VideoPlayerDirect::VideoPlayerDirect()
{
	directDecoder = NULL;
}


bool VideoPlayerDirect::open(StreamInfo hints, OMXClock* omxClock_, OMXReader* omxReader_, ofxOMXPlayerSettings& settings_)
{
    
    
    settings = settings_;

	omxStreamInfo   = hints;
	fps             = 25.0f;
	currentPTS      = DVD_NOPTS_VALUE;
	doFlush         = false;
	cachedSize      = 0;
	speed           = DVD_PLAYSPEED_NORMAL;
    omxClock = omxClock_;
    clockComponent = omxClock->getComponent();
    omxReader = omxReader_;
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


bool VideoPlayerDirect::openDecoder()
{


	directDecoder = new VideoDecoderDirect();
	
	decoder = (BaseVideoDecoder*)directDecoder;
	if(!directDecoder->open(omxStreamInfo, omxClock, settings))
	{
        directDecoder->close();
        delete directDecoder;
        directDecoder = NULL;
        decoder = NULL;
		return false;
	}
	
	stringstream info;
	info << "Video width: "	<<	omxStreamInfo.width         << "\n";
	info << "Video height: "	<<	omxStreamInfo.height    << "\n";
	info << "Video profile: "	<<	omxStreamInfo.profile   << "\n";
	info << "Video fps: "		<<	fps                     << "\n";
	ofLogVerbose(__func__) << "\n" << info.str();

	return true;
}

void VideoPlayerDirect::close()
{
	doFlush = true;

	//flush();

	if(isThreadRunning())
	{
        stopThread();
	}
	
	if (directDecoder)
	{
        directDecoder->close();
		delete directDecoder;
		directDecoder = NULL;
	}
    decoder     = NULL;
    isOpen      = false;
	currentPTS  = DVD_NOPTS_VALUE;
	speed       = DVD_PLAYSPEED_NORMAL;
}

