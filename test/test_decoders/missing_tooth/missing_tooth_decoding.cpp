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

//TODO Classify
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

  uint32_t triggerLog[currentDecodingTest->eventCount];
  uint32_t timingOffsetFrom0 = micros();
  
  for (int i = 0; i < currentDecodingTest->eventCount; i++) {
    
    //Only perform calculations before a test. We don't need to recalculate before subsequent tests
    if ( ( i == 0 || ( i > 0 && currentDecodingTest->events[i-1].type != tet_TEST ) ) && currentDecodingTest->events[i].type == tet_TEST) {
      currentStatus.RPM = getRPM();
      doCrankSpeedCalcs();
    }

    if (currentDecodingTest->events[i].time < UINT_MAX) { // UINT_MAX Entries are parsed as soon as they are reached
      delayUntil(currentDecodingTest->events[i].time + timingOffsetFrom0);
    }

    triggerLog[i] = micros();

    currentDecodingTest->events[i].execute();

    //UnityPrint("trigger ");
    //UnityPrintNumberUnsigned(triggerLog[triggerLogPos-1]);
    //UNITY_PRINT_EOL();

    /*snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();*/

  }

  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (int i = 0; i < currentDecodingTest->eventCount; i++) {
    uint32_t displayTime = triggerLog[i] - lastTriggerTime;
    lastTriggerTime = triggerLog[i];

    snprintf(unityMessage, unityMessageLength, "time %lu timefromstart %lu interval %lu ", triggerLog[i], triggerLog[i] - timingOffsetFrom0, displayTime);
    UnityPrint(unityMessage);
    if (currentDecodingTest->events[i].test != nullptr) {
      UnityPrint(timedTestTypeFriendlyName[currentDecodingTest->events[i].test->type]);
    }
    UNITY_PRINT_EOL();
  }

}


//TODO: What are the expected decoder outputs?

void testMissingToothDecoding() {
  // Perform the test
  for (auto testData : decodingTests) {
    currentDecodingTest = &testData;

    // Don't get poisoned by the previous test name on this INFO
    Unity.CurrentTestName = NULL;

    UnityMessage(currentDecodingTest->name, __LINE__);
    testMissingToothDecoding_execute();

    for (int i = 0; i < currentDecodingTest->eventCount; i++) {

      currentDecodingTest->events[i].run_test();

    }

    testDecoding_reset();
  }
}