
#include "BaseVideoPlayer.h"

unsigned count_bits(int32_t value)
{
	unsigned bits = 0;
	for(; value; ++bits)
	{
		value &= value - 1;
	}
	return bits;
}

BaseVideoPlayer::BaseVideoPlayer()
{
	isOpen          = false;
	decoder       = NULL;
	fps           = 25.0f;
	doFlush         = false;
	cachedSize   = 0;
	currentPTS	= DVD_NOPTS_VALUE;
	speed         = DVD_PLAYSPEED_NORMAL;
    omxReader = NULL;
    omxClock = NULL;
    clockComponent = NULL;
	decoder = NULL;
	validHistoryPTS = 0;
}

void BaseVideoPlayer::adjustFPS()
{
    lock();
    if (omxStreamInfo.fpsrate && omxStreamInfo.fpsscale)
    {
        fps = AV_TIME_BASE / OMXReader::normalizeFrameduration((double)AV_TIME_BASE * omxStreamInfo.fpsscale / omxStreamInfo.fpsrate);
    }
    else
    {
        fps = 25;
    }
    
    if( fps > 100 || fps < 5 )
    {
        //ofLogVerbose(__func__) << "Invalid framerate " << fps  << " using forced 25fps and just trust timestamps";
        fps = 25;
    }
    omxStreamInfo.fps = fps;
    unlock();
}

double BaseVideoPlayer::getCurrentPTS()
{
	return currentPTS;
}

double BaseVideoPlayer::getFPS()
{
	return fps;
}

unsigned int BaseVideoPlayer::getCached()
{
    unsigned int result = 0;
    //lock();
	result =  cachedSize;
    //unlock();
    return result;

}


void BaseVideoPlayer::setSpeed(int speed_)
{
   // lock();
	speed = speed_;
    //unlock();

}

int BaseVideoPlayer::getSpeed()
{
	return speed;
}

void BaseVideoPlayer::applyFilter(OMX_IMAGEFILTERTYPE filter)
{
    //lock();
    if(!settings.enableFilters)
    {
        ofLogError(__func__) << "FILTERS ARE DISABLED via ofxOMXPlayerSettings";
        return;
    }

    decoder->filterManager.setFilter(filter);
   // unlock();
}



bool BaseVideoPlayer::decode(OMXPacket *omxPacket)
{
    bool result = false;
    if(!omxPacket)
    {
        return result;
    }
    lock();

    if(omxPacket->pts != DVD_NOPTS_VALUE)
    {
        //ofLogVerbose(__func__) << "SETTING currentPTS";
        currentPTS = omxPacket->pts;
    }
    result = decoder->decode(omxPacket);
    unlock();
    return result;
}




void BaseVideoPlayer::flush()
{


	while (!packets.empty())
	{
		OMXPacket *pkt = packets.front();
		packets.pop_front();
		omxReader->freePacket(pkt, __func__);

	}

	currentPTS = DVD_NOPTS_VALUE;
	cachedSize = 0;

}



bool BaseVideoPlayer::addPacket(OMXPacket *pkt)
{
	bool ret = false;

	if(!pkt)
	{
		return ret;
	}


    lock();
    int targetSize = cachedSize + pkt->size;
	if(targetSize < MAX_DATA_SIZE)
	{
		
			cachedSize += pkt->size;
			packets.push_back(pkt);
		
		ret = true;
    }else
    {
        //ofLogError(__func__) << "targetSize: " << targetSize << " MAX_DATA_SIZE: " << MAX_DATA_SIZE << " packets.size: " << packets.size() << " CACHE FULL";
    }
    unlock();
	return ret;
}




void BaseVideoPlayer::threadedFunction()
{
	OMXPacket *omxPacket = NULL;

	//m_pts was previously set to 0 - might need later...
	//currentPTS = 0;
	
	while(isThreadRunning())
	{

		if(doAbort)
		{
			break;
		}

		if(doFlush && omxPacket)
		{
			omxReader->freePacket(omxPacket, __func__);
			omxPacket = NULL;
			doFlush = false;
		}
		else
		{
			if(!omxPacket && !packets.empty())
			{
				omxPacket = packets.front();
				cachedSize -= omxPacket->size;
                //ofLogNotice() << "omxPacket->pts: " << omxPacket->pts;
				packets.pop_front();
			}
		}

		//lockDecoder();
		if(doFlush && omxPacket)
		{
			omxReader->freePacket(omxPacket, __func__);
			omxPacket = NULL;
			doFlush = false;
		}
		else
		{
			if(omxPacket && decode(omxPacket))
			{
				
				omxReader->freePacket(omxPacket, __func__);
				omxPacket = NULL;
			}
		}
		//unlockDecoder();


	}
	
	if(omxPacket)
	{
		omxReader->freePacket(omxPacket, __func__);
	}
}

void BaseVideoPlayer::submitEOS()
{
    //lock();
	if(decoder)
	{
		decoder->submitEOS();
	}
    //unlock();
}

bool BaseVideoPlayer::EOS()
{
	bool atEndofStream = false;
    //lock();
	if (decoder)
	{
		if (packets.empty() && decoder->EOS())
		{

			atEndofStream = true;
		}
	}
   // unlock();
	return atEndofStream;
}

