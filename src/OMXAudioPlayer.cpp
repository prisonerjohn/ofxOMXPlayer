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

#include "OMXAudioPlayer.h"

#include <stdio.h>
#include <unistd.h>
#include "XMemUtils.h"

#define MAX_DATA_SIZE    1 * 1024 * 1024

OMXAudioPlayer::OMXAudioPlayer()
{
    isOpen          = false;
    omxClock = NULL;
    clockComponent        = NULL;
    omxReader       = NULL;
    decoder         = NULL;
    doFlush         = false;
    cachedSize      = 0;
    channelMap      = NULL;
    audioCodecOMX   = NULL;
    speed         = DVD_PLAYSPEED_NORMAL;
    hasErrors  = false;

    pthread_cond_init(&m_packet_cond, NULL);
    pthread_cond_init(&m_audio_cond, NULL);
    pthread_mutex_init(&m_lock, NULL);
    pthread_mutex_init(&m_lock_decoder, NULL);
}

OMXAudioPlayer::~OMXAudioPlayer()
{

    pthread_cond_destroy(&m_audio_cond);
    pthread_cond_destroy(&m_packet_cond);
    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_lock_decoder);
}

void OMXAudioPlayer::lock()
{
    pthread_mutex_lock(&m_lock);
}

void OMXAudioPlayer::unlock()
{
    pthread_mutex_unlock(&m_lock);
}

void OMXAudioPlayer::lockDecoder()
{
    pthread_mutex_lock(&m_lock_decoder);
}

void OMXAudioPlayer::unlockDecoder()
{
    pthread_mutex_unlock(&m_lock_decoder);
}

bool OMXAudioPlayer::open(StreamInfo& hints, 
                          OMXClock *omxClock_, 
                          OMXReader *omxReader_,
                          std::string device)
{
    if(ThreadHandle())
    {
        close();
    }


    omxStreamInfo   = hints;
    omxClock = omxClock_;
    clockComponent        = omxClock->getComponent();
    omxReader       = omxReader_;
    deviceName      = device;
    currentPTS      = DVD_NOPTS_VALUE;
    doAbort         = false;
    doFlush         = false;
    cachedSize      = 0;
    audioCodecOMX   = NULL;
    channelMap      = NULL;
    speed           = DVD_PLAYSPEED_NORMAL;

    bool success  = openCodec();
    if(!success)
    {
        ofLogError(__func__) << "openCodec: " << success;
        close();
        return success;
    }

    success = openDecoder();
    if(!success)
    {
        ofLogError(__func__) << "openDecoder: " << success;
        close();
        return success;
    }

    Create();

    isOpen        = true;

    return true;
}

bool OMXAudioPlayer::close()
{
    doAbort  = true;
    doFlush   = true;

    flush();

    if(ThreadHandle())
    {
        lock();
        pthread_cond_broadcast(&m_packet_cond);
        unlock();

        StopThread("OMXAudioPlayer");
    }

    closeDecoder();
    closeCodec();

    isOpen          = false;
    currentPTS   = DVD_NOPTS_VALUE;
    speed         = DVD_PLAYSPEED_NORMAL;


    return true;
}



bool OMXAudioPlayer::decode(OMXPacket *pkt)
{
    if(!pkt)
    {
        return false;
    }

    /* last decoder reinit went wrong */
    if(!decoder || !audioCodecOMX)
    {
        return true;
    }

    if(!omxReader->isActive(OMXSTREAM_AUDIO, pkt->stream_index))
    {
        return true;
    }

    int channels = pkt->hints.channels;

    /* 6 channel have to be mapped to 8 for PCM */
    if(channels == 6)
    {
        channels = 8;
    }

    unsigned int old_bitrate = omxStreamInfo.bitrate;
    unsigned int new_bitrate = pkt->hints.bitrate;

    /* only check bitrate changes on AV_CODEC_ID_DTS, AV_CODEC_ID_AC3, AV_CODEC_ID_EAC3 */
    if(omxStreamInfo.codec != AV_CODEC_ID_DTS && omxStreamInfo.codec != AV_CODEC_ID_AC3 && omxStreamInfo.codec != AV_CODEC_ID_EAC3)
    {
        new_bitrate = old_bitrate = 0;
    }

    /* audio codec changed. reinit device and decoder */
    if(omxStreamInfo.codec!= pkt->hints.codec ||
       omxStreamInfo.channels != channels ||
       omxStreamInfo.samplerate != pkt->hints.samplerate ||
       old_bitrate != new_bitrate ||
       omxStreamInfo.bitspersample != pkt->hints.bitspersample)
    {
       
        closeDecoder();
        closeCodec();

        omxStreamInfo = pkt->hints;

        
        ofLogError(__func__) << "AUDIO ERROR " << __LINE__ << " omxStreamInfo: " << omxStreamInfo.toString();
        hasErrors = true;
        return false;
    }
    
#if 1
    if(!((int)decoder->GetSpace() > pkt->size))
    {
       // omxClock->sleep(10);
    }

    if((int)decoder->GetSpace() > pkt->size)
    {
        if(pkt->pts != DVD_NOPTS_VALUE)
        {
            currentPTS = pkt->pts;
        }
        else if(pkt->dts != DVD_NOPTS_VALUE)
        {
            currentPTS = pkt->dts;
        }

        uint8_t *data_dec = pkt->data;
        int            data_len = pkt->size;

        while(data_len > 0)
        {
            int len = audioCodecOMX->decode((BYTE *)data_dec, data_len);
            if( (len < 0) || (len >  data_len) )
            {
                audioCodecOMX->Reset();
                break;
            }
            
            data_dec+= len;
            data_len -= len;
            
            uint8_t *decoded;
            int decoded_size = audioCodecOMX->GetData(&decoded);
            
            if(decoded_size <=0)
            {
                continue;
            }
            
            int ret = 0;
            
            ret = decoder->addPackets(decoded, decoded_size, pkt->dts, pkt->pts);
            
            if(ret != decoded_size)
            {
                ofLogError(__func__) << "ret : " << ret << " NOT EQUAL TO " << decoded_size;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
#endif
}

void OMXAudioPlayer::process()
{
    OMXPacket* omxPacket = NULL;

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
        else if(!omxPacket && !packets.empty())
        {
            omxPacket = packets.front();
            cachedSize -= omxPacket->size;
            packets.pop_front();
        }
        unlock();

        lockDecoder();
        if(doFlush && omxPacket)
        {
            omxReader->freePacket(omxPacket, __func__);
            omxPacket = NULL;
            doFlush = false;
        }
        else if(omxPacket && decode(omxPacket))
        {
            omxReader->freePacket(omxPacket, __func__);
            omxPacket = NULL;
        }
        unlockDecoder();
    }

    if(omxPacket)
    {
        omxReader->freePacket(omxPacket, __func__);
    }
}

void OMXAudioPlayer::flush()
{
    //ofLogVerbose(__func__) << "OMXAudioPlayer::Flush start";
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
        decoder->flush();
    }
    unlockDecoder();
    unlock();
    //ofLogVerbose(__func__) << "OMXAudioPlayer::Flush end";
}

bool OMXAudioPlayer::addPacket(OMXPacket *pkt)
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
       // ofLogVerbose(__func__) << "pkt->size: " << pkt->size << " cachedSize: " << cachedSize;
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

bool OMXAudioPlayer::openCodec()
{
    audioCodecOMX = new AudioCodecOMX();

    if(!audioCodecOMX->open(omxStreamInfo))
    {
        delete audioCodecOMX;
        audioCodecOMX = NULL;
        return false;
    }

    channelMap = audioCodecOMX->GetChannelMap();
    return true;
}

void OMXAudioPlayer::closeCodec()
{
    if(audioCodecOMX)
    {
        delete audioCodecOMX;
    }
    audioCodecOMX = NULL;
}


bool OMXAudioPlayer::openDecoder()
{

    bool bAudioRenderOpen = false;

    decoder = new OMXAudioDecoder();
    stringstream ss;
    ss << deviceName.substr(4); 
    string name = ss.str();
    bAudioRenderOpen = decoder->init(name, 
                                       channelMap,
                                       omxStreamInfo, 
                                       clockComponent, 
                                       doBoostOnDownmix);
    
    
    codecName = omxReader->getCodecName(OMXSTREAM_AUDIO);

    if(!bAudioRenderOpen)
    {
        ofLogError(__func__) << "bAudioRenderOpen: " << bAudioRenderOpen;
        delete decoder;
        decoder = NULL;
        return false;
    }
    else
    {
        stringstream info;
        info << "\n";
        info << "Audio codec : " << codecName;
        info << "\n";
        info << "Audio channels : " << 2;
        info << "\n";
        info << "Audio samplerate : " << omxStreamInfo.samplerate;
        info << "\n";
        info << "Audio bitspersample : " << omxStreamInfo.bitspersample;
        info << "\n";
        ofLogVerbose(__func__) << info.str();
    }
    return true;
}

bool OMXAudioPlayer::closeDecoder()
{
    if(decoder)
    {
        delete decoder;
    }
    decoder   = NULL;
    return true;
}

void OMXAudioPlayer::submitEOS()
{
    if(decoder)
    {
        decoder->submitEOS();
    }
}

bool OMXAudioPlayer::EOS()
{
    return packets.empty() && (!decoder || decoder->EOS());
}

void OMXAudioPlayer::WaitCompletion()
{
    ofLogVerbose(__func__) << "OMXAudioPlayer::WaitCompletion";
    if(!decoder)
    {
        return;
    }

    unsigned int nTimeOut = 2.0f * 1000;
    while(nTimeOut)
    {
        if(EOS())
        {
            ofLog(OF_LOG_VERBOSE, "%s::%s - got eos\n", "OMXAudioPlayer", __func__);
            break;
        }

        if(nTimeOut == 0)
        {
            ofLog(OF_LOG_ERROR, "%s::%s - wait for eos timed out\n", "OMXAudioPlayer", __func__);
            break;
        }
        //omxClock->sleep(50);
        nTimeOut -= 50;
    }
}

void OMXAudioPlayer::setCurrentVolume(long nVolume)
{
    //ofLogVerbose(__func__) << "nVolume: " << nVolume;
    if(decoder)
    {
        decoder->setCurrentVolume(nVolume);
    }
}

long OMXAudioPlayer::getCurrentVolume()
{
    if(decoder)
    {
        return decoder->getCurrentVolume();
    }
    else
    {
        return 0;
    }
}

void OMXAudioPlayer::setSpeed(int speed_)
{
    speed = speed_;
}

