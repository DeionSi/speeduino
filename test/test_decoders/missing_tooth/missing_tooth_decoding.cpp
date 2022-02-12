#include "Arduino.h"
#include "unity.h"
#include "init.h"
#include "globals.h"
#include "decoders.h"
#include "missing_tooth_decoding_testdata.h"

decodingTest *currentDecodingTest;
uint16_t expectedRPM;

void testDecoding_reset() {
  //This is from Speeduinos main loop which check for engine stall/turn off and resets a bunch decoder related values
  //TODO: Use a function in speeduino to do this
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
  currentDecodingTest->decodingSetup();

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
  const uint32_t delayLength = 1000;

  // TODO: Everything below is to do:

  // 12-1
  const byte gaps = 11;
  uint32_t delays[gaps] = {
    delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength, delayLength*2
  };
  uint16_t finalDelay = 0;
  uint32_t nextTrigger = micros();
  
  const byte unityMessageLength = 100;
  char unityMessage[unityMessageLength];

  const byte triggersCount = 20;


  uint32_t rotationTime = 0;
  for (int i = 0; i < gaps; i++) {
    rotationTime += delays[gaps];
  }
  expectedRPM = 1 / (rotationTime / 1000 / 1000 / 60);
  snprintf(unityMessage, unityMessageLength, "expectedRPM %u", expectedRPM);
  UnityPrint(unityMessage);
  UNITY_PRINT_EOL();

  for (int secondaryPosition = -3, patternPosition = 1; secondaryPosition < triggersCount; secondaryPosition++, patternPosition++) {
    
    triggerHandler();

    snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    UNITY_PRINT_EOL();

    /*if (secondaryPosition % secondaryInterval == 0) {
      triggerSec_UniversalDecoder();
    }*/

    if (patternPosition >= gaps) {
      patternPosition = 0;
    }
    nextTrigger += delays[patternPosition];

    snprintf(unityMessage, unityMessageLength, "delay %lu", nextTrigger - micros());
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();

    while (micros() < nextTrigger-12000) {
      delay(10);
    }

    finalDelay = nextTrigger - micros();
    delayMicroseconds(finalDelay);
  }

}

void testSync() {
  TEST_ASSERT_EQUAL(currentDecodingTest->expectedSync, currentStatus.hasSync);
}
void testHalfSync() {
  TEST_ASSERT_EQUAL(currentDecodingTest->expectedHalfSync, BIT_CLEAR(currentStatus.status3, BIT_STATUS3_HALFSYNC));
}
void testSyncLossCount() {
  TEST_ASSERT_EQUAL(currentDecodingTest->expectedSyncLossCount, currentStatus.syncLossCounter);
}
void testRPM() {
  TEST_ASSERT_EQUAL(expectedRPM, getRPM());
}
void testRevolutionCounter() {
  TEST_ASSERT_EQUAL(currentDecodingTest->expectedRevolutionCount, currentStatus.startRevolutions); // How to count?
}

void outputTests() {

  //TODO: What are the expected decoder outputs?
  RUN_TEST(testSync);
  RUN_TEST(testHalfSync);
  RUN_TEST(testSyncLossCount);
  //TODO: move calculation of expected RPM to here
  RUN_TEST(testRPM);
  RUN_TEST(testRevolutionCounter);
}

void testMissingToothDecoding() {
  // Perform the test
  for (auto testData : decodingTests) {
    currentDecodingTest = &testData;
    UnityMessage(currentDecodingTest->name, __LINE__);
    testMissingToothDecoding_execute();
    outputTests();
    testDecoding_reset();
  }
}