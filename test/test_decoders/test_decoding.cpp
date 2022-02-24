/* Decoder expected outputs
 *
 * These variables contain the sync status of the decoder.
 * currentStatus.hasSync - True if full sync is achieved (360 or 720 degree precision depending on settings).
 * BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC) - True if 360 degree precision is achieved when 720 is requested.
 * currentStatus.syncLossCounter - Increases by one if sync-state degrades (except if stalltime is exceeded). Is never reset (only wraps itself on overflow).
 *
 * These variables/functions must all contain/return valid values if in sync or half-sync. If losing all sync these should all be reset.
 * currentStatus.startRevolutions - Increases by one for every completed crank revolution (360 crank degrees) after gaining sync. TODO resets at sync loss??
 * toothLastToothTime - the microsecond timestamp of the last tooth of the primary? trigger. TODO resets at sync loss?? TODO should be replaced by lastGap.
 * toothLastMinusOneToothTime - the microsecond timestamp of the tooth before the last tooth of the primary? trigger. TODO resets at sync loss?? TODO should be replaced by lastGap.
 * triggerToothAngle - The angle in degrees between the last two teeth.
 * revolutionTime - The time in microseconds for a 360 crank revolution.
 * getRPM() - Returns the revolutions per minute
 * getCrankAngle() - Returns the crankangle. Over 720 degrees if in sequential mode and in half-sync. Over 360 degrees otherwise.
 * 
 * --------------
 * triggerToothAngleIsCorrect - Must always be true, this variable should be removed after the decoders have been updated
 * MAX_STALL_TIME - Must always be correct if in sync. Should be set so an actual RPM of less than 50 causes stall. //TODO: actual rpm for stall, maybe should be configurable? calculate in speeduino, not as part of decoder
 * 
 * TODO: specify which variable is normally set where
 */

#include "arduino.h"
#include "decoders.h"
#include "crankMaths.h"
#include "unity.h"
#include "init.h"
#include "test_decoding.h"

const bool individual_test_reports = true; // Shows each test output rather than one per event
const bool individual_test_reports_debug = true; // Shows delta/expected/result for each individual test

// Test output variables
const byte unityMessageLength = 100;
char unityMessage[unityMessageLength];

/************ This file is sorted into 3 parts ************
 * Part 1: Preparation
 * Part 2: Gathering the results
 * Part 3: Running the tests (comparing results and giving output)
 * Part 4: Cleanup / other
 */

//************ Part 1: Preparation ************

decodingTest::decodingTest(const char* const a_name, void (*const a_decoderSetup)(), timedEvent* const a_events, const byte a_eventCount) : name(a_name), decoderSetup(a_decoderSetup), events(a_events), eventCount(a_eventCount) { }

void decodingTest::execute() {


  Unity.CurrentTestName = NULL; // Don't get the previous test name on this INFO message
  UnityMessage(name, __LINE__);

  if ( verifyEventOrder() ) { // Part 1
    decodingSetup(); // Part 1
    gatherResults(); // Part 2
    compareResults(); // Part 3
    resetTest(); // Part 4
  }
}

bool decodingTest::verifyEventOrder() const {
  bool result = true;
  uint32_t lastEventTime = 0;
  for (int i = 0; i < eventCount; i++) {
    if (events[i].time < lastEventTime) {
      snprintf(unityMessage, unityMessageLength, "ERROR: Out of sequence event %lu i %d", events[i].time, i);
      UnityMessage(unityMessage, __LINE__);
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

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, testResults* const a_results, const testTooth* const a_tooth) :
testCount(a_testCount), results(a_results), type(a_type), time(a_time), tests(a_tests), tooth(a_tooth) { };

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, testResults* const a_results) :
testCount(a_testCount), results(a_results), type(a_type), time(a_time), tests(a_tests) { };

testParams::testParams(const timedTestType a_type, const uint32_t a_expected) : type(a_type), expected(a_expected) {};
testParams::testParams(const timedTestType a_type, const uint32_t a_expected, const uint16_t a_delta) : type(a_type), expected(a_expected), delta(a_delta) {};

const char* const testParams::friendlyNames[] = {
  [SYNC] = "Sync",
  [HALFSYNC] = "Half-sync",
  [SYNCLOSSCOUNT] = "Sync-loss count",
  [REVCOUNT] = "Revolution count",
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

//************ Part 2: Gathering the results ************

void decodingTest::gatherResults() {
  startTime = micros();
  for (int i = 0; i < eventCount; i++) {
    events[i].trigger(startTime);
  }
}

void timedEvent::trigger(uint32_t testStartTime) {
  // Delay until it's time to trigger
  if (time < UINT32_MAX) { // UINT32_MAX Entries are not delayed
    uint32_t delayUntil = time + testStartTime;
    while(micros() < delayUntil) {};
  }

  triggeredAt = micros();

  // Execute any actions
  if (type == PRITRIG) {
    triggerHandler();
  }

  // Get data from tests
  if (tests != nullptr) {
    preTestsCommands();
    for (int i = 0; i < testCount; i++) {
      results[i].retrievedAt = micros();
      results[i].value = tests[i].getResult();
    }
  }
};

void timedEvent::preTestsCommands() {
  currentStatus.RPM = getRPM();
  doCrankSpeedCalcs();
}

uint32_t testParams::getResult() const {
  uint32_t result = 0;

  switch(type) {
    case CRANKANGLE_c:
      result = getCrankAngle();
      break;
    case RPM:
    case RPM_c_deltaPerThousand:
      result = getRPM();
      break;
    case SYNC:
      result = currentStatus.hasSync;
      break;
    case HALFSYNC:
      result = BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC);
      break;
    case SYNCLOSSCOUNT:
      result = currentStatus.syncLossCounter;
      break;
    case REVCOUNT:
      result = currentStatus.startRevolutions;
      break;
    case REVTIME_c:
      result = revolutionTime;
      break;
    case TOOTHANGLECORRECT:
      result = triggerToothAngleIsCorrect;
      break;
    case TOOTHANGLE_c:
      result = triggerToothAngle;
      break;
    case LASTTOOTHTIME_c:
      result = toothLastToothTime;
      break;
    case LASTTOOTHTIMEMINUSONE_c:
      result = toothLastMinusOneToothTime;
      break;
    case STALLTIME_c:
      result = MAX_STALL_TIME;
      break;
    default:
      break;
  }

  return result;
}

//************ Part 3: Running the tests (comparing results and giving output) ************

// For keeping track of current state
testResults* timedEvent::wrapperResult;
const testParams* timedEvent::wrapperTest;
timedEvent* decodingTest::wrapperEvent;
decodingTest* decodingTest::wrapperDecodingTest;

// Used to keep track of variables for calculated fields
timedEvent* lastPRITRIGevent;
uint32_t testLastToothTime;
uint32_t testLastToothMinusOneTime;
float testLastUsPerDegree;
uint16_t testLastToothDegrees;
uint32_t testToothOneTime;
uint32_t testRevolutionTime;

void decodingTest::compareResults() {
  for (int i = 0; i < eventCount; i++) {

    if (events[i].type == timedEvent::PRITRIG) {
      testLastToothMinusOneTime = testLastToothTime;
      testLastToothTime = events[i].triggeredAt;

      if (events[i].tooth->angle == 0) {
        if (testToothOneTime > 0) {
          testRevolutionTime = events[i].triggeredAt - testToothOneTime;
        }
        testToothOneTime = events[i].triggeredAt;
      }

      if (lastPRITRIGevent != nullptr) {
        uint32_t interval = events[i].triggeredAt - lastPRITRIGevent->triggeredAt;
        testLastUsPerDegree = (float)interval / (float)lastPRITRIGevent->tooth->degrees;

        testLastToothDegrees = lastPRITRIGevent->tooth->degrees;

      }

      lastPRITRIGevent = &events[i];
    }

    if (events[i].tests != nullptr) {
      

      if (individual_test_reports) {
        events[i].runTests(startTime);
      }
      else {
        wrapperEvent = &events[i];
        wrapperDecodingTest = this;
        snprintf(unityMessage, unityMessageLength, "%lu triggered at %lu", events[i].time, events[i].triggeredAt - startTime);
        UnityDefaultTestRun(events[i].runTestsWrapper, unityMessage, __LINE__);
      }
      
    }
    
  }
}

// This is needed because Unity tests cannot call non-static member functions or use arguments
void timedEvent::runTestsWrapper() {
  decodingTest::wrapperEvent->runTests(decodingTest::wrapperDecodingTest->startTime);
}

void timedEvent::runTests(uint32_t testStartTime) {
  if (tests != nullptr) {
    for (int i = 0; i < testCount; i++) {

      if (individual_test_reports) {
        wrapperTest = &tests[i];
        wrapperResult = &results[i];
        snprintf(unityMessage, unityMessageLength, "%lu retrieved at %lu '%s'", time, results[i].retrievedAt - testStartTime, tests[i].name() );
        UnityDefaultTestRun(tests[i].runTestWrapper, unityMessage, __LINE__);
      }
      else {
        tests[i].runTest(&results[i]);
      }

    }
  }
}

// This is needed because Unity tests cannot call non-static member functions or use arguments
void testParams::runTestWrapper() {
  timedEvent::wrapperTest->runTest(timedEvent::wrapperResult);
}

void testParams::runTest(testResults* result) const {

  uint32_t expectedCalculated = expected;
  uint16_t deltaCalculated = delta;

  // Calculate our crank angle compare angle based on how much time passed until test
  switch(type) {
    case CRANKANGLE_c:
      if (testLastUsPerDegree > 0 && lastPRITRIGevent != nullptr) {
        uint32_t delay = result->retrievedAt - lastPRITRIGevent->triggeredAt;
        expectedCalculated = lastPRITRIGevent->tooth->angle + ((float)delay / testLastUsPerDegree);
      }
      break;
    case LASTTOOTHTIME_c:
      expectedCalculated = testLastToothTime + 4; // Add 4 because trigger function is called after this time was saved and micros() rounds to 4
      break;
    case LASTTOOTHTIMEMINUSONE_c:
      expectedCalculated = testLastToothMinusOneTime + 4; // Add 4 because trigger function is called after this time was saved and micros() rounds to 4
      break;
    case REVTIME_c:
      expectedCalculated = testRevolutionTime;
      break;
    case TOOTHANGLE_c:
      expectedCalculated = testLastToothDegrees;
      break;
    case STALLTIME_c:
      if (lastPRITRIGevent != nullptr) {
        expectedCalculated = lastPRITRIGevent->tooth->degrees * 3333UL; // 3333,33 microseconds per degree at 50 revolutions per minute
      }
      break;
    case RPM_c_deltaPerThousand:
      if (testLastToothMinusOneTime > 0 && testLastToothTime > 0) {
        uint32_t delay = testLastToothTime - testLastToothMinusOneTime;
        expectedCalculated = (60000000.0f / delay) / ( 360.0f / testLastToothDegrees ); // This converts delay in microseconds and degrees of rotation to revolutions per minute
        deltaCalculated = deltaCalculated * expectedCalculated / 1000;
      }
      break;
    default:
      break;
  }

  if (individual_test_reports_debug) {
    const char testMessageLength = 40;
    char testMessage[testMessageLength];
    snprintf(testMessage, testMessageLength, "result %lu expected %lu delta %u ", result->value, expectedCalculated, deltaCalculated);
    UnityPrint(testMessage);
    for (int i = strlen(testMessage); i < testMessageLength+1; i++) { UnityPrint(" "); } //Padding
  }

  if (deltaCalculated == 0) {
    TEST_ASSERT_EQUAL_MESSAGE(expectedCalculated, result->value, name());
  }
  else {
    TEST_ASSERT_INT_WITHIN_MESSAGE(deltaCalculated, expectedCalculated, result->value, name());
  }
}

//************ Part 4: Cleanup / other ************

void decodingTest::resetTest() {
  // Resets the testing global variables
  testLastToothTime = 0;
  testLastToothMinusOneTime = 0;
  testLastUsPerDegree = 0;
  testLastToothDegrees = 0;
  testToothOneTime = 0;
  testRevolutionTime = 0;
  lastPRITRIGevent = nullptr;

  stallCleanup(); // Resets Speeduinos decoder variables
}

void decodingTest::stallCleanup() {
  //TODO: Use a function in speeduino to do all of this

  //This is from Speeduinos main loop which checks for engine stall/turn off and resets a bunch decoder related values
  currentStatus.RPM = 0;
  toothLastToothTime = 0;
  toothLastSecToothTime = 0;
  currentStatus.hasSync = false;
  BIT_CLEAR(currentStatus.status3, BIT_STATUS3_HALFSYNC);
  currentStatus.startRevolutions = 0;
  toothSystemCount = 0;
  secondaryToothCount = 0;

  //TODO: These should be added to Speeduinos code
  /*toothLastMinusOneToothTime = 0;
  triggerToothAngle = 0;
  triggerToothAngleIsCorrect = false;
  revolutionTime = 0;
  MAX_STALL_TIME = 0;*/
}

void decodingTest::showTriggerlog() {
  // Show a little log of times
  uint32_t lastTriggerTime = 0;
  for (int i = 0; i < eventCount; i++) {
    uint32_t displayTime = events[i].triggeredAt - lastTriggerTime;
    lastTriggerTime = events[i].triggeredAt;

    snprintf(unityMessage, unityMessageLength, "time %lu timefromstart %lu interval %lu ", events[i].triggeredAt, events[i].triggeredAt - startTime, displayTime);
    UnityPrint(unityMessage);
    UNITY_PRINT_EOL();
  }
}