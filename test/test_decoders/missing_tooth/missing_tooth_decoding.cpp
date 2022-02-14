#include "Arduino.h"
#include "unity.h"
#include "init.h"
#include "globals.h"
#include "decoders.h"
#include "missing_tooth_decoding_testdata.h"
#include "crankMaths.h"

const byte unityMessageLength = 200;
char unityMessage[unityMessageLength];

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

void delayUntil(uint32_t time) {
  if (micros() < time) { // No delay if we have already passed
    if (time >= 12000) { // Make sure we don't compare against negative time
      while (micros() < time - 12000) { delay(10); } // Delay in MILLIseconds until we can delay in MICROseconds
    }
    delayMicroseconds(time - micros()); // Remaining delay in MICROseconds
  }
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

  uint32_t triggerLog[currentDecodingTest->primaryTriggerPatternExecuteCount + currentDecodingTest->timedTestsCount];
  uint32_t nextTrigger = 0;
  uint32_t timingOffsetFrom0 = micros();
  byte timedTestPos = 0;
  byte triggerLogPos = 0;
  
  for (int i = 0, patternPosition = currentDecodingTest->primaryTriggerPatternStartPos; i < currentDecodingTest->primaryTriggerPatternExecuteCount;) {

    delayUntil(nextTrigger + timingOffsetFrom0);

    triggerLog[triggerLogPos] = micros();
    triggerLogPos++;

    triggerHandler();

    //UnityPrint("trigger ");
    //UnityPrintNumberUnsigned(triggerLog[triggerLogPos-1]);
    //UNITY_PRINT_EOL();

    //snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    //UNITY_PRINT_EOL();
    
    if (patternPosition >= currentDecodingTest->primaryTriggerPatternCount) { patternPosition = 0; }
    uint32_t previousTrigger = nextTrigger;
    nextTrigger += currentDecodingTest->primaryTriggerPattern[patternPosition];

    i++;
    patternPosition++;

    // This is done in the normal Speeduino loop and is needed for (some?) getCrankAngle calculations
    currentStatus.RPM = getRPM();
    doCrankSpeedCalcs();

    // Do any timed tests before we wait for the next trigger
    while (timedTestPos < currentDecodingTest->timedTestsCount
            && currentDecodingTest->timedTests[timedTestPos].time < nextTrigger
            && currentDecodingTest->timedTests[timedTestPos].time >= previousTrigger
            ) {
      
      // Delay until test target time
      delayUntil(currentDecodingTest->timedTests[timedTestPos].time + timingOffsetFrom0);

      triggerLog[triggerLogPos] = micros();
      triggerLogPos++;

      currentDecodingTest->timedTests[timedTestPos].prepareResult();
      timedTestPos++;

      //snprintf(unityMessage, unityMessageLength, "interval %lu", displayTime);
      /*UnityPrint("trigger test");
      UNITY_PRINT_EOL();*/
    }

  }

  // Run remaining tests that should happen at end
  while (timedTestPos < currentDecodingTest->timedTestsCount) {
    currentDecodingTest->timedTests[timedTestPos].prepareResult();
    timedTestPos++;
  }

  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (int i = 0; i < triggerLogPos; i++) {
    uint32_t displayTime = triggerLog[i] - lastTriggerTime;
    lastTriggerTime = triggerLog[i];

    snprintf(unityMessage, unityMessageLength, "time %lu timefromstart %lu interval %lu", triggerLog[i], triggerLog[i] - timingOffsetFrom0, displayTime);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();
  }

}

static void run_test() {
  TEST_ASSERT_INT_WITHIN(currentTimedTest->delta, currentTimedTest->expected, currentTimedTest->result);
}

//TODO: What are the expected decoder outputs?

void testMissingToothDecoding() {
  // Perform the test
  for (auto testData : decodingTests) {
    currentDecodingTest = &testData;

    // Do this to not get poisoned by the previous test name on this INFO
    Unity.CurrentTestName = NULL;

    UnityMessage(currentDecodingTest->name, __LINE__);
    testMissingToothDecoding_execute();

    for (int i = 0; i < currentDecodingTest->timedTestsCount; i++) {
      currentTimedTest = &currentDecodingTest->timedTests[i];

      //snprintf(unityMessage, unityMessageLength, "delta %u expected %u result %u", currentTimedTest->delta, currentTimedTest->expected, currentTimedTest->result);
      //UnityMessage(unityMessage, __LINE__);

      snprintf(unityMessage, unityMessageLength, "%d %s", i, timedTestTypeFriendlyName[currentTimedTest->type]);
      UnityDefaultTestRun(run_test, unityMessage, __LINE__);
    }

    testDecoding_reset();
  }
}