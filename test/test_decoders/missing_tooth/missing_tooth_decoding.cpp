#include "Arduino.h"
#include "unity.h"
#include "init.h"
#include "globals.h"
#include "decoders.h"

void testMissingToothDecoding_reset() {
  //This is from Speeduinos main loop which check for engine stall/turn off and resets a bunch decoder related values
  currentStatus.RPM = 0;
  toothLastToothTime = 0;
  toothLastSecToothTime = 0;
  currentStatus.hasSync = false;
  BIT_CLEAR(currentStatus.status3, BIT_STATUS3_HALFSYNC);
  currentStatus.startRevolutions = 0;
  toothSystemCount = 0;
  secondaryToothCount = 0;
}

void testMissingToothDecoding_execute() {
  //TODO: Call the function that is called when engine stalls and the decoder is reset

  //// Decoder configuration - START
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_POLL; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 1; //TODO: What value to use and test with?
  configPage10.vvt2Enabled = 0; //TODO: Enable VVT testing
  currentStatus.crankRPM = 200; //TODO: What value to use and test with?
  configPage4.triggerFilter = 0; //TODO: Test filters

  //TODD: These are relevant to each other, how to set/test them?
  configPage4.sparkMode = IGN_MODE_SEQUENTIAL;
  configPage2.injLayout == INJ_SEQUENTIAL;
  CRANK_ANGLE_MAX = CRANK_ANGLE_MAX_IGN = 720;

  // These are not important but set them for clarity's sake
  configPage4.TrigEdge = 0;
  configPage4.TrigEdgeSec = 0;
  configPage10.TrigEdgeThrd = 0;
  
  //// Decoder configuration - END

  // Set pins because we need the trigger pin numbers, BoardID 3 (Speeduino v0.4 board) appears to have pin mappings for most variants
  setPinMapping(3);

  // initaliseTriggers attaches interrupts to trigger functions, so disabled interrupts, detach them and enable interrupts again
  noInterrupts();
  initialiseTriggers();
  detachInterrupt( digitalPinToInterrupt(pinTrigger) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger2) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger3) );
  interrupts();

  // TODO: figure out a reasonable delay
  const uint32_t delayLength = 150000;

  // TODO: Everything below is to do:

  // 12-1
  const byte gaps = 11;
  uint32_t delays[gaps] = {
    delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength*2
  };
  const byte secondaryInterval = (gaps)*2;

  uint16_t finalDelay = 0;
  uint32_t nextTrigger = micros();
  
  const byte unityMessageLength = 100;
  char unityMessage[unityMessageLength];


  for (int secondaryPosition = -3, patternPosition = 1; secondaryPosition < 70; secondaryPosition++, patternPosition++) {
    
    while (micros() < nextTrigger-12000) {
      delay(10);
    }
    finalDelay = nextTrigger - micros();
    delayMicroseconds(finalDelay);

    triggerHandler();

    snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    UnityMessage(unityMessage, __LINE__);

    /*if (secondaryPosition % secondaryInterval == 0) {
      triggerSec_UniversalDecoder();
    }*/

    if (patternPosition >= gaps) {
      patternPosition = 0;
    }
    nextTrigger += delays[patternPosition];
  }

  //TODO: What are the expected decoder outputs?

  TEST_ASSERT_TRUE(currentStatus.hasSync);
  TEST_ASSERT_EQUAL(0, currentStatus.syncLossCounter);
  //TEST_ASSERT_EQUAL(3, currentStatus.startRevolutions); // How to count?

}

void testMissingToothDecoding() {
  
  RUN_TEST(testMissingToothDecoding_execute);
}