#include "arduino.h"
#include "decoders.h"
#include "crankMaths.h"
#include "unity.h"
#include "init.h"
#include "test_decoding.h"

const bool individual_test_reports = true; // Shows each test output rather than one per event
const bool individual_test_reports_debug = true; // Shows delta/expected/result for each individual test

// Global variables

const byte unityMessageLength = 100;
char unityMessage[unityMessageLength];

testResults *currentResult;
timedEvent *currentEvent;
timedEvent *lastPRITRIGevent;
const testParams* currentTest;
decodingTest* currentDecodingTest;

// testParams

testParams::testParams(const timedTestType a_type, const uint32_t a_expected) : type(a_type), expected(a_expected) {};
testParams::testParams(const timedTestType a_type, const uint32_t a_expected, const uint16_t a_delta) : type(a_type), expected(a_expected), delta(a_delta) {};

const char* const testParams::friendlyNames[] = {
  [SYNC] = "Sync",
  [HALFSYNC] = "Half-sync",
  [SYNCLOSSCOUNT] = "Sync-loss count",
  [REVCOUNT] = "Revolution count",
  [TOOTHANGLECORRECT] = "Tooth angle is correct",
  [TOOTHANGLE] = "Tooth angle",
  [LASTTOOTHTIME] = "Last tooth time",
  [LASTTOOTHTIMEMINUSONE] = "Last tooth time minus one",
  [RPM] = "RPM",
  [STALLTIME] = "Stall time",
  [CRANKANGLE] = "Crank angle",
  [ENUMEND] = "enum end / invalid",
};

uint32_t testParams::getResult() const {
  uint32_t result = 0;

  switch(type) {
    case CRANKANGLE:
      result = getCrankAngle();
      break;
    case RPM:
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
    case TOOTHANGLECORRECT:
      result = triggerToothAngleIsCorrect;
      break;
    case TOOTHANGLE:
      result = triggerToothAngle;
      break;
    case LASTTOOTHTIME:
      result = toothLastToothTime;
      break;
    case LASTTOOTHTIMEMINUSONE:
      result = toothLastMinusOneToothTime;
      break;
    case STALLTIME:
      result = MAX_STALL_TIME;
      break;
    default:
      break;
  }

  return result;
}

void testParams::runTest() {

  uint32_t expected = currentTest->expected;
//TODO: maybe move some static variables out of classes?
  // Calculate our crank angle compare angle based on how much time passed until test
  if (currentTest->type == testParams::CRANKANGLE && decodingTest::testLastUsPerDegree > 0 && lastPRITRIGevent != nullptr) {
    uint32_t delay = currentResult->retrievedAt - lastPRITRIGevent->triggeredAt;
    expected = lastPRITRIGevent->tooth->angle + ((float)delay / decodingTest::testLastUsPerDegree);
  }
  else if (currentTest->type == testParams::LASTTOOTHTIME) {
    expected = currentDecodingTest->testLastToothTime + 4; // Add 4 because trigger function is called after this time was saved
  }
  else if (currentTest->type == testParams::LASTTOOTHTIMEMINUSONE) {
    expected = currentDecodingTest->testLastToothMinusOneTime + 4; // Add 4 because trigger function is called after this time was saved
  }
  else if (currentTest->type == testParams::TOOTHANGLE && decodingTest::testLastToothDegrees > 0) {
    expected = decodingTest::testLastToothDegrees;
  }

  if (individual_test_reports_debug) {
    const char testMessageLength = 40;
    char testMessage[testMessageLength];
    snprintf(testMessage, testMessageLength, "result %lu expected %lu delta %u ", currentResult->value, expected, currentTest->delta);
    UnityPrint(testMessage);
    for (int i = strlen(testMessage); i < testMessageLength+1; i++) { UnityPrint(" "); } //Padding
  }

  if (currentTest->delta == 0) {
    TEST_ASSERT_EQUAL_MESSAGE(expected, currentResult->value, currentTest->name());
  }
  else {
    TEST_ASSERT_INT_WITHIN_MESSAGE(currentTest->delta, expected, currentResult->value, currentTest->name());
  }
}

const char* testParams::name() const {
  return friendlyNames[this->type];
}

// timedEvent

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, testResults* const a_results, const testTooth* const a_tooth) :
testCount(a_testCount), results(a_results), type(a_type), time(a_time), tests(a_tests), tooth(a_tooth) { };

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, testResults* const a_results) :
testCount(a_testCount), results(a_results), type(a_type), time(a_time), tests(a_tests) { };

void timedEvent::trigger() {
  // Delay until it's time to trigger
  if (time < UINT32_MAX) { // UINT32_MAX Entries are not delayed
    delayUntil(time + currentDecodingTest->startTime);
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

void timedEvent::delayUntil(uint32_t time) {
  while(micros() < time) {};
}

void timedEvent::runTests() {
  if (currentEvent->tests != nullptr) {
    for (int i = 0; i < currentEvent->testCount; i++) {
      currentTest = &currentEvent->tests[i];
      currentResult = &currentEvent->results[i];

      if (individual_test_reports) {
        snprintf(unityMessage, unityMessageLength, "%lu retrieved at %lu '%s'", currentEvent->time, currentResult->retrievedAt - currentDecodingTest->startTime, currentEvent->tests[i].name() );
        UnityDefaultTestRun(currentEvent->tests[i].runTest, unityMessage, __LINE__);
      }
      else {
        currentEvent->tests[i].runTest();
      }

    }
  }
}

// decodingTest

decodingTest::decodingTest(const char* const a_name, void (*const a_decoderSetup)(), timedEvent* const a_events, const byte a_eventCount) : name(a_name), decoderSetup(a_decoderSetup), events(a_events), eventCount(a_eventCount) { }

uint32_t decodingTest::startTime;
uint32_t decodingTest::testLastToothTime;
uint32_t decodingTest::testLastToothMinusOneTime;
float decodingTest::testLastUsPerDegree;
float decodingTest::testLastToothDegrees;

void decodingTest::execute() {
  // Reset globals for subsequent tests
  currentResult = nullptr;
  currentEvent = nullptr;
  lastPRITRIGevent = nullptr;
  currentTest = nullptr;
  testLastUsPerDegree = 0;
  currentDecodingTest = this;

  Unity.CurrentTestName = NULL; // Don't get the previous test name on this INFO message
  UnityMessage(name, __LINE__);

  if ( verifyEventOrder() ) {
    decodingSetup();
    gatherResults();
    compareResults();
    resetSpeeduino();
  }
}

bool decodingTest::verifyEventOrder() const {
  bool result = true;
  uint32_t lastEventTime = 0;
  for (int i; i < eventCount; i++) {
    if (events[i].time < lastEventTime) {
      snprintf(unityMessage, unityMessageLength, "ERROR: Out of sequence event %lu", events[i].time);
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

void decodingTest::gatherResults() {
  startTime = micros();
  for (int i = 0; i < eventCount; i++) {
    events[i].trigger();
  }
}

void decodingTest::compareResults() {
  for (int i = 0; i < eventCount; i++) {
    currentEvent = &events[i];

    if (events[i].type == timedEvent::PRITRIG) {
      testLastToothMinusOneTime = testLastToothTime;
      testLastToothTime = events[i].triggeredAt;

      if (lastPRITRIGevent != nullptr) {
        uint32_t interval = currentEvent->triggeredAt - lastPRITRIGevent->triggeredAt;
        testLastUsPerDegree = (float)interval / (float)lastPRITRIGevent->tooth->degrees;

        testLastToothDegrees = lastPRITRIGevent->tooth->degrees;

      }

      lastPRITRIGevent = currentEvent;
    }

    if (events[i].tests != nullptr) {
      
      if (individual_test_reports) {
        events[i].runTests();
      }
      else {
        snprintf(unityMessage, unityMessageLength, "%lu triggered at %lu", events[i].time, events[i].triggeredAt - startTime);
        UnityDefaultTestRun(events[i].runTests, unityMessage, __LINE__);
      }
      
    }
    
  }
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

void decodingTest::resetSpeeduino() {
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