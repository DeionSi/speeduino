#include "Arduino.h"
#include "unity.h"
#include "globals.h"
#include "decoders.h"
#include "../test_decoding.h"

/* Instructions:
 * Don't start tests on tooth one (unless there is only one tooth)
 * Tooth one is identified by angle 0
 * 
 */

void test0_setup() {
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.triggerAngle = 0; //TODO: Use different values
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_SINGLE; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 0; //TODO: What value to use and test with? //There are calculation issues with high rpm and initial rpm calculation
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

// TODO: test deltas
testParams test0_unsynced0[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
};
// TODO: Separate calculated tests for shared usage
testParams test0_state1[] = {
  { testParams::SYNC, 1 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
  { testParams::REVCOUNT_c, 0 },
  { testParams::REVTIME_c, 0, 52 },
  { testParams::TOOTHANGLECORRECT, 1 },
  { testParams::TOOTHANGLE_c, 0 },
  { testParams::LASTTOOTHTIME_c, 0, 4 },
  { testParams::LASTTOOTHTIMEMINUSONE_c, 0, 4 },
  { testParams::RPM, 5000, 20 }, // This test cannot be more precise because of micros() rounding to every 4th
  { testParams::STALLTIME_c, 0 },
  { testParams::CRANKANGLE_c, 0, 1 },
};
// TODO: reset test global variables when losing sync
testParams test0_state2[] = {
  { testParams::SYNC, 1 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
  { testParams::REVCOUNT_c, 0 },
  { testParams::REVTIME_c, 0, 52 },
  { testParams::TOOTHANGLECORRECT, 1 },
  { testParams::TOOTHANGLE_c, 0 },
  { testParams::LASTTOOTHTIME_c, 0, 4 },
  { testParams::LASTTOOTHTIMEMINUSONE_c, 0, 4 },
  { testParams::RPM, 5000, 20 }, // This test cannot be more precise because of micros() rounding to every 4th
  { testParams::STALLTIME_c, 0 },
  { testParams::CRANKANGLE_c, 0, 1 },
};
testParams test0_crank[] = {
  { testParams::CRANKANGLE_c, 0, 1 }
};

testTooth mt_t1  { 0, 30 };
testTooth mt_t2  { 30, 30 };
testTooth mt_t3  { 60, 30 };
testTooth mt_t4  { 90, 30 };
testTooth mt_t5  { 120, 30 };
testTooth mt_t6  { 150, 30 };
testTooth mt_t7  { 180, 30 };
testTooth mt_t8  { 210, 30 };
testTooth mt_t9  { 240, 30 };
testTooth mt_t10 { 270, 30 };
testTooth mt_t11 { 300, 60 };

//TODO: a stalling test to verify all parameters are returned
//TODO: Cranking tests
timedEvent test0_events[] {
  { timedEvent::PRITRIG, 1000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t1 },
  { timedEvent::PRITRIG, 2000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t2 },
  { timedEvent::PRITRIG, 3000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t3 },
  { timedEvent::PRITRIG, 4000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t4 },
  { timedEvent::PRITRIG, 5000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t5 },
  { timedEvent::PRITRIG, 6000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t6 },
  { timedEvent::PRITRIG, 7000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t7 },
  { timedEvent::PRITRIG, 8000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t8 },
  { timedEvent::PRITRIG, 9000,     timedEventArrayTestEntry(test0_unsynced0), &mt_t9 },
  { timedEvent::PRITRIG, 10000,    timedEventArrayTestEntry(test0_unsynced0), &mt_t10 },
  { timedEvent::PRITRIG, 11000,    timedEventArrayTestEntry(test0_unsynced0), &mt_t11 },
  { timedEvent::PRITRIG, 13000,    timedEventArrayTestEntry(test0_state1), &mt_t1 },
  { timedEvent::PRITRIG, 14000,    timedEventArrayTestEntry(test0_state1), &mt_t2 },
  { timedEvent::PRITRIG, 15000,    timedEventArrayTestEntry(test0_state1), &mt_t3 },
  { timedEvent::PRITRIG, 16000,    timedEventArrayTestEntry(test0_state1), &mt_t4 },
  { timedEvent::PRITRIG, 17000,    timedEventArrayTestEntry(test0_state1), &mt_t5 },
  { timedEvent::PRITRIG, 18000,    timedEventArrayTestEntry(test0_state1), &mt_t6 },
  { timedEvent::PRITRIG, 19000,    timedEventArrayTestEntry(test0_state1), &mt_t7 },
  { timedEvent::PRITRIG, 20000,    timedEventArrayTestEntry(test0_state1), &mt_t8 },
  { timedEvent::PRITRIG, 21000,    timedEventArrayTestEntry(test0_state1), &mt_t9 },
  { timedEvent::PRITRIG, 22000,    timedEventArrayTestEntry(test0_state1), &mt_t10 },
  { timedEvent::PRITRIG, 23000,    timedEventArrayTestEntry(test0_state1), &mt_t11 },
  { timedEvent::PRITRIG, 25000,    timedEventArrayTestEntry(test0_state2), &mt_t1 },
  { timedEvent::TEST,    25500,    timedEventArrayTestEntry(test0_crank) },
  { timedEvent::PRITRIG, 26000,    timedEventArrayTestEntry(test0_state2), &mt_t2 },
  { timedEvent::PRITRIG, 27000,    timedEventArrayTestEntry(test0_state2), &mt_t3 },
  { timedEvent::PRITRIG, 28000,    timedEventArrayTestEntry(test0_state2), &mt_t4 },
/*  { .type = timedEvent::PRITRIG, .time = 29000,    .test = nullptr }, // 5
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
  { "Missing tooth test 0, 12-1 wasted spark", test0_setup, test0_events, countof(test0_events) },
};

void testDecodingMissingTooth() {
  for (auto testData : decodingTests) {
    testData.execute();
    testData.showTriggerlog();
  }
}