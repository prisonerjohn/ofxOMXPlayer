#include "ofMain.h"

#define AUDIO_ENABLED false

#include "ofApp.h"


int main()
{
    ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp( new ofApp());
}