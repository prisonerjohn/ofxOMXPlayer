
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
	pthread_cond_init(&m_packet_cond, NULL);
	pthread_mutex_init(&m_lock, NULL);
	pthread_mutex_init(&m_lock_decoder, NULL);
	validHistoryPTS = 0;
    doApplyFilter = false;
}

BaseVideoPlayer::~BaseVideoPlayer()
{
    
    pthread_cond_destroy(&m_packet_cond);
    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_lock_decoder);
    omxClock = NULL;
    clockComponent = NULL;
    decoder       = NULL;
    omxReader = NULL;

}
void BaseVideoPlayer::adjustFPS()
{
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
	return cachedSize;
}


void BaseVideoPlayer::setSpeed(int speed_)
{
	speed = speed_;
}

int BaseVideoPlayer::getSpeed()
{
	return speed;
}

void BaseVideoPlayer::applyFilter(OMX_IMAGEFILTERTYPE filter_)
{
    
    
    lock();
    doApplyFilter = true;
    filter = filter_;
    unlock();
}

void BaseVideoPlayer::lock()
{
	pthread_mutex_lock(&m_lock);
}

void BaseVideoPlayer::unlock()
{
	pthread_mutex_unlock(&m_lock);
}

void BaseVideoPlayer::lockDecoder()
{
	pthread_mutex_lock(&m_lock_decoder);
}

void BaseVideoPlayer::unlockDecoder()
{
	pthread_mutex_unlock(&m_lock_decoder);
}


bool BaseVideoPlayer::decode(OMXPacket *omxPacket)
{

    
    if(!omxPacket)
    {
        return false;
    }
    if(omxPacket->pts != DVD_NOPTS_VALUE)
    {
        //ofLogVerbose(__func__) << "SETTING currentPTS";
        currentPTS = omxPacket->pts;
    }
    return decoder->decode(omxPacket);
}




void BaseVideoPlayer::flush()
{


	lock();
	lockDecoder();
	doFlush = true;
	while (!packets.empty())
	{
		OMXPacket *pkt = packets.front();
		packets.pop_front();
		omxReader->freePacket(pkt, __func__);

	}

	currentPTS = DVD_NOPTS_VALUE;
	cachedSize = 0;

	if(decoder)
	{
		decoder->Reset();
	}

	unlockDecoder();
	unlock();
}



bool BaseVideoPlayer::addPacket(OMXPacket *pkt)
{
	bool ret = false;

	if(!pkt)
	{
		return ret;
	}

	if(doStop || doAbort)
	{
		return ret;
	}

    
    int targetSize = cachedSize + pkt->size;
	if(targetSize < MAX_DATA_SIZE)
	{
		lock();
			cachedSize += pkt->size;
			packets.push_back(pkt);
		unlock();
		ret = true;
		pthread_cond_broadcast(&m_packet_cond);
    }else
    {
        //ofLogError(__func__) << "targetSize: " << targetSize << " MAX_DATA_SIZE: " << MAX_DATA_SIZE << " packets.size: " << packets.size() << " CACHE FULL";
    }

	return ret;
}




void BaseVideoPlayer::process()
{
	OMXPacket *omxPacket = NULL;

	//m_pts was previously set to 0 - might need later...
	//currentPTS = 0;
	
	while(!doStop && !doAbort)
	{
		lock();
		if(packets.empty())
		{
			pthread_cond_wait(&m_packet_cond, &m_lock);
		}
		unlock();

		if(doAbort)
		{
			break;
		}

		lock();
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
    
		unlock();

		lockDecoder();
        
        if(doApplyFilter)
        {
            decoder->filterManager.setFilter(filter);
            doApplyFilter = false;
        }
        
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
       
		unlockDecoder();


	}
	
	if(omxPacket)
	{
		omxReader->freePacket(omxPacket, __func__);
	}
}

bool BaseVideoPlayer::closeDecoder()
{
	if(decoder)
	{
		delete decoder;
	}
	decoder   = NULL;
	return true;
}

void BaseVideoPlayer::submitEOS()
{
	if(decoder)
	{
		decoder->submitEOS();
	}
}

bool BaseVideoPlayer::EOS()
{
	bool atEndofStream = false;

	if (decoder)
	{
		if (packets.empty() && decoder->EOS())
		{

			atEndofStream = true;
		}
	}
	return atEndofStream;
}

