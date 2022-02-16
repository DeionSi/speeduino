#include "arduino.h"
#include "decoders.h"

//TODO: What are the expected decoder outputs?
// sync, halfsync, synclosscount, revolutioncount, rpm, crankangle, MAX_STALL_TIME, toothCurrentCount, toothLastToothTime, 
// toothLastMinusOneToothTime, toothOneTime, toothOneMinusOneTime, triggerToothAngle, triggerToothAngleIsCorrect, secondDerivEnabled

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
    TEST_ASSERT_INT_WITHIN(currentTest->delta, currentTest->expected, currentTest->result);
  }

};

struct timedEvent {
  void execute() {
    if (type == tet_PRIMARYTRIGGER) {
      triggerHandler();
    }
    else if (type == tet_TEST) {
      test.execute();
    }
  };
  void run_test() {
    if (type == tet_TEST) {
      currentTest = &test;
      RUN_TEST(test.run_test);
    }
  }

  const timedEventType type;
  const uint32_t time;
  testParams test;
};

struct decodingTest;
decodingTest *currentDecodingTest;
struct timedEvent;
timedEvent *currentTimedTest;

struct decodingTest {
  const char *name;
  void (*decodingSetup)();
  const byte primaryTriggerPatternCount;
  const byte primaryTriggerPatternStartPos;
  const byte primaryTriggerPatternExecuteCount;
  const uint16_t *primaryTriggerPattern;

  timedEvent *timedTests;
  const byte timedTestsCount;

};


void decodingTest0setup() {
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
// TODO: figure out a reasonable delay
const uint16_t test0delayLength = 1000;
const uint16_t test0delays[] = {
  test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength, test0delayLength*2
};

timedEvent test0timedTests[] = {
  { .type = tet_TEST, .time = 10500,    .test = { .type = ttt_SYNC,          .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = 10500,    .test = { .type = ttt_HALFSYNC,      .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = 12500,    .test = { .type = ttt_SYNC,          .expected = 1,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = 12500,    .test = { .type = ttt_HALFSYNC,      .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = 12700,    .test = { .type = ttt_REVCOUNT,      .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = 25500,    .test = { .type = ttt_CRANKANGLE,    .expected = 45, .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_SYNC,          .expected = 1,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_HALFSYNC,      .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_SYNCLOSSCOUNT, .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_RPM,           .expected = 0,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_REVCOUNT,      .expected = 2,  .delta = 0, .result = 0 } },
  { .type = tet_TEST, .time = UINT_MAX, .test = { .type = ttt_SYNC,          .expected = 1,  .delta = 0, .result = 0 } },
};

decodingTest decodingTests[] = {
  { // Missing tooth 12-1
  .name = "Missing tooth 12-1, wasted spark",
  .decodingSetup = decodingTest0setup,
  .primaryTriggerPatternCount = sizeof(test0delays)/sizeof(test0delays[0]),
  .primaryTriggerPatternStartPos = 0,
  .primaryTriggerPatternExecuteCount = 35,
  .primaryTriggerPattern = test0delays,
  .timedTests = test0timedTests,
  .timedTestsCount = sizeof(test0timedTests)/sizeof(test0timedTests[0]),
  },
};