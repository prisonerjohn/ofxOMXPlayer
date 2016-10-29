#include "VideoDecoderDirect.h"



VideoDecoderDirect::VideoDecoderDirect()
{

	doHDMISync   = false;
}



bool VideoDecoderDirect::open(StreamInfo& streamInfo_, OMXClock* omxClock_, ofxOMXPlayerSettings& settings_)
{
	OMX_ERRORTYPE error   = OMX_ErrorNone;
    streamInfo = streamInfo_;
	videoWidth  = streamInfo.width;
	videoHeight = streamInfo.height;
    
    settings = settings_;
    omxClock = omxClock_;
    clockComponent = omxClock->getComponent();
	doHDMISync = settings.directDisplayOptions.doHDMISync;
    

	if(!videoWidth || !videoHeight)
	{
		return false;
	}

	if(streamInfo.extrasize > 0 && streamInfo.extradata != NULL)
	{
		extraSize = streamInfo.extrasize;
		extraData = (uint8_t *)malloc(extraSize);
		memcpy(extraData, streamInfo.extradata, streamInfo.extrasize);
	}

	processCodec(streamInfo);

	if(settings.enableFilters)
	{
		doFilters = true;
	}
	else
	{
		doFilters = false;
	}
    OMX_Maps::getInstance().filtersEnabled = doFilters;

	std::string componentName = "OMX.broadcom.video_decode";
    decoderComponent = new Component();
	if(!decoderComponent->init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

	componentName = "OMX.broadcom.video_render";
    renderComponent = new Component();

	if(!renderComponent->init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

	componentName = "OMX.broadcom.video_scheduler";
    schedulerComponent = new Component();

	if(!schedulerComponent->init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

    if(doFilters)
    {

        componentName = "OMX.broadcom.image_fx";
        imageFXComponent = new Component();

        if(!imageFXComponent->init(componentName, OMX_IndexParamImageInit))
        {
            return false;
        }
        decoderTunnel = new Tunnel();
        decoderTunnel->init(decoderComponent, 
                           decoderComponent->outputPort, 
                           imageFXComponent, 
                           imageFXComponent->inputPort);
        imageFXTunnel = new Tunnel();

        imageFXTunnel->init(imageFXComponent, 
                           imageFXComponent->outputPort, 
                           schedulerComponent, 
                           schedulerComponent->inputPort);
    }
    else
    {
        decoderTunnel = new Tunnel();

        decoderTunnel->init(decoderComponent, 
                           decoderComponent->outputPort, 
                           schedulerComponent, 
                           schedulerComponent->inputPort);
    }
    schedulerTunnel = new Tunnel();

    schedulerTunnel->init(schedulerComponent,
                         schedulerComponent->outputPort,
                         renderComponent,
                         renderComponent->inputPort);
    
    clockTunnel = new Tunnel();

    clockTunnel->init(clockComponent,
                     clockComponent->inputPort + 1,
                     schedulerComponent,
                     schedulerComponent->outputPort + 1);
    
	error = clockTunnel->Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	error = decoderComponent->setState(OMX_StateIdle);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
	OMX_INIT_STRUCTURE(formatType);
	formatType.nPortIndex = decoderComponent->inputPort;
	formatType.eCompressionFormat = omxCodingType;

	if (streamInfo.fpsscale > 0 && streamInfo.fpsrate > 0)
	{
		formatType.xFramerate = (long long)(1<<16)*streamInfo.fpsrate / streamInfo.fpsscale;
	}
	else
	{
		formatType.xFramerate = 25 * (1<<16);
	}
    
	error = decoderComponent->setParameter(OMX_IndexParamVideoPortFormat, &formatType);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_PARAM_PORTDEFINITIONTYPE portParam;
	OMX_INIT_STRUCTURE(portParam);
	portParam.nPortIndex = decoderComponent->inputPort;

	error = decoderComponent->getParameter(OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	portParam.nPortIndex = decoderComponent->inputPort;

	portParam.format.video.nFrameWidth  = videoWidth;
	portParam.format.video.nFrameHeight = videoHeight;

	error = decoderComponent->setParameter(OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
	OMX_INIT_STRUCTURE(concanParam);
	concanParam.bStartWithValidFrame = OMX_FALSE;

	error = decoderComponent->setParameter(OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;


	if (doFilters)
	{
		OMX_PARAM_U32TYPE extra_buffers;
		OMX_INIT_STRUCTURE(extra_buffers);
		extra_buffers.nU32 = 5;

		error = decoderComponent->setParameter(OMX_IndexParamBrcmExtraBuffers, &extra_buffers);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

	// broadcom omx entension:
	// When enabled, the timestamp fifo mode will change the way incoming timestamps are associated with output images.
	// In this mode the incoming timestamps get used without re-ordering on output images.
	
    // recent firmware will actually automatically choose the timestamp stream with the least variance, so always enable

    OMX_CONFIG_BOOLEANTYPE timeStampMode;
    OMX_INIT_STRUCTURE(timeStampMode);
    timeStampMode.bEnabled = OMX_TRUE;
    error = decoderComponent->setParameter((OMX_INDEXTYPE)OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;


	if(NaluFormatStartCodes(streamInfo.codec, extraData, extraSize))
	{
		OMX_NALSTREAMFORMATTYPE nalStreamFormat;
		OMX_INIT_STRUCTURE(nalStreamFormat);
		nalStreamFormat.nPortIndex = decoderComponent->inputPort;
		nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;

		error = decoderComponent->setParameter((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

	if(doHDMISync)
	{
		OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
		OMX_INIT_STRUCTURE(latencyTarget);
		latencyTarget.nPortIndex = renderComponent->inputPort;
		latencyTarget.bEnabled = OMX_TRUE;
		latencyTarget.nFilter = 2;
		latencyTarget.nTarget = 4000;
		latencyTarget.nShift = 3;
		latencyTarget.nSpeedFactor = -135;
		latencyTarget.nInterFactor = 500;
		latencyTarget.nAdjCap = 20;

		error = renderComponent->setConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

   // Alloc buffers for the omx input port.
	error = decoderComponent->allocInputBuffers(); 
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

    
	error = decoderTunnel->Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	
	error = decoderComponent->setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;


	if(doFilters)
	{ 
		error = imageFXTunnel->Establish(false);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
        
		error = imageFXComponent->setState(OMX_StateExecuting);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
        
        filterManager.setup(imageFXComponent);
        filterManager.setFilter(settings.filter);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;

	}

	error = schedulerTunnel->Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	
	error = schedulerComponent->setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
	
	error = renderComponent->setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
   
    
	if(!sendDecoderConfig())
	{
		return false;
	}

	isOpen           = true;
	doSetStartTime      = true;
	
	display.setup(renderComponent, streamInfo, settings);

	isFirstFrame   = true;
    
	// start from assuming all recent frames had valid pts
	validHistoryPTS = ~0;
	return true;
}







