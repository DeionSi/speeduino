#include "Arduino.h"
#include "unity.h"
#include "globals.h"
#include "decoders.h"
#include "../test_decoding.h"

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

testParams test0_state0[] = { { testParams::SYNC, 0 }, { testParams::HALFSYNC, 0 }, { testParams::REVCOUNT, 0 } };
testParams test0_state1[] = { { testParams::SYNC, 1 }, { testParams::HALFSYNC, 0 } };
testParams test0_test0[] = { { testParams::REVCOUNT, 0 } };

//TODO: a stalling test to verify all parameters are returned
//TODO: Cranking tests
timedEvent test0_events[] {
  { .type = timedEvent::PRITRIG, .time = 1000,     timedEventArrayTestEntry(test0_state0) }, // 1
  { .type = timedEvent::PRITRIG, .time = 2000,     timedEventArrayTestEntry(test0_state0) }, // 2
  { .type = timedEvent::PRITRIG, .time = 3000,     timedEventArrayTestEntry(test0_state0) }, // 3
  { .type = timedEvent::PRITRIG, .time = 4000,     timedEventArrayTestEntry(test0_state0) }, // 4
  { .type = timedEvent::PRITRIG, .time = 5000,     timedEventArrayTestEntry(test0_state0) }, // 5
  { .type = timedEvent::PRITRIG, .time = 6000,     timedEventArrayTestEntry(test0_state0) }, // 6
  { .type = timedEvent::PRITRIG, .time = 7000,     timedEventArrayTestEntry(test0_state0) }, // 7
  { .type = timedEvent::PRITRIG, .time = 8000,     timedEventArrayTestEntry(test0_state0) }, // 8
  { .type = timedEvent::PRITRIG, .time = 9000,     timedEventArrayTestEntry(test0_state0) }, // 9
/*  { .type = timedEvent::PRITRIG, .time = 10000,    timedEventArrayTestEntry(test0_state0) }, // 10
  { .type = timedEvent::PRITRIG, .time = 11000,    timedEventArrayTestEntry(test0_state0) }, // 11
  { .type = timedEvent::PRITRIG, .time = 13000,    timedEventArrayTestEntry(test0_state0) }, // 1 (incl missing tooth 12)
  { .type = timedEvent::TEST,    .time = 13700,    timedEventArrayTestEntry(test0_test0) },
  { .type = timedEvent::PRITRIG, .time = 14000,    timedEventArrayTestEntry(test0_state1) }, // 2
  { .type = timedEvent::PRITRIG, .time = 15000,    timedEventArrayTestEntry(test0_state1) }, // 3
  { .type = timedEvent::PRITRIG, .time = 16000,    timedEventArrayTestEntry(test0_state1) }, // 4
  { .type = timedEvent::PRITRIG, .time = 17000,    .test = nullptr }, // 5
  { .type = timedEvent::PRITRIG, .time = 18000,    .test = nullptr }, // 6
  { .type = timedEvent::PRITRIG, .time = 19000,    .test = nullptr }, // 7
  { .type = timedEvent::PRITRIG, .time = 20000,    .test = nullptr }, // 8
  { .type = timedEvent::PRITRIG, .time = 21000,    .test = nullptr }, // 9
  { .type = timedEvent::PRITRIG, .time = 22000,    .test = nullptr }, // 10
  { .type = timedEvent::PRITRIG, .time = 23000,    .test = nullptr }, // 11
  { .type = timedEvent::PRITRIG, .time = 25000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = timedEvent::TEST,           .time = 25500,    .test = new testParams ( testParams::CRANKANGLE, 15) },
  { .type = timedEvent::PRITRIG, .time = 26000,    .test = nullptr }, // 2
  { .type = timedEvent::TEST,           .time = 26500,    .test = new testParams ( testParams::CRANKANGLE, 45) },
  { .type = timedEvent::PRITRIG, .time = 27000,    .test = nullptr }, // 3
  { .type = timedEvent::PRITRIG, .time = 28000,    .test = nullptr }, // 4
  { .type = timedEvent::PRITRIG, .time = 29000,    .test = nullptr }, // 5
  { .type = timedEvent::PRITRIG, .time = 30000,    .test = nullptr }, // 6
  { .type = timedEvent::PRITRIG, .time = 31000,    .test = nullptr }, // 7
  { .type = timedEvent::PRITRIG, .time = 32000,    .test = nullptr }, // 8
  { .type = timedEvent::PRITRIG, .time = 33000,    .test = nullptr }, // 9
  { .type = timedEvent::PRITRIG, .time = 34000,    .test = nullptr }, // 10
  { .type = timedEvent::PRITRIG, .time = 35000,    .test = nullptr }, // 11
  { .type = timedEvent::PRITRIG, .time = 37000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = timedEvent::TEST,           .time = UINT32_MAX, .test = new testParams (testParams::SYNC,          1   ) },
  { .type = timedEvent::TEST,           .time = UINT32_MAX, .test = new testParams (testParams::HALFSYNC,      0   ) },
  { .type = timedEvent::TEST,           .time = UINT32_MAX, .test = new testParams (testParams::SYNCLOSSCOUNT, 0   ) },
  { .type = timedEvent::TEST,           .time = UINT32_MAX, .test = new testParams (testParams::RPM,           5000) },
  { .type = timedEvent::TEST,           .time = UINT32_MAX, .test = new testParams (testParams::REVCOUNT,      2   ) },*/
};

decodingTest decodingTests[] = {
  {
  .name = "Missing tooth test 0, 12-1 wasted spark",
  .decoderSetup = test0_setup,
  .events = test0_events,
  .eventCount = countof(test0_events),
  },
};

void testDecodingMissingTooth() {
  for (auto testData : decodingTests) {
    testData.execute();
    testData.showTriggerlog();
  }
}