#include "Arduino.h"
#include "unity.h"
#include "globals.h"
#include "decoders.h"
#include "../test_decoding.h"

/* Instructions:
 * Tooth one is identified by angle 0
 * 
 */

void mt_test0_setup() {
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.triggerAngle = -197;
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_SINGLE; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 0; //TODO: What value to use and test with? //TODO: This should be irrelevant to the decoder and be removed from here
  configPage10.vvt2Enabled = 0; //TODO: Enable VVT testing
  currentStatus.crankRPM = 200; //TODO: What value to use and test with?
  configPage4.triggerFilter = 0; //TODO: Test filters

  configPage4.sparkMode = IGN_MODE_WASTED;
  configPage2.injLayout = INJ_PAIRED;
  CRANK_ANGLE_MAX = CRANK_ANGLE_MAX_IGN = 360;

  // These are not important but set them for clarity's sake
  configPage4.TrigEdge = 0;
  configPage4.TrigEdgeSec = 0;
  configPage10.TrigEdgeThrd = 0;
}

const testParams unsynced_0loss[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
};
const testParams unsynced_2loss[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 2 },
};
// TODO: Separate calculated tests for shared usage
const testParams mt_test0_sync[] = {
  { testParams::SYNC, 1 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
  { testParams::RPM, 5000, 20 },
  { testParams::REVCOUNT_c, 0 },
  { testParams::REVTIME_c, 0, 52 },
  { testParams::TOOTHANGLECORRECT, 1 },
  { testParams::TOOTHANGLE_c, 0 },
  { testParams::LASTTOOTHTIME_c, 0, 4 },
  { testParams::LASTTOOTHTIMEMINUSONE_c, 0, 4 },
  { testParams::CRANKANGLE_c, 0, 1 },
};

const testParams mt_test0_syncloss1[] = {
  { testParams::SYNC, 1 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 1 },
  { testParams::RPM, 5000, 20 },
  { testParams::REVCOUNT_c, 0 },
  { testParams::REVTIME_c, 0, 52 },
  { testParams::TOOTHANGLECORRECT, 1 },
  { testParams::TOOTHANGLE_c, 0 },
  { testParams::LASTTOOTHTIME_c, 0, 4 },
  { testParams::LASTTOOTHTIMEMINUSONE_c, 0, 4 },
  { testParams::CRANKANGLE_c, 0, 1 },
};

const testTooth mt_t0_t1  { 0, 30 };
const testTooth mt_t0_t2  { 30, 30 };
const testTooth mt_t0_t3  { 60, 30 };
const testTooth mt_t0_t4  { 90, 30 };
const testTooth mt_t0_t5  { 120, 30 };
const testTooth mt_t0_t6  { 150, 30 };
const testTooth mt_t0_t7  { 180, 30 };
const testTooth mt_t0_t8  { 210, 30 };
const testTooth mt_t0_t9  { 240, 30 };
const testTooth mt_t0_t10 { 270, 30 };
const testTooth mt_t0_t11 { 300, 60 };

//TODO: a stalling test to verify all parameters are reset
//TODO: Cranking tests
//TODO: Test for strong acceleration / deceleration
timedEvent mt_test0_events[] {
  { timedEvent::PRITRIG, 1000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t1 },
  { timedEvent::PRITRIG, 2000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t2 },
  { timedEvent::PRITRIG, 3000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t3 },
  { timedEvent::PRITRIG, 4000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t4 },
  { timedEvent::PRITRIG, 5000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t5 },
  { timedEvent::PRITRIG, 6000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t6 },
  { timedEvent::PRITRIG, 7000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t7 },
  { timedEvent::PRITRIG, 8000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t8 },
  { timedEvent::PRITRIG, 9000,  timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t9 },
  { timedEvent::PRITRIG, 10000, timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t10 },
  { timedEvent::PRITRIG, 11000, timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t11 },
  { timedEvent::PRITRIG, 13000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t1 },
  { timedEvent::PRITRIG, 14000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t2 },
  { timedEvent::PRITRIG, 15000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t3 },
  { timedEvent::PRITRIG, 16000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t4 },
  { timedEvent::PRITRIG, 17000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t5 },
  { timedEvent::PRITRIG, 18000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t6 },
  { timedEvent::PRITRIG, 19000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t7 },
  { timedEvent::PRITRIG, 20000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t8 },
  { timedEvent::PRITRIG, 21000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t9 },
  { timedEvent::PRITRIG, 22000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t10 },
  { timedEvent::PRITRIG, 23000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t11 },
  { timedEvent::PRITRIG, 25000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t1 },
  { timedEvent::PRITRIG, 26000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t2 },
  { timedEvent::PRITRIG, 27000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t3 },
  { timedEvent::PRITRIG, 28000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t4 },
  { timedEvent::PRITRIG, 29000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t5 },
  { timedEvent::PRITRIG, 30000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t6 },
  { timedEvent::PRITRIG, 31000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t7 },
  { timedEvent::PRITRIG, 32000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t8 },
  { timedEvent::PRITRIG, 33000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t9 },
  { timedEvent::PRITRIG, 34000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t10 },
  { timedEvent::PRITRIG, 35000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t11 },
  { timedEvent::PRITRIG, 37000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t1 },
  { timedEvent::STALL,   150000, timedEventArrayTestEntry(unsynced_0loss) },
  { timedEvent::PRITRIG, 151000, timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t9 },
  { timedEvent::PRITRIG, 152000, timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t10 },
  { timedEvent::PRITRIG, 153000, timedEventArrayTestEntry(unsynced_0loss), &mt_t0_t11 },
  { timedEvent::PRITRIG, 155000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t1 },
  { timedEvent::PRITRIG, 156000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t2 },
  { timedEvent::PRITRIG, 157000, timedEventArrayTestEntry(mt_test0_sync), &mt_t0_t3 },
//  { timedEvent::PRITRIG, 158000, timedEventArrayTestEntry(mt_test0_sync), &mt_t4 }, // Simulated lost trigger
  { timedEvent::PRITRIG, 159000, &mt_t0_t5 }, // TODO: Invalid input after tooth before, undefined behaviour or should it fail because of long input, or should it detect missed tooth and increment in the background?
  { timedEvent::PRITRIG, 160000, &mt_t0_t6 },
  { timedEvent::PRITRIG, 161000, &mt_t0_t7 },
  { timedEvent::PRITRIG, 162000, &mt_t0_t8 },
  { timedEvent::PRITRIG, 163000, &mt_t0_t9 },
  { timedEvent::PRITRIG, 164000, &mt_t0_t10 },
  { timedEvent::PRITRIG, 165000, &mt_t0_t11 }, 
  { timedEvent::PRITRIG, 167000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t1 }, // Lost trigger should be detected here
  { timedEvent::PRITRIG, 168000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t2 },
  { timedEvent::PRITRIG, 169000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t3 },
  { timedEvent::PRITRIG, 170000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t4 },
  { timedEvent::PRITRIG, 171000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t5 },
  { timedEvent::PRITRIG, 172000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t6 },
  { timedEvent::PRITRIG, 173000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t7 },
  { timedEvent::PRITRIG, 174000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t8 },
  { timedEvent::PRITRIG, 174400 }, // Extra fake signal
  { timedEvent::PRITRIG, 175000, &mt_t0_t9 }, //Invalid input here so don't test anything more
  { timedEvent::PRITRIG, 176000, &mt_t0_t10 },
  { timedEvent::PRITRIG, 177000, timedEventArrayTestEntry(unsynced_2loss), &mt_t0_t11 }, // Extra trigger should be detected here (this is too short to be the missing tooth)
  { timedEvent::PRITRIG, 179000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t1 },
  { timedEvent::PRITRIG, 180000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t2 },
  { timedEvent::PRITRIG, 181000, timedEventArrayTestEntry(mt_test0_syncloss1), &mt_t0_t3 },
};

// TODO: What to do at sync loss?
// TODO: Require tooth for trigger events

void mt_test1_setup() {
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.triggerAngle = 243;
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_SINGLE; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 0; //TODO: What value to use and test with? //TODO: This should be irrelevant to the decoder and be removed from here
  configPage10.vvt2Enabled = 0; //TODO: Enable VVT testing
  currentStatus.crankRPM = 200; //TODO: What value to use and test with?
  configPage4.triggerFilter = 0; //TODO: Test filters

  configPage4.sparkMode = IGN_MODE_WASTED;
  configPage2.injLayout = INJ_PAIRED;
  CRANK_ANGLE_MAX = CRANK_ANGLE_MAX_IGN = 360;

  // These are not important but set them for clarity's sake
  configPage4.TrigEdge = 0;
  configPage4.TrigEdgeSec = 0;
  configPage10.TrigEdgeThrd = 0;
}

decodingTest decodingTests[] = {
  { "Missing tooth test 0, 12-1 wasted spark", mt_test0_setup, mt_test0_events, countof(mt_test0_events) },
};

void testDecodingMissingTooth() {
  for (auto testData : decodingTests) {
    testData.execute();
    testData.showTriggerlog();
  }
}