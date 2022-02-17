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

testParams test0_range1_crankON(ttt_SYNC, 0);

//TODO: Automatic sorting of this list or relative time
//TODO: Show test number on output
//TODO: Cranking tests
timedEvent test0_events[] {
  { .type = tet_PRITRIG, .time = 1000,     .test = nullptr }, // 1
  { .type = tet_PRITRIG, .time = 2000,     .test = nullptr }, // 2
  { .type = tet_PRITRIG, .time = 3000,     .test = nullptr }, // 3
  { .type = tet_PRITRIG, .time = 4000,     .test = nullptr }, // 4
  { .type = tet_PRITRIG, .time = 5000,     .test = nullptr }, // 5
  { .type = tet_PRITRIG, .time = 6000,     .test = nullptr }, // 6
  { .type = tet_TEST,           .time = 6500,     .test = &test0_range1_crankON },
  { .type = tet_PRITRIG, .time = 7000,     .test = nullptr }, // 7
  { .type = tet_PRITRIG, .time = 8000,     .test = nullptr }, // 8
  { .type = tet_PRITRIG, .time = 9000,     .test = nullptr }, // 9
  { .type = tet_PRITRIG, .time = 10000,    .test = nullptr }, // 10
  { .type = tet_PRITRIG, .time = 11000,    .test = nullptr }, // 11
  { .type = tet_TEST,           .time = 11500,    .test = new testParams (ttt_SYNC,     0) },
  { .type = tet_TEST,           .time = 11500,    .test = new testParams (ttt_HALFSYNC, 0) },
  { .type = tet_PRITRIG, .time = 13000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = 13500,    .test = new testParams (ttt_SYNC,     1) },
  { .type = tet_TEST,           .time = 13500,    .test = new testParams (ttt_HALFSYNC, 0) },
  { .type = tet_TEST,           .time = 13700,    .test = new testParams (ttt_REVCOUNT, 0) },
  { .type = tet_PRITRIG, .time = 14000,    .test = nullptr }, // 2
  { .type = tet_PRITRIG, .time = 15000,    .test = nullptr }, // 3
  { .type = tet_PRITRIG, .time = 16000,    .test = nullptr }, // 4
  { .type = tet_PRITRIG, .time = 17000,    .test = nullptr }, // 5
  { .type = tet_PRITRIG, .time = 18000,    .test = nullptr }, // 6
  { .type = tet_PRITRIG, .time = 19000,    .test = nullptr }, // 7
  { .type = tet_PRITRIG, .time = 20000,    .test = nullptr }, // 8
  { .type = tet_PRITRIG, .time = 21000,    .test = nullptr }, // 9
  { .type = tet_PRITRIG, .time = 22000,    .test = nullptr }, // 10
  { .type = tet_PRITRIG, .time = 23000,    .test = nullptr }, // 11
  { .type = tet_PRITRIG, .time = 25000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = 25500,    .test = new testParams ( ttt_CRANKANGLE, 15) },
  { .type = tet_PRITRIG, .time = 26000,    .test = nullptr }, // 2
  { .type = tet_TEST,           .time = 26500,    .test = new testParams ( ttt_CRANKANGLE, 45) },
  { .type = tet_PRITRIG, .time = 27000,    .test = nullptr }, // 3
  { .type = tet_PRITRIG, .time = 28000,    .test = nullptr }, // 4
  { .type = tet_PRITRIG, .time = 29000,    .test = nullptr }, // 5
  { .type = tet_PRITRIG, .time = 30000,    .test = nullptr }, // 6
  { .type = tet_PRITRIG, .time = 31000,    .test = nullptr }, // 7
  { .type = tet_PRITRIG, .time = 32000,    .test = nullptr }, // 8
  { .type = tet_PRITRIG, .time = 33000,    .test = nullptr }, // 9
  { .type = tet_PRITRIG, .time = 34000,    .test = nullptr }, // 10
  { .type = tet_PRITRIG, .time = 35000,    .test = nullptr }, // 11
  { .type = tet_PRITRIG, .time = 37000,    .test = nullptr }, // 12+1 Missing tooth
  { .type = tet_TEST,           .time = UINT32_MAX, .test = new testParams (ttt_SYNC,          1   ) },
  { .type = tet_TEST,           .time = UINT32_MAX, .test = new testParams (ttt_HALFSYNC,      0   ) },
  { .type = tet_TEST,           .time = UINT32_MAX, .test = new testParams (ttt_SYNCLOSSCOUNT, 0   ) },
  { .type = tet_TEST,           .time = UINT32_MAX, .test = new testParams (ttt_RPM,           5000) },
  { .type = tet_TEST,           .time = UINT32_MAX, .test = new testParams (ttt_REVCOUNT,      2   ) },
};

decodingTest decodingTests[] = {
  {
  .name = "Missing tooth 12-1, wasted spark",
  .decodingSetup = test0_setup,
  .events = test0_events,
  .eventCount = sizeof(test0_events)/sizeof(test0_events[0]),
  },
};

void testDecodingMissingTooth() {
  for (auto testData : decodingTests) {
    testData.execute();
    testData.run_tests();
    testData.reset_decoding();
  }
}