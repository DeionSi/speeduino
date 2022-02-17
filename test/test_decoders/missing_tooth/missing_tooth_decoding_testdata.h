#include "arduino.h"
#include "decoders.h"
#include "missing_tooth_decoding.h"
#include "crankMaths.h"

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

enum timedTestType {
  ttt_CRANKANGLE,
  ttt_SYNC,
  ttt_HALFSYNC,
  ttt_SYNCLOSSCOUNT,
  ttt_RPM,
  ttt_REVCOUNT
};

const char *timedTestTypeFriendlyName[] = { 
  "Crankangle",
  "Sync",
  "Half-sync",
  "Sync-loss count",
  "RPM",
  "Revolution count",
};

enum timedEventType {
  tet_PRIMARYTRIGGER,
  tet_TEST,
};

struct testParams;
testParams *currentTest;

struct testParams {

  const timedTestType type;
  const uint16_t expected;
  const uint8_t delta;
  uint16_t result;

  void execute() {
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

  static void run_test() {
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

};

struct timedEvent {
  void execute() {
    if (type == tet_PRIMARYTRIGGER) {
      triggerHandler();
    }
    else if (type == tet_TEST) {
      test->execute();
    }
  };
  void run_test() {
    if (type == tet_TEST) {
      currentTest = test;
      UnityDefaultTestRun(test->run_test, timedTestTypeFriendlyName[currentTest->type], __LINE__);
    }
  }

  const timedEventType type;
  const uint32_t time;
  testParams *test;
};

struct timedEvent;
timedEvent *currentTimedTest;

struct decodingTest {
  const char *name;
  void (*decodingSetup)();
  timedEvent *events;
  const byte eventCount;

  void execute() {
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

  void run_tests() {
    for (int i = 0; i < eventCount; i++) {
      events[i].run_test();
    }
  }

  static void reset_decoding() {
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

};

void test0_setup() {
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.triggerAngle = 0; //TODO: Use different values
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_SINGLE; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 1; //TODO: What value to use and test with? //There are calculation issues with high rpm and initial rpm calculation
  configPage10.vvt2Enabled = 0; //TODO: Enable VVT testing
  currentStatus.crankRPM = 200; //TODO: What value to use and test with?
  configPage4.triggerFilter = 0; //TODO: Test filters

  //TODD: These are relevant to each other, how to set/test them?
  configPage4.sparkMode = IGN_MODE_WASTED;
  configPage2.injLayout = INJ_PAIRED;
  CRANK_ANGLE_MAX = CRANK_ANGLE_MAX_IGN = 360;

  // These are not important but set them for clarity's sake
  configPage4.TrigEdge = 0;
  configPage4.TrigEdgeSec = 0;
  configPage10.TrigEdgeThrd = 0;
}

//TODO: Cranking tests
timedEvent test0_events[] {
  { .type = tet_PRIMARYTRIGGER, .time = 1000,     .test = nullptr }, // 1
  { .type = tet_PRIMARYTRIGGER, .time = 2000,     .test = nullptr }, // 2
  { .type = tet_PRIMARYTRIGGER, .time = 3000,     .test = nullptr }, // 3
  { .type = tet_PRIMARYTRIGGER, .time = 4000,     .test = nullptr }, // 4
  { .type = tet_PRIMARYTRIGGER, .time = 5000,     .test = nullptr }, // 5
  { .type = tet_PRIMARYTRIGGER, .time = 6000,     .test = nullptr }, // 6
  { .type = tet_PRIMARYTRIGGER, .time = 7000,     .test = nullptr }, // 7
  { .type = tet_PRIMARYTRIGGER, .time = 8000,     .test = nullptr }, // 8
  { .type = tet_PRIMARYTRIGGER, .time = 9000,     .test = nullptr }, // 9
  { .type = tet_PRIMARYTRIGGER, .time = 10000,    .test = nullptr }, // 10
  { .type = tet_PRIMARYTRIGGER, .time = 11000,    .test = nullptr }, // 11
  { .type = tet_TEST,           .time = 11500,    .test = new testParams { .type = ttt_SYNC,          .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = 11500,    .test = new testParams { .type = ttt_HALFSYNC,      .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_PRIMARYTRIGGER, .time = 13000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = 13500,    .test = new testParams { .type = ttt_SYNC,          .expected = 1,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = 13500,    .test = new testParams { .type = ttt_HALFSYNC,      .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = 13700,    .test = new testParams { .type = ttt_REVCOUNT,      .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_PRIMARYTRIGGER, .time = 14000,    .test = nullptr }, // 2
  { .type = tet_PRIMARYTRIGGER, .time = 15000,    .test = nullptr }, // 3
  { .type = tet_PRIMARYTRIGGER, .time = 16000,    .test = nullptr }, // 4
  { .type = tet_PRIMARYTRIGGER, .time = 17000,    .test = nullptr }, // 5
  { .type = tet_PRIMARYTRIGGER, .time = 18000,    .test = nullptr }, // 6
  { .type = tet_PRIMARYTRIGGER, .time = 19000,    .test = nullptr }, // 7
  { .type = tet_PRIMARYTRIGGER, .time = 20000,    .test = nullptr }, // 8
  { .type = tet_PRIMARYTRIGGER, .time = 21000,    .test = nullptr }, // 9
  { .type = tet_PRIMARYTRIGGER, .time = 22000,    .test = nullptr }, // 10
  { .type = tet_PRIMARYTRIGGER, .time = 23000,    .test = nullptr }, // 11
  { .type = tet_PRIMARYTRIGGER, .time = 25000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = 25500,    .test = new testParams { .type = ttt_CRANKANGLE,    .expected = 15,   .delta = 0, .result = 0 } },
  { .type = tet_PRIMARYTRIGGER, .time = 26000,    .test = nullptr }, // 2
  { .type = tet_TEST,           .time = 26500,    .test = new testParams { .type = ttt_CRANKANGLE,    .expected = 45,   .delta = 0, .result = 0 } },
  { .type = tet_PRIMARYTRIGGER, .time = 27000,    .test = nullptr }, // 3
  { .type = tet_PRIMARYTRIGGER, .time = 28000,    .test = nullptr }, // 4
  { .type = tet_PRIMARYTRIGGER, .time = 29000,    .test = nullptr }, // 5
  { .type = tet_PRIMARYTRIGGER, .time = 30000,    .test = nullptr }, // 6
  { .type = tet_PRIMARYTRIGGER, .time = 31000,    .test = nullptr }, // 7
  { .type = tet_PRIMARYTRIGGER, .time = 32000,    .test = nullptr }, // 8
  { .type = tet_PRIMARYTRIGGER, .time = 33000,    .test = nullptr }, // 9
  { .type = tet_PRIMARYTRIGGER, .time = 34000,    .test = nullptr }, // 10
  { .type = tet_PRIMARYTRIGGER, .time = 35000,    .test = nullptr }, // 11
  { .type = tet_PRIMARYTRIGGER, .time = 37000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = UINT_MAX, .test = new testParams { .type = ttt_SYNC,          .expected = 1,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = UINT_MAX, .test = new testParams { .type = ttt_HALFSYNC,      .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = UINT_MAX, .test = new testParams { .type = ttt_SYNCLOSSCOUNT, .expected = 0,    .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = UINT_MAX, .test = new testParams { .type = ttt_RPM,           .expected = 5000, .delta = 0, .result = 0 } },
  { .type = tet_TEST,           .time = UINT_MAX, .test = new testParams { .type = ttt_REVCOUNT,      .expected = 2,    .delta = 0, .result = 0 } },
};

decodingTest decodingTests[] = {
  {
  .name = "Missing tooth 12-1, wasted spark",
  .decodingSetup = test0_setup,
  .events = test0_events,
  .eventCount = sizeof(test0_events)/sizeof(test0_events[0]),
  },
};