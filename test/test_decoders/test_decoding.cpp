#include "arduino.h"
#include "decoders.h"
#include "crankMaths.h"
#include "unity.h"
#include "init.h"
#include "test_decoding.h"

const byte unityMessageLength = 200;
char unityMessage[unityMessageLength];

const char *timedTestTypeFriendlyName[] = { 
  "Crankangle",
  "Sync",
  "Half-sync",
  "Sync-loss count",
  "RPM",
  "Revolution count",
};

//TODO: What are the expected decoder outputs?
// sync, halfsync, synclosscount, revolutioncount, rpm, crankangle, MAX_STALL_TIME, toothCurrentCount, toothLastToothTime, 
// toothLastMinusOneToothTime, toothOneTime, toothOneMinusOneTime, triggerToothAngle, triggerToothAngleIsCorrect, secondDerivEnabled

void delayUntil(uint32_t time) {
  if (micros() < time) { // No delay if we have already passed
    if (time >= 12000) { // Make sure we don't compare against negative time
      while (micros() < time - 12000) { delay(10); } // Delay in MILLIseconds until we can delay in MICROseconds
    }
    delayMicroseconds(time - micros()); // Remaining delay in MICROseconds
  }
}

testParams *currentTest;

void testParams::execute() {
  switch(type) {
    case ttt_CRANKANGLE:
      result = getCrankAngle();
    break;
    case ttt_SYNC:
      result = currentStatus.hasSync;
    break;
    case ttt_HALFSYNC:
      result = BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC);
    break;
    case ttt_SYNCLOSSCOUNT:
      result = currentStatus.syncLossCounter;
    break;
    case ttt_RPM:
      result = getRPM();
    break;
    case ttt_REVCOUNT:
      result = currentStatus.startRevolutions;
    break;
  }
}

void testParams::run_test() {
  snprintf(unityMessage, unityMessageLength, "delta %u expected %u result %u ", currentTest->delta, currentTest->expected, currentTest->result);
  UnityPrint(unityMessage);
  for (int i = strlen(unityMessage); i < 40; i++) { UnityPrint(" "); }

  if (currentTest->delta == 0) {
    TEST_ASSERT_EQUAL(currentTest->expected, currentTest->result);
  }
  else {
    TEST_ASSERT_INT_WITHIN(currentTest->delta, currentTest->expected, currentTest->result);
  }
}

void timedEvent::execute() {
  if (type == tet_PRIMARYTRIGGER) {
    triggerHandler();
  }
  else if (type == tet_TEST) {
    test->execute();
  }
};

void timedEvent::run_test() {
  if (type == tet_TEST) {
    currentTest = test;
    UnityDefaultTestRun(test->run_test, timedTestTypeFriendlyName[currentTest->type], __LINE__);
  }
}

void decodingTest::execute() {
  // Don't get the previous test name on this INFO message
  Unity.CurrentTestName = NULL;

  UnityMessage(name, __LINE__);

  decodingSetup();

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

  uint32_t triggerLog[eventCount];
  uint32_t timingOffsetFrom0 = micros();

  for (int i = 0; i < eventCount; i++) {
    
    //Only perform calculations before a test. We don't need to recalculate before subsequent tests
    if ( ( i == 0 || ( i > 0 && events[i-1].type != tet_TEST ) ) && events[i].type == tet_TEST) {
      currentStatus.RPM = getRPM();
      doCrankSpeedCalcs();
    }

    if (events[i].time < UINT_MAX) { // UINT_MAX Entries are parsed as soon as they are reached
      delayUntil(events[i].time + timingOffsetFrom0);
    }

    triggerLog[i] = micros();

    events[i].execute();

    //UnityPrint("trigger ");
    //UnityPrintNumberUnsigned(triggerLog[triggerLogPos-1]);
    //UNITY_PRINT_EOL();

    /*snprintf(unityMessage, unityMessageLength, "getCrankAngle %d / hasSync %d", getCrankAngle(), currentStatus.hasSync);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();*/

  }

  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (int i = 0; i < eventCount; i++) {
    uint32_t displayTime = triggerLog[i] - lastTriggerTime;
    lastTriggerTime = triggerLog[i];

    snprintf(unityMessage, unityMessageLength, "time %lu timefromstart %lu interval %lu ", triggerLog[i], triggerLog[i] - timingOffsetFrom0, displayTime);
    UnityPrint(unityMessage);
    if (events[i].test != nullptr) {
      UnityPrint(timedTestTypeFriendlyName[events[i].test->type]);
    }
    UNITY_PRINT_EOL();
  }

  }

void decodingTest::run_tests() {
  for (int i = 0; i < eventCount; i++) {
    events[i].run_test();
  }
}

void decodingTest::reset_decoding() {
  //This is from Speeduinos main loop which checks for engine stall/turn off and resets a bunch decoder related values
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