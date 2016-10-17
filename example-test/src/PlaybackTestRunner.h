#pragma once
#include "BaseTest.h"



class PlaybackTestRunner
{
public:
    
    BaseTest* test;
    
    //pause test
    int pauseStartTime;
    bool doPause;
    bool doRestart;

    //step test
    bool doStepTest;
    int stepCounter;

    //scrub test
    bool doScrubTest;
    int scrubCounter;
    
    //volume test
    bool doVolumeTest;
    bool increaseTestComplete;
    bool decreaseTestComplete;
    
    bool doSeekTest;
    bool doSeekToFrameTest;
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
        doVolumeTest = false;
        increaseTestComplete = false;
        decreaseTestComplete = false;
        
        doSeekTest = false;
        doSeekToFrameTest = false;
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
    
    void startSeekTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doSeekTest = true;
        ofLog() << "STARTING SEEK TEST";
    }
    
    void startSeekToFrameTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doSeekToFrameTest = true;
        ofLog() << "STARTING SEEK TO FRAME TEST";
    }
    
    void startVolumeTest(BaseTest* test_)
    {
        stopAll();
        test = test_;
        doVolumeTest = true;
        ofLog() << "STARTING VOLUME TEST";
    }
    void update()
    {
        if(test)
        {
            if(doPause)
            {
                ofLogVerbose() << "Paused test currentFrame: " << test->omxPlayer->getCurrentFrame();
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
                        doPause = true;
                             
                    }
                }
                
            }
            if(doSeekTest)
            {
                float totalSeconds = test->omxPlayer->getDurationInSeconds();
                if(totalSeconds)
                {
                    float middle = totalSeconds*0.5;
                    int frameTarget = middle*test->omxPlayer->getFPS();
                    stringstream info;
                    info << "totalSeconds: " << totalSeconds << endl;
                    info << "middle: " << middle << endl;
                    info << "frameTarget: " << frameTarget << endl;

                    ofLogVerbose(__func__) << info.str();

                    test->omxPlayer->seekToTimeInSeconds(middle);
                    //doPause = true;
                }else
                {
                    ofLogError(__func__) << "TOTAL SECONDS IS ZERO";
                }
                
                doSeekTest = false;
                
            }
            
            if(doSeekToFrameTest)
            {
                test->omxPlayer->seekToFrame(250);
                doSeekToFrameTest =false;
            }
   
            if(doVolumeTest)
            {
                float currentVolume = test->omxPlayer->getVolume();
                if(ofGetFrameNum() % 30 == 0)
                {

                    if(!increaseTestComplete)
                    {
                        ofLog() << "increaseTestComplete: " << increaseTestComplete << "currentVolume: " << currentVolume;
                        if(currentVolume < 1)
                        {
                            test->omxPlayer->increaseVolume();
                            currentVolume = test->omxPlayer->getVolume();
                            if(currentVolume >= 1)
                            {
                                increaseTestComplete = true;
                            }
                        }
                    }else
                    {
                        ofLog() << "decreaseTestComplete: " << decreaseTestComplete << "currentVolume: " << currentVolume;

                        if(!decreaseTestComplete)
                        {
                            currentVolume = test->omxPlayer->getVolume();
                            if(currentVolume >= 0)
                            {
                                test->omxPlayer->decreaseVolume();
                                currentVolume = test->omxPlayer->getVolume();
                                if(currentVolume <= 0)
                                {
                                    decreaseTestComplete = true;
                                }
                            }
                        }
                    }
                    if(decreaseTestComplete && increaseTestComplete)
                    {
                        test->omxPlayer->setVolume(0.5);
                        doVolumeTest = false;
                    }
                }
            }
            
        }
    
    }
    
};