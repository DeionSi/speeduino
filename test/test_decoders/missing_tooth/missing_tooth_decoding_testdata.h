#include "arduino.h"
#include "decoders.h"


void decodingTest0setup() {
  configPage4.TrigPattern = DECODER_MISSING_TOOTH; //TODO: Use different values
  configPage4.triggerTeeth = 12; //TODO: Use different values
  configPage4.triggerMissingTeeth = 1; //TODO: Use different values
  configPage4.TrigSpeed = CRANK_SPEED; //TODO: Use different values
  configPage4.trigPatternSec = SEC_TRIGGER_SINGLE; //TODO: Use different values
  configPage4.PollLevelPolarity = HIGH;
  configPage2.perToothIgn = false; //TODO: Test with this enabled, but the primary function of this tester is not to test new ignition mode
  configPage4.StgCycles = 1; //TODO: What value to use and test with?
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

struct decodingTest {
  const char *name;
  void (*decodingSetup)();

  //TODO: What are the expected decoder outputs?
  // sync, halfsync, synclosscount, revolutioncount, rpm, crankangle, MAX_STALL_TIME, toothCurrentCount, toothLastToothTime, 
  // toothLastMinusOneToothTime, toothOneTime, toothOneMinusOneTime, triggerToothAngle, triggerToothAngleIsCorrect, secondDerivEnabled
  const bool expectedSync;
  const bool expectedHalfSync;
  const byte expectedSyncLossCount;
  const byte expectedRevolutionCount;
};

const byte testCount = 1;
decodingTest decodingTests[testCount] = {
  { // Missing tooth 12-1
  .name = "Missing tooth 12-1, wasted spark",
  .decodingSetup = decodingTest0setup,
  .expectedSync = true,
  .expectedHalfSync = false,
  .expectedSyncLossCount = 0,
  .expectedRevolutionCount = 0,
  }
};