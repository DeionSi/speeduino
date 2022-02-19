#include "arduino.h"
#include "decoders.h"
#include "crankMaths.h"
#include "unity.h"
#include "init.h"
#include "test_decoding.h"

#define INDIVIDUAL_TEST_REPORTS // Shows each test output rather than one per event
#define INDIVIDUAL_TEST_REPORTS_DEBUG // Shows delta/expected/result for each individual test

// Global variables

const byte unityMessageLength = 100;
char unityMessage[unityMessageLength];

uint32_t *currentResult;
timedEvent *currentEvent;
const testParams* currentTest;
decodingTest* currentDecodingTest;

// TODO: replace with timers

void delayUntil(uint32_t time) {
  if (micros() < time) { // No delay if we have already passed
    if (time >= 12000) { // Make sure we don't compare against negative time
      while (micros() < time - 12000) { delay(10); } // Delay in MILLIseconds until we can delay in MICROseconds
    }
    delayMicroseconds(time - micros()); // Remaining delay in MICROseconds
  }
}

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

uint32_t testParams::execute() const {
  uint32_t result = 0;

  switch(type) {
    case CRANKANGLE:
      result = getCrankAngle();
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
    case RPM:
      result = getRPM();
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

void testParams::run_test() {

  #ifdef INDIVIDUAL_TEST_REPORTS_DEBUG
  const char testMessageLength = 40;
  char testMessage[testMessageLength];
  snprintf(testMessage, testMessageLength, "result %lu expected %lu delta %u ", *currentResult, currentTest->expected, currentTest->delta);
  UnityPrint(testMessage);
  for (int i = strlen(testMessage); i < testMessageLength+1; i++) { UnityPrint(" "); } //Padding
  #endif

  if (currentTest->delta == 0) {
    TEST_ASSERT_EQUAL(currentTest->expected, *currentResult);
  }
  else {
    TEST_ASSERT_INT_WITHIN(currentTest->delta, currentTest->expected, *currentResult);
  }
}

const char* testParams::name() const {
  return friendlyNames[this->type];
}

// timedEvent

timedEvent::timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, uint32_t* const a_results) :
type(a_type), time(a_time), tests(a_tests), testCount(a_testCount), results(a_results)
{ };

void timedEvent::execute() {
  triggeredAt = micros();
  if (type == PRITRIG) {
    triggerHandler();
  }
  if (tests != nullptr) {
    for (int i = 0; i < testCount; i++) {
      results[i] = tests[i].execute();
    }
  }
};

void timedEvent::run_tests() {
  if (currentEvent->tests != nullptr) {
    for (int i = 0; i < currentEvent->testCount; i++) {
      currentTest = &currentEvent->tests[i];
      currentResult = &currentEvent->results[i];

      #ifdef INDIVIDUAL_TEST_REPORTS
      snprintf(unityMessage, unityMessageLength, "'%s' triggered at %lu", currentEvent->tests[i].name(), currentEvent->triggeredAt - currentDecodingTest->startTime);
      UnityDefaultTestRun(currentEvent->tests[i].run_test, unityMessage, __LINE__);
      #else
      currentEvent->tests[i].run_test();
      #endif
    }
  }
}

// decodingTest

decodingTest::decodingTest(const char* const a_name, void (*const a_decodingSetup)(), timedEvent* const a_events, const byte a_eventCount) : name(a_name), decodingSetup(a_decodingSetup), events(a_events), eventCount(a_eventCount) { } ;

void decodingTest::execute() {
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
  startTime = micros();

  for (int i = 0; i < eventCount; i++) {
    
    //Only perform calculations before a test. We don't need to recalculate before subsequent tests
    if ( ( i == 0 || ( i > 0 && events[i-1].type != timedEvent::TEST ) ) && events[i].type == timedEvent::TEST) {
      currentStatus.RPM = getRPM();
      doCrankSpeedCalcs();
    }

    if (events[i].time < UINT32_MAX) { // UINT32_MAX Entries are parsed as soon as they are reached
      delayUntil(events[i].time + startTime);
    }

    events[i].execute();

  }

}

void decodingTest::show_triggerlog() {
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

void decodingTest::run_tests() {
  currentDecodingTest = this;
  for (int i = 0; i < eventCount; i++) {
    if (events[i].tests != nullptr) {
      currentEvent = &events[i];
      
      #ifdef INDIVIDUAL_TEST_REPORTS
      events[i].run_tests();
      #else
      snprintf(unityMessage, unityMessageLength, "%lu triggered at %lu", events[i].time, events[i].triggeredAt - startTime);
      UnityDefaultTestRun(events[i].run_tests, unityMessage, __LINE__);
      #endif
    }
  }
}

void decodingTest::reset_decoding() {
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