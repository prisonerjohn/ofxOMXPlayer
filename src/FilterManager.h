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
        if(!imageFXComponent)
        {
            ofLogError(__func__) << "RIP";
            return;
        }
        string filterName = GetImageFilterString(imageFilter_);
        ofLogVerbose(__func__) << "filterName: " << filterName;

        OMX_CONFIG_IMAGEFILTERTYPE imagefilterConfig;
        OMX_INIT_STRUCTURE(imagefilterConfig);
        imagefilterConfig.nPortIndex = imageFXComponent->outputPort;
        imagefilterConfig.eImageFilter = imageFilter_;
        OMX_ERRORTYPE error = OMX_SetConfig(imageFXComponent->handle, OMX_IndexConfigCommonImageFilter, &imagefilterConfig);
        if(error == OMX_ErrorBadParameter)
        {
            ofLogError(__func__) << filterName << " FAILED";
        }
        OMX_TRACE(error);
    }
    
    /*void applyFilter()
    {
        OMX_ERRORTYPE error   = OMX_ErrorNone;
        
        error = imageFXComponent->setConfig(OMX_IndexConfigCommonImageFilter, &imagefilterConfig);
        OMX_TRACE(error);

    }*/
    
    
};