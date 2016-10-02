#pragma once

#include "ofMain.h"
#include "TexturedLoopTest.h"
#include "TexturedStreamTest.h"

#include "TerminalListener.h"
#include "PlaybackTestRunner.h"

class ofApp : public ofBaseApp, public KeyListener, public TestListener
{
    
    
public:
    
    
    vector<BaseTest*> tests;
    BaseTest* currentTest;
    TerminalListener consoleListener;
    bool doDeleteCurrent;
    bool doNextTest;
    int currentTestID;
    PlaybackTestRunner playbackTestRunner;
    
    void setup()
    {
        
        doDeleteCurrent = false;
        doNextTest = false;
        
        consoleListener.setup(this);

        TexturedLoopTest* texturedLoopTest = new TexturedLoopTest();
        texturedLoopTest->setup("TexturedLoopTest");
        
        TexturedStreamTest* texturedStreamTest = new TexturedStreamTest();
        texturedStreamTest->setup("TexturedStreamTest");
        
        tests.push_back(texturedLoopTest);
        tests.push_back(texturedStreamTest);
        
        currentTestID = 0;
        currentTest = tests[currentTestID];
        currentTest->listener = this;
        currentTest->start();
        playbackTestRunner.test = currentTest;
  
    }
    
    
    void update()
    {
        if(doDeleteCurrent)
        {
            ofLogVerbose(__func__) << "CLOSING TEST: " << currentTest->name;
            currentTest->close();
            doDeleteCurrent = false;
            doNextTest = true;
            playbackTestRunner.stopAll();
        }
        
        if(doNextTest)
        {
            doNextTest = false;
            if(currentTestID+1 < tests.size())
            {
                currentTestID++;
            }else
            {
                currentTestID=0;
            }
            currentTest = tests[currentTestID];
            currentTest->listener = this;
            ofLogVerbose(__func__) << "STARTING NEW TEST: " << currentTest->name;
            ofSleepMillis(2000);
            
            currentTest->start();
            
        }
        
        if(currentTest)
        {
            currentTest->update();
        }
        
        playbackTestRunner.update();
    }
   
    
    
    
    void draw()
    {
        if(currentTest)
        {
            currentTest->draw();
        }
    }
    
    void onTestComplete(BaseTest* test)
    {
        ofLogVerbose(__func__) << test->name << " COMPLETE";
        doDeleteCurrent = true;
        
    }
    void onCharacterReceived(KeyListenerEventData& e)
    {
        keyPressed((int)e.character);
    }
    
    void keyPressed  (int key)
    {
        
        ofLogVerbose(__func__) << "key: " << key;
        switch (key) 
        {
            case '1':
            {
                playbackTestRunner.startPauseTest(currentTest);
                break;
            }
            case '2':
            {
                playbackTestRunner.startRestartTest(currentTest);
                break;
            }
            case '3':
            {
                playbackTestRunner.startStepTest(currentTest);
                break;
            }    
                
            case 'n':
            {
                doDeleteCurrent = true;
                break;
            }
                
        }
        if(currentTest)
        {
            currentTest->onKeyPressed(key);
        }
        
    }
};

