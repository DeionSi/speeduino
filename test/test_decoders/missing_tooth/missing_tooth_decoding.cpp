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
  currentDecodingTest->decodingSetup();

  // Set pins because we need the trigger pin numbers, BoardID 3 (Speeduino v0.4 board) appears to have pin mappings for most variants
  setPinMapping(3);

  // initaliseTriggers attaches interrupts to trigger functions, so disabled interrupts, detach them and enable interrupts again
  // Interrupts are needed for micros() function
  noInterrupts();
  initialiseTriggers();
  detachInterrupt( digitalPinToInterrupt(pinTrigger) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger2) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger3) );
  interrupts();

  // TODO: Everything below is to do:

  const byte unityMessageLength = 200;
  char unityMessage[unityMessageLength];

  // Get expected RPM
  uint32_t rotationTime = 0;
  for (int i = 0; i < currentDecodingTest->primaryTriggerPatternCount; i++) { rotationTime += currentDecodingTest->primaryTriggerPattern[i]; }
  expectedRPM = 60000000L / rotationTime; // microseconds per minute divided by microseconds per revolution
  snprintf(unityMessage, unityMessageLength, "expectedRPM %u rotationTime %lu", expectedRPM, rotationTime);
  UnityPrint(unityMessage);
  UNITY_PRINT_EOL();

  uint32_t triggerLog[30];
  uint32_t nextTrigger = micros();
  
  for (int i = 0, patternPosition = currentDecodingTest->primaryTriggerPatternStartPos; i < currentDecodingTest->primaryTriggerPatternExecuteCount;) {

    triggerLog[i] = micros();

    triggerHandler();

    //snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    //UNITY_PRINT_EOL();

    
    if (patternPosition >= currentDecodingTest->primaryTriggerPatternCount) { patternPosition = 0; }
    nextTrigger += currentDecodingTest->primaryTriggerPattern[patternPosition];

    i++;
    patternPosition++;

    // Delay until next
    while (micros() < nextTrigger-12000) {
      delay(10);
    }
    delayMicroseconds(nextTrigger - micros());

  }

  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (uint32_t time : triggerLog) {
    uint32_t displayTime = 0;
    if (lastTriggerTime > 0) { displayTime = time - lastTriggerTime; }
    else { displayTime = time; }
    lastTriggerTime = time;

    snprintf(unityMessage, unityMessageLength, "interval %lu", displayTime);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();
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

    uint32_t rotationTime = 0;
    for (int i = 0; i < currentDecodingTest->primaryTriggerPatternCount; i++) { rotationTime += currentDecodingTest->primaryTriggerPattern[i]; }
    currentDecodingTest->expectedRPM = 60000000L / rotationTime;

    UnityMessage(currentDecodingTest->name, __LINE__);
    testMissingToothDecoding_execute();
    outputTests();
    testDecoding_reset();
  }
}