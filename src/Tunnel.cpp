#include "Tunnel.h"

#pragma mark Tunnel


//#if OMX_LOG_LEVEL >= OMX_LOG_LEVEL_ERROR_ONLY
#define TUNNEL_LOG(x)  ofLogVerbose(__func__) << ofToString(x);

//#else
//#undef DEBUG_TUNNELS
//#define TUNNEL_LOG(x) 
//#endif


//#define DEBUG_TUNNELS
#define T_LOCK lock(__func__)
#define T_UNLOCK unlock(__func__)

Tunnel::Tunnel()
{
    sourceComponentName = "UNDEFINED";
    destinationComponentName = "UNDEFINED";
    sourceComponent       = NULL;
    destinationComponent       = NULL;
    sourcePort            = 0;
    destinationPort            = 0;
    havePortSettingsChanged = false;
    isEstablished = false;
    isLocked = false;

    pthread_mutex_init(&m_lock, NULL);
}

Tunnel::~Tunnel()
{
    if(isEstablished)
    {
        Deestablish();
    }
    
    pthread_mutex_destroy(&m_lock);
}

void Tunnel::lock(string caller)
{
    //ofLogVerbose(__func__) << "sourceComponentName: " << sourceComponentName << "TO " << " destinationComponentName: " << destinationComponentName << " caller: " << caller << " isLocked: " << isLocked;
    
    if(isLocked)
    {
        ofLogError(__func__) << "source: " << sourceComponentName << " TO " << " dest: " << destinationComponentName << " HAS DEADLOCK FROM " << caller;
    }
    
    
    pthread_mutex_lock(&m_lock);
    isLocked = true;

}

void Tunnel::unlock(string caller)
{
   // ofLogVerbose(__func__) << "sourceComponentName: " << sourceComponentName << "TO " << " destinationComponentName: " << destinationComponentName << " caller: " << caller << " isLocked: " << isLocked;
    pthread_mutex_unlock(&m_lock);
    isLocked = false;

}

void Tunnel::init(Component *src_component, unsigned int src_port, Component *destination, unsigned int dst_port)
{

    sourceComponent  = src_component;
    sourcePort    = src_port;
    destinationComponent  = destination;
    destinationPort    = dst_port;
    
    sourceComponentName = sourceComponent->name;
    destinationComponentName = destinationComponent->name;
}

OMX_ERRORTYPE Tunnel::flush()
{
    if(!sourceComponent || !destinationComponent || !isEstablished)
    {
        return OMX_ErrorUndefined;
    }
    
    T_LOCK;
    
    sourceComponent->flushAll();
    destinationComponent->flushAll();
    
    T_UNLOCK;
    return OMX_ErrorNone;
}


OMX_ERRORTYPE Tunnel::Deestablish(bool doTunnelSrcToNULL, bool doTunnelDestToNULL)
{/*
    if(OMX_Maps::getInstance().filtersEnabled)
    {
        ofLogVerbose(__func__) << "BAILING EARLY";
        isEstablished = false;
        return OMX_ErrorNone;

    }*/
    ofLogVerbose(__func__) << sourceComponentName << " TO " << destinationComponentName;
    //TUNNEL_LOG(ofToString(sourceComponent->name + " : " + destinationComponent->name));
    if (!isEstablished)
    {
        ofLogError(__func__) << "RETURING EARLY as isEstablished: " << isEstablished;
        return OMX_ErrorNone;
    }
    if(!sourceComponent)
    {
        ofLogError(__func__) << "NO SOURCE EXISTS: " << sourceComponentName;
        
        return OMX_ErrorUndefined;
    }
    if(!destinationComponent)
    {
        ofLogError(__func__) << "NO DEST EXISTS: " << destinationComponentName;
        
        return OMX_ErrorUndefined;
    }

    
    T_LOCK;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    if(havePortSettingsChanged)
    {
        error = sourceComponent->waitForEvent(OMX_EventPortSettingsChanged);
        OMX_TRACE(error);
    }

    if(doTunnelSrcToNULL)
    {
        bool sourceToNull = sourceComponent->tunnelToNull(sourcePort);
        ofLogVerbose(__func__) << sourceComponentName << " sourceToNull: " << sourceToNull;
       
    }
    if(doTunnelDestToNULL)
    {
        bool destToNull = destinationComponent->tunnelToNull(destinationPort);
        ofLogVerbose(__func__) << destinationComponentName << " destToNull: " << destToNull;

    }
    
    T_UNLOCK;
    isEstablished = false;
    return error;
}

OMX_ERRORTYPE Tunnel::Establish(bool portSettingsChanged)
{
    T_LOCK;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_PARAM_U32TYPE param;
    OMX_INIT_STRUCTURE(param);
    if(!sourceComponent || !destinationComponent || !sourceComponent->handle || !destinationComponent->handle)
    {
        T_UNLOCK;
        return OMX_ErrorUndefined;
    }
    
    error = sourceComponent->setState(OMX_StateIdle);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        T_UNLOCK;
        return error;
    }
#ifdef DEBUG_TUNNELS
    ofLogVerbose(__func__) << sourceComponent->name << " TUNNELING TO " << destinationComponent->name;
    ofLogVerbose(__func__) << "portSettingsChanged: " << portSettingsChanged;
#endif
    if(portSettingsChanged)
    {
        error = sourceComponent->waitForEvent(OMX_EventPortSettingsChanged);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone)
        {
            T_UNLOCK;
            return error;
        }
    }
    
    error = sourceComponent->disablePort(sourcePort);
    OMX_TRACE(error);
    
    
    error = destinationComponent->disablePort(destinationPort);
    OMX_TRACE(error);
    
    error = OMX_SetupTunnel(sourceComponent->handle, sourcePort, destinationComponent->handle, destinationPort);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        T_UNLOCK;
        return error;
    }
    else
    {
        isEstablished =true;
    }
    
    error = sourceComponent->enablePort(sourcePort);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        T_UNLOCK;
        return error;
    }
    
    error = destinationComponent->enablePort(destinationPort);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        T_UNLOCK;
        return error;
    }
    
    if(destinationComponent->getState() == OMX_StateLoaded)
    {
        //important to wait for audio
        error = destinationComponent->waitForCommand(OMX_CommandPortEnable, destinationPort);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone)
        {
            T_UNLOCK;
            return error;
        }
        error = destinationComponent->setState(OMX_StateIdle);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone)
        {
            T_UNLOCK;
            return error;
        }
    }
    
    error = sourceComponent->waitForCommand(OMX_CommandPortEnable, sourcePort);
    if(error != OMX_ErrorNone)
    {
        T_UNLOCK;
        return error;
    }
    
    havePortSettingsChanged = portSettingsChanged;
    
    T_UNLOCK;
    
    
    return error;
}
