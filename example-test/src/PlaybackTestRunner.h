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
        pauseStartTime = 0;
        pauseComplete = false;
    }
    
    void startPauseTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doPause = true;
    }
    void startRestartTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doRestart = true;
    }
    
    void startStepTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doStepTest = true;
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
                int currentFrame = test->omxPlayer->getCurrentFrame();
                if(currentFrame+1 < test->omxPlayer->getTotalNumFrames())
                {
                    test->omxPlayer->stepFrameForward();
                    ofSleepMillis(1000);
                }else
                {
                    doStepTest = false;
                    test->omxPlayer->restartMovie();
                }
            }
            
        }
    
    }
    
};