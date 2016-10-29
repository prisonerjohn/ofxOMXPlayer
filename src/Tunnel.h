#pragma once

#include "Component.h"

class Tunnel
{
public:
    Tunnel();
    ~Tunnel();
    
    void init(Component*, unsigned int, Component*, unsigned int);
    OMX_ERRORTYPE flush();
    OMX_ERRORTYPE Deestablish(bool doTunnelToNULL = true, bool doTunnelDestToNULL=true);
    OMX_ERRORTYPE Establish(bool portSettingsChanged);
    string sourceComponentName;
    string destinationComponentName;
    void            lock(string caller="UNDEFINED");
    void            unlock(string caller="UNDEFINED");
private:
    bool isEstablished;
    pthread_mutex_t   m_lock;
    bool            havePortSettingsChanged;
    Component*      sourceComponent;
    Component*      destinationComponent;
    unsigned int    sourcePort;
    unsigned int    destinationPort;
    bool isLocked;
    

};