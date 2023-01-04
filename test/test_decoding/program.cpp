/* Decoder expected outputs
 *
 * These variables contain the sync status of the decoder.
 * currentStatus.hasSync - True if full sync is achieved (360 or 720 degree precision depending on settings).
 * BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC) - True if 360 degree precision is achieved when 720 is requested.
 * currentStatus.syncLossCounter - Increases by one if sync-state degrades (except if stalltime is exceeded). Is never reset (only wraps itself on overflow).
 *
 * These variables/functions must all contain/return valid values if in sync or half-sync.
 * currentStatus.startRevolutions - Increases by one for every completed crank revolution (360 crank degrees) after gaining sync.
 * toothLastToothTime - the microsecond timestamp of the last tooth of the primary? trigger.
 * toothLastMinusOneToothTime - the microsecond timestamp of the tooth before the last tooth of the primary? trigger. //TODO should be replaced by lastGap.
 * triggerToothAngle - The angle in degrees between the last two teeth.
 * triggerToothAngleIsCorrect - Must always be true //TODO: this variable should be removed after the decoders have been updated
 * revolutionTime - The time in microseconds for a 360 crank revolution.
 * getRPM() - Returns the revolutions per minute
 * getCrankAngle() - Returns the crankangle. Over 720 degrees if in sequential mode and in half-sync. Over 360 degrees otherwise.
 * getStallTime() - Returns the number of microseconds from the current tooth to the next at 50 rpm. //TODO: how to calculate (should stall time be updated depending on respective tooth length?)
 * isDecoderStalled() - Returns true if the decoder determines RPM is below 50 (by using the output from getStallTime()). //TODO: how to calculate
 * 
 * These variable are set and forget
 * MAX_STALL_TIME - Should be set so an actual RPM of 50 causes stall. Is only used internally by the decoder until sync is gained. //TODO: how to calculate
 * 
 *                                                        Resets at stall/syncloss/syncdegrade
 * currentStatus.hasSync                                  y y y
 * BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC) y y n
 * currentStatus.syncLossCounter                          y y y (increments)
 * currentStatus.startRevolutions                         y y n
 * toothLastToothTime                                     y y n
 * toothLastMinusOneToothTime                             y y n
 * triggerToothAngle                                      y y n
 * revolutionTime                                         y y n
 * MAX_STALL_TIME                                         n/a
 * triggerToothAngleIsCorrect                             y y n
 * 
 */

/************ This file is separated into parts ************
 * Part 1: Preparation
 * Part 2: Gathering the results
 * Part 3: Running the tests (comparing results and giving output)
 * Part 4: Cleanup / other
 */

#include "arduino.h"
#include "decoders.h"
#include "crankMaths.h"
#include "unity.h"
#include "init.h"
#include "program.h"

#ifndef UNIT_TEST
  extern unsigned long micros_injection; // Hack so that vscode sees the variable
#endif

// Output settings
const bool individual_test_reports = true; // Shows each test output rather than one per event
const bool individual_test_reports_debug = true; // Shows delta/expected/result for each individual test

// Test output variables
const byte unityMessageLength = 100;
char unityMessage[unityMessageLength];

//************ Part 1: Preparation ************

decodingTest::decodingTest(const char* const a_name, void (*const a_decoderSetup)(), timedEvent* const a_events, const byte a_eventCount) : name(a_name), decoderSetup(a_decoderSetup), events(a_events), eventCount(a_eventCount) { }

void decodingTest::execute() {


  Unity.CurrentTestName = NULL; // Don't get the previous test name on this INFO message
  UnityMessage(name, __LINE__);

  if ( verifyEventOrder() ) { // Part 1
    decodingSetup(); // Part 1
    gatherResults(); // Part 2
    resetTest(); // Part 4
    stallCleanup(); // Reset speeduino
  }
}

bool decodingTest::verifyEventOrder() const {
  bool result = true;
  uint32_t lastEventTime = 0;
  for (int i = 0; i < eventCount; i++) {
    if (events[i].time < lastEventTime) {
      snprintf(unityMessage, unityMessageLength, "Out of sequence event %lu i %d", events[i].time, i);
      TEST_FAIL_MESSAGE(unityMessage);
      result = false;
    }
    lastEventTime = events[i].time;
  }
  return result;
}

void decodingTest::decodingSetup() {
  decoderSetup();

  // Set pins because we need the trigger pin numbers, BoardID 3 (Speeduino v0.4 board) appears to have pin mappings for most variants
  setPinMapping(3);

  // initaliseTriggers attaches interrupts to trigger functions, so: disable, detach and reenable interrupts
  // Interrupts are needed for micros() function
  noInterrupts();
  initialiseTriggers();
  detachInterrupt( digitalPinToInterrupt(pinTrigger) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger2) );
  detachInterrupt( digitalPinToInterrupt(pinTrigger3) );
  interrupts();
}

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testGroup* const a_tests, const testTooth* const a_tooth) :
type(a_type), time(a_time), tests(a_tests), tooth(a_tooth) { };

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testGroup* const a_tests) :
type(a_type), time(a_time), tests(a_tests) { };

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testTooth* const a_tooth) :
type(a_type), time(a_time), tooth(a_tooth) { };

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time) :
type(a_type), time(a_time) { };

testParams::testParams(const timedTestType a_type, const int32_t a_expected) : type(a_type), expected(a_expected) {};
testParams::testParams(const timedTestType a_type, const int32_t a_expected, const uint16_t a_delta) : type(a_type), expected(a_expected), delta(a_delta) {};

const char* const testParams::friendlyNames[] = {
  [SYNC] = "Sync",
  [HALFSYNC] = "Half-sync",
  [SYNCLOSSCOUNT] = "Sync-loss count",
  [REVCOUNT_c] = "Revolution count",
  [REVTIME_c] = "Revolution time",
  [TOOTHANGLECORRECT] = "Tooth angle is correct",
  [TOOTHANGLE_c] = "Tooth angle",
  [LASTTOOTHTIME_c] = "Last tooth time",
  [LASTTOOTHTIMEMINUSONE_c] = "Last tooth time minus one",
  [RPM] = "RPM",
  [RPM_c_deltaPerThousand] = "Calculated RPM",
  [STALLTIME_c] = "Stall time",
  [CRANKANGLE_c] = "Crank angle",
  [ENUMEND] = "enum end / invalid",
};

const char* testParams::name() const {
  return friendlyNames[this->type];
}

//************ Part 2: Running tests ************

// For keeping track of current state
const testParams* timedEvent::wrapperTest;
timedEvent* timedEvent::wrapperEvent;
decodingTest* timedEvent::wrapperDecodingTest;

// Used to keep track of variables for calculated fields
timedEvent* lastPRITRIGevent;
uint32_t testLastToothTime;
uint32_t testLastToothMinusOneTime;
float testLastUsPerDegree;
uint16_t testLastToothDegrees;
uint16_t testLastRPM;
uint32_t testToothOneTime;
uint32_t testToothOneMinusOneTime;
uint32_t testRevolutionTime;
byte testRevolutionCount;
bool testHasSyncOrHalfsync;

void decodingTest::gatherResults() {
  for (int i = 0; i < eventCount; i++) {
    events[i].trigger(this);
  }
}

void timedEvent::trigger(decodingTest* currentDecodingTest) {

  micros_injection = time;

  // Execute any actions
  switch(type) {
    case PRITRIG:
      triggerHandler();
      break;

    case STALL:
      currentDecodingTest->stallCleanup();
      break;

    case TEST:
    default:
      break; // do nothing special
    
  }

  // Calculate expected values
  calculateExpected();

  // Get run tests
  if (tests != nullptr) {
    preTestsCommands();

    if (individual_test_reports) {
      runTests();
    }
    else {
      wrapperEvent = this;
      wrapperDecodingTest = currentDecodingTest;
      snprintf(unityMessage, unityMessageLength, "%lu", time);
      UnityDefaultTestRun(runTestsWrapper, unityMessage, __LINE__);
    }
  }

}

void timedEvent::calculateExpected() {
  if (type == timedEvent::PRITRIG && tooth != nullptr) {
    // Calculate expected tooth times, tooth one times and revolution count
    if (tooth->angle == 0 && testLastToothTime > 0) { // If this is the first tooth ever, don't count it as the decoder will not have been able to identify it
      testToothOneMinusOneTime = testToothOneTime;
      testToothOneTime = time;

      if (testToothOneMinusOneTime > 0) {
        testRevolutionCount++;
      }
    }
    
    testLastToothMinusOneTime = testLastToothTime;
    testLastToothTime = time;
    
    // Calculate expected revolution time and RPM
    if (testToothOneMinusOneTime > 0 && testLastRPM > currentStatus.crankRPM) {
      testRevolutionTime = testToothOneTime - testToothOneMinusOneTime;
    }
    else if (lastPRITRIGevent != nullptr && testLastToothMinusOneTime > 0) {
      uint32_t testTriggerToothAngleTime = testLastToothTime - testLastToothMinusOneTime;
      testRevolutionTime = testTriggerToothAngleTime * (CRANK_ANGLE_MAX / lastPRITRIGevent->tooth->degrees);
    }
    testLastRPM = 60000000 / testRevolutionTime;

    // Calculate expected micros per degree and last tooth degrees
    if (lastPRITRIGevent != nullptr) {
      uint32_t interval = time - lastPRITRIGevent->time;
      testLastUsPerDegree = (float)interval / (float)lastPRITRIGevent->tooth->degrees;

      testLastToothDegrees = lastPRITRIGevent->tooth->degrees;
    }

    lastPRITRIGevent = this;
  }
  else if (type == timedEvent::STALL) {
    decodingTest::resetTest();
  }

}

void timedEvent::preTestsCommands() {
  currentStatus.RPM = getRPM();
  doCrankSpeedCalcs();
}

void decodingTest::stallCleanup() {
  resetDecoderState();
  currentStatus.RPM = 0; // Extra variable which is not owned by the decoder but is used by the decoder // TODO: Make the decoder own this variable?
  decodingSetup();
}

//************ Part 3: Running the tests (comparing results and giving output) ************



// This is needed because Unity tests cannot call non-static member functions or use arguments
void timedEvent::runTestsWrapper() {
  timedEvent::wrapperEvent->runTests();
}

void timedEvent::runTests() {

  testHasSyncOrHalfsync = hasSyncOrHalfsync();

  if (tests != nullptr) {
    for (int i = 0; i < tests->testCount; i++) {

      if (individual_test_reports) {
        wrapperTest = &tests->tests[i];

        snprintf(unityMessage, unityMessageLength, "%lu '%s'", time, tests->tests[i].name() );
        UnityDefaultTestRun(tests->tests[i].runTestWrapper, unityMessage, __LINE__);
      }
      else {
        tests->tests[i].runTest();
      }

    }
  }
}

bool timedEvent::hasSyncOrHalfsync() {
  bool result = false;
  if (tests != nullptr) {
    for (int i = 0; i < tests->testCount; i++) {

      result = tests->tests[i].hasSyncOrHalfsync();
      if (result == true) { break; }
      
    }
  }
  return result;
}

bool testParams::hasSyncOrHalfsync() const {
  if (type == SYNC || type == HALFSYNC) {
    if (expected == 1) {
      return true;
    }
  }
  return false;
}

// This is needed because Unity tests cannot call non-static member functions or use arguments
void testParams::runTestWrapper() {
  timedEvent::wrapperTest->runTest();
}

void testParams::runTest() const {

  int32_t actual = 0;
  int32_t expectedCalculated = expected;
  uint16_t deltaCalculated = delta;

  // Calculate our crank angle compare angle based on how much time passed until test
  switch(type) {
    case CRANKANGLE_c:
      if (testLastUsPerDegree > 0 && lastPRITRIGevent != nullptr) {
        uint32_t delay = micros_injection - lastPRITRIGevent->time;
        expectedCalculated = lastPRITRIGevent->tooth->angle + ((float)delay / testLastUsPerDegree);
        expectedCalculated += configPage4.triggerAngle;
        while (expectedCalculated >= CRANK_ANGLE_MAX) { expectedCalculated -= CRANK_ANGLE_MAX; }
        while (expectedCalculated < 0) { expectedCalculated += CRANK_ANGLE_MAX; }
      }
      actual = getCrankAngle();
      break;

    case RPM:
      actual = getRPM();
      break;

    case RPM_c_deltaPerThousand:
      expectedCalculated = testLastRPM;
      actual = getRPM();
      break;

    case SYNC:
      actual = currentStatus.hasSync;
      break;

    case HALFSYNC:
      actual = BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC);
      break;

    case SYNCLOSSCOUNT:
      actual = currentStatus.syncLossCounter;
      break;

    case REVCOUNT_c:
      expectedCalculated = testRevolutionCount;
      actual = currentStatus.startRevolutions;
      break;

    case REVTIME_c:
      expectedCalculated = testRevolutionTime;
      actual = revolutionTime;
      break;

    case TOOTHANGLECORRECT:
      actual = BIT_CHECK(decoderState, BIT_DECODER_TOOTH_ANG_CORRECT);
      break;

    case TOOTHANGLE_c:
      expectedCalculated = testLastToothDegrees;
      actual = triggerToothAngle;
      break;

    case LASTTOOTHTIME_c:
      expectedCalculated = testLastToothTime + 4; // Add 4 because trigger function is called after this time was saved and micros() rounds to 4
      actual = toothLastToothTime;
      break;

    case LASTTOOTHTIMEMINUSONE_c:
      expectedCalculated = testLastToothMinusOneTime + 4; // Add 4 because trigger function is called after this time was saved and micros() rounds to 4
      actual = toothLastMinusOneToothTime;
      break;
      
    case STALLTIME_c:
      /* TODO: how to calculate
        if (testHasSyncOrHalfsync == true) {
        if (lastPRITRIGevent != nullptr) {
          expectedCalculated = lastPRITRIGevent->tooth->degrees * 3333UL; // 3333,33 microseconds per degree at 50 revolutions per minute
        }
      }
      else {
        expectedCalculated = MAX_STALL_TIME; //TODO: Change this to calculate max stall time from maximum tooth length
      } */
      // result = getStallTime(); //TODO: how to calculate
      break;
    default:
      break;
  }

  if (individual_test_reports_debug) {
    const char testMessageLength = 40;
    char testMessage[testMessageLength];
    snprintf(testMessage, testMessageLength, "actual %ld expected %ld delta %u ", actual, expectedCalculated, deltaCalculated);
    UnityPrint(testMessage);
    for (int i = strlen(testMessage); i < testMessageLength+1; i++) { UnityPrint(" "); } //Padding
  }

  if (deltaCalculated == 0) {
    TEST_ASSERT_EQUAL_MESSAGE(expectedCalculated, actual, name());
  }
  else {
    TEST_ASSERT_INT_WITHIN_MESSAGE(deltaCalculated, expectedCalculated, actual, name());
  }
}

//************ Part 4: Cleanup / other ************

void decodingTest::resetTest() {
  // Resets the testing global variables
  testLastToothTime = 0;
  testLastToothMinusOneTime = 0;
  testLastUsPerDegree = 0;
  testLastToothDegrees = 0;
  testLastRPM = 0;
  testToothOneTime = 0;
  testToothOneMinusOneTime = 0;
  testRevolutionTime = 0;
  testRevolutionCount = 0;
  testHasSyncOrHalfsync = false;
  lastPRITRIGevent = nullptr;
}

void decodingTest::showTriggerlog() {
  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (int i = 0; i < eventCount; i++) {
    uint32_t displayTime = events[i].time - lastTriggerTime;
    lastTriggerTime = events[i].time;

    snprintf(unityMessage, unityMessageLength, "time %lu interval %lu ", events[i].time, displayTime);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();
  }
}