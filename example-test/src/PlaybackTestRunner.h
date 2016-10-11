#pragma once
#include "BaseTest.h"



class PlaybackTestRunner
{
public:
    
    BaseTest* test;
    int pauseStartTime;
    bool pauseComplete;
    bool doPause;
    bool doRestart;
    bool doStepTest;
    bool doScrubTest;
    int scrubCounter;
    int stepCounter;

    PlaybackTestRunner()
    {
        stopAll();
    }
    
    
    void stopAll()
    {
        test = NULL;
        doPause = false;
        doRestart = false;
        doStepTest = false;
        doScrubTest = false;
        stepCounter = 0;
        scrubCounter = 0;
        pauseStartTime = 0;
        pauseComplete = false;
    }
    
    void startPauseTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doPause = true;
        ofLog() << "STARTING PAUSE TEST";

    }
    void startRestartTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doRestart = true;
        ofLog() << "STARTING RESTART TEST";

    }
    
    void startStepTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doStepTest = true;
        ofLog() << "STARTING STEP TEST";

    }
    void startScrubTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doScrubTest = true;
        ofLog() << "STARTING SCRUB TEST";
    }

    void update()
    {
        if(test)
        {
            if(doPause)
            {
                if(!pauseStartTime)
                {
                    pauseStartTime = ofGetFrameNum();
                    test->omxPlayer->setPaused(true);
                }
                if(ofGetFrameNum()-pauseStartTime == 300)
                {
                    test->omxPlayer->setPaused(false);
                    doPause = false;
                    pauseStartTime = 0;
                    ofLogVerbose(__func__) << test->name << " PAUSE TEST COMPLETE";
                }
                
            }
            if(doRestart)
            {
                doRestart = false;
                test->omxPlayer->restartMovie();
            }
            if(doStepTest)
            {
                bool doStopStepTest = false;
                int currentFrame = test->omxPlayer->getCurrentFrame();
                if(ofGetFrameNum() % 30 == 0)
                {
                    ofLogVerbose() << "stepCounter: " << stepCounter << " currentFrame: " << currentFrame;

                    if(currentFrame+1 < test->omxPlayer->getTotalNumFrames())
                    {
                        test->omxPlayer->stepFrameForward();
                        stepCounter++;
                        
                    }else
                    {
                        doStopStepTest =true;
                    }
                    if(stepCounter == 10)
                    {
                        doStopStepTest =true;
                    }
                    
                    if(doStopStepTest)
                    {
                        doStepTest = false;
                        stepCounter = 0;
                        test->omxPlayer->restartMovie();
                    }
                }
                
            }
            if(doScrubTest)
            {
                int step = 10;
                if(ofGetFrameNum() % 30 == 0)
                {
                    if(scrubCounter < 10)
                    {
                        test->omxPlayer->scrubForward(step);
                        scrubCounter++;
                        ofLog() << "scrubCounter: " << scrubCounter;
                    }else
                    {
                        doScrubTest = false;
                        scrubCounter = 0;
                        ofLogVerbose(__func__) << test->name << " SCRUB TEST COMPLETE";
                        
                    }
                }
                
            }
            
        }
    
    }
    
};