#include "VideoDecoderBase.h"



#include "OMXStreamInfo.h"

#include "XMemUtils.h"

#include <sys/time.h>
#include <inttypes.h>


VideoDecoderBase::VideoDecoderBase()
{

	isOpen           = false;
	doPause             = false;
	doSetStartTime      = true;
	extraData         = NULL;
	extraSize         = 0;
	isFirstFrame       = true;
	omxClock			= NULL;
	clockComponent			= NULL;
}

VideoDecoderBase::~VideoDecoderBase()
{
    SingleLock lock (m_critSection);
    OMX_ERRORTYPE error = OMX_ErrorNone;
    error = clockTunnel.Deestablish();
    OMX_TRACE(error);
    
    error = decoderTunnel.Deestablish();
    OMX_TRACE(error);
    
    error = schedulerTunnel.Deestablish();
    OMX_TRACE(error);
    
    bool didDeinit = false;
    
    didDeinit = schedulerComponent.Deinitialize(__func__); 
    if(!didDeinit) ofLogError(__func__) << "didDeinit failed on schedulerComponent";

    
    didDeinit = decoderComponent.Deinitialize(__func__); 
    if(!didDeinit) ofLogError(__func__) << "didDeinit failed on decoderComponent";

    
    didDeinit = renderComponent.Deinitialize(__func__); 
    if(!didDeinit) ofLogError(__func__) << "didDeinit failed on renderComponent";

    if(extraData)
    {
        free(extraData);
    }
    extraData = NULL;

    omxClock          = NULL;
    clockComponent = NULL;
    isOpen       = false;

}


bool VideoDecoderBase::NaluFormatStartCodes(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize)
{
	switch(codec)
	{
		case CODEC_ID_H264:
		{
			if (in_extrasize < 7 || in_extradata == NULL)
			{
				return true;
			}
			// valid avcC atom data always starts with the value 1 (version), otherwise annexb
			else if ( *in_extradata != 1 )
			{
				return true;
			}
		}
		default:
			break;
	}
	return false;
}



bool VideoDecoderBase::sendDecoderConfig()
{
	OMX_ERRORTYPE error   = OMX_ErrorNone;

	/* send decoder config */
	if(extraSize > 0 && extraData != NULL)
	{

		OMX_BUFFERHEADERTYPE *omxBuffer = decoderComponent.getInputBuffer();

		if(omxBuffer == NULL)
		{
			ofLogError(__func__) << "buffer error";
			return false;
		}

		omxBuffer->nOffset = 0;
		omxBuffer->nFilledLen = extraSize;
		if(omxBuffer->nFilledLen > omxBuffer->nAllocLen)
		{
			ofLogError(__func__) << "omxBuffer->nFilledLen > omxBuffer->nAllocLen";
			return false;
		}

		memset((unsigned char *)omxBuffer->pBuffer, 0x0, omxBuffer->nAllocLen);
		memcpy((unsigned char *)omxBuffer->pBuffer, extraData, omxBuffer->nFilledLen);
		omxBuffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;

		error = decoderComponent.EmptyThisBuffer(omxBuffer);
        OMX_TRACE(error);
		if (error != OMX_ErrorNone)
		{
			return false;
		}

	}

	return true;
}



void VideoDecoderBase::submitEOS()
{
	if(!isOpen)
	{
		return;
	}

	OMX_ERRORTYPE error = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *omxBuffer = decoderComponent.getInputBuffer();

	if(omxBuffer == NULL)
	{
		ofLogError(__func__) << "buffer NULL";
		return;
	}

	omxBuffer->nOffset     = 0;
	omxBuffer->nFilledLen  = 0;
	omxBuffer->nTimeStamp  = ToOMXTime(0LL);

	omxBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_TIME_UNKNOWN;

	error = decoderComponent.EmptyThisBuffer(omxBuffer);
    OMX_TRACE(error);

}

bool VideoDecoderBase::EOS()
{
	bool isEndOfStream = false;
	if(!isOpen)
	{
		isEndOfStream =  true;
	}
	else
	{
		if (decoderComponent.EOS())
		{

			isEndOfStream =  true;

		}
		//return renderComponent.EOS();
	}
	if (isEndOfStream)
	{
		ofLogVerbose("VideoDecoderBase::EOS") << "isEndOfStream: " << isEndOfStream;
	}
	return isEndOfStream;
}

bool VideoDecoderBase::pause()
{
	if(renderComponent.getHandle() == NULL)
	{
		return false;
	}

	if(doPause)
	{
		return true;
	}

	doPause = true;

	schedulerComponent.setState(OMX_StatePause);
	renderComponent.setState(OMX_StatePause);

	return true;
}

bool VideoDecoderBase::resume()
{
	if(renderComponent.getHandle() == NULL)
	{
		return false;
	}

	if(!doPause)
	{
		return true;
	}
	doPause = false;

	schedulerComponent.setState(OMX_StateExecuting);
	renderComponent.setState(OMX_StateExecuting);

	return true;
}

void VideoDecoderBase::Reset()
{
	//ofLogVerbose(__func__) << " START";

	decoderComponent.flushInput();
	decoderTunnel.flush();

	//ofLogVerbose(__func__) << " END";
}


void VideoDecoderBase::processCodec(OMXStreamInfo& hints)
{
	switch (hints.codec)
	{
		case CODEC_ID_H264:
		{
			switch(hints.profile)
			{
				case FF_PROFILE_H264_BASELINE:
					// (role name) video_decoder.avc
					// H.264 Baseline profile
					omxCodingType = OMX_VIDEO_CodingAVC;
					break;
				case FF_PROFILE_H264_MAIN:
					// (role name) video_decoder.avc
					// H.264 Main profile
					omxCodingType = OMX_VIDEO_CodingAVC;
					break;
				case FF_PROFILE_H264_HIGH:
					// (role name) video_decoder.avc
					// H.264 Main profile
					omxCodingType = OMX_VIDEO_CodingAVC;
					break;
				case FF_PROFILE_UNKNOWN:
					omxCodingType = OMX_VIDEO_CodingAVC;
					break;
				default:
					omxCodingType = OMX_VIDEO_CodingAVC;
					break;
			}
		}
		break;
		case CODEC_ID_MPEG4:
			// (role name) video_decoder.mpeg4
			// MPEG-4, DivX 4/5 and Xvid compatible
			omxCodingType = OMX_VIDEO_CodingMPEG4;
			break;
		case CODEC_ID_MPEG1VIDEO:
		case CODEC_ID_MPEG2VIDEO:
			// (role name) video_decoder.mpeg2
			// MPEG-2
			omxCodingType = OMX_VIDEO_CodingMPEG2;
			break;
		case CODEC_ID_H263:
			// (role name) video_decoder.mpeg4
			// MPEG-4, DivX 4/5 and Xvid compatible
			omxCodingType = OMX_VIDEO_CodingMPEG4;
			break;
		case CODEC_ID_VP6:
		case CODEC_ID_VP6F:
		case CODEC_ID_VP6A:
			// (role name) video_decoder.vp6
			// VP6
			omxCodingType = OMX_VIDEO_CodingVP6;
			break;
		case CODEC_ID_VP8:
			// (role name) video_decoder.vp8
			// VP8
			omxCodingType = OMX_VIDEO_CodingVP8;
			break;
		case CODEC_ID_THEORA:
			// (role name) video_decoder.theora
			// theora
			omxCodingType = OMX_VIDEO_CodingTheora;
			break;
		case CODEC_ID_MJPEG:
		case CODEC_ID_MJPEGB:
			// (role name) video_decoder.mjpg
			// mjpg
			omxCodingType = OMX_VIDEO_CodingMJPEG;
			break;
		case CODEC_ID_VC1:
		case CODEC_ID_WMV3:
			// (role name) video_decoder.vc1
			// VC-1, WMV9
			omxCodingType = OMX_VIDEO_CodingWMV;
			break;
		default:
            omxCodingType = OMX_VIDEO_CodingUnused;
			ofLog(OF_LOG_VERBOSE, "Video codec id unknown: %x\n", hints.codec);
			break;
	}
}