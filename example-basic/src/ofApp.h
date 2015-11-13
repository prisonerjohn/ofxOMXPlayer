#pragma once

#include "ofMain.h"
#include "ofxOMXPlayer.h"

class ofApp : public ofBaseApp, public ofxOMXPlayerListener
{

public:

    void setup();
    void update();
    void draw();
        
    ofxOMXPlayer omxPlayer;
    void onVideoEnd(ofxOMXPlayerListenerEventData& e);
    void onVideoLoop(ofxOMXPlayerListenerEventData& e);
};

