//
//  FilterManager.h
//
//  Created by jason van cleave on 12/2/15.
//



class FilterManager
{
public:
    
    Component* imageFXComponent;
    FilterManager()
    {
        imageFXComponent = NULL;  
    }
    ~FilterManager()
    {
        imageFXComponent = NULL;  
    }
    
    void setup(Component* imageFXComponent_)
    {
        imageFXComponent = imageFXComponent_;
 
        
    }
    void setFilter(OMX_IMAGEFILTERTYPE imageFilter_)
    {

        ofLogVerbose(__func__) << "imageFilter_: " << GetImageFilterString(imageFilter_);
       /*
        OMX_CONFIG_IMAGEFILTERTYPE imagefilterConfig;
        OMX_INIT_STRUCTURE(imagefilterConfig);
        imagefilterConfig.nPortIndex = imageFXComponent->outputPort;
        
        imagefilterConfig.eImageFilter = imageFilter_;
        
        OMX_ERRORTYPE error = imageFXComponent->setConfig(OMX_IndexConfigCommonImageFilter, &imagefilterConfig);
        OMX_TRACE(error);
        
        */
        OMX_CONFIG_IMAGEFILTERPARAMSTYPE config;
        OMX_INIT_STRUCTURE(config);
        config.nPortIndex = imageFXComponent->outputPort;
        config.nNumParams = 1;
        config.nParams[0] = 3;
        config.eImageFilter = imageFilter_;
        OMX_ERRORTYPE error = imageFXComponent->setConfig(OMX_IndexConfigCommonImageFilterParameters, &config);
        OMX_TRACE(error);

        
        
    }
    
};