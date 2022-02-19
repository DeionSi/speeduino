#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) .tests = testEntry, .testCount = countof(testEntry), .results = new uint32_t[countof(testEntry)]


struct testParams {

  enum timedTestType {
    SYNC,
    HALFSYNC,
    SYNCLOSSCOUNT,
    REVCOUNT,
    TOOTHANGLECORRECT,
    TOOTHANGLE,
    LASTTOOTHTIME,
    LASTTOOTHTIMEMINUSONE,
    RPM,
    STALLTIME,
    CRANKANGLE,
    ENUMEND,
  } const type;

  const uint32_t expected;
  const uint16_t delta = 0;
  static const char* const friendlyNames[];

  uint32_t getResult() const;
  static void runTest();
  const char* name() const;
  testParams(const timedTestType, const uint32_t);
  testParams(const timedTestType, const uint32_t, const uint16_t);

};

//TODO: What are the expected decoder outputs?
// easy: sync, halfsync, synclosscount, revolutioncount
// medium: toothLastToothTime, toothLastMinusOneToothTime, rpm, triggerToothAngle, triggerToothAngleIsCorrect
// hard: crankangle, MAX_STALL_TIME


struct timedEvent {

  enum timedEventType {
    PRITRIG,
    TEST,
  } const type;

  void trigger();
  static void runTests();

  const uint32_t time;
  const testParams* const tests;
  const byte testCount;
  uint32_t* const results;
  uint32_t triggeredAt = 0;

  timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, uint32_t* const a_results);
};

// TODO: structs into classes
struct decodingTest {
  const char* const name;
  void (*const decoderSetup)();
  timedEvent* const events;
  const byte eventCount;
  uint32_t startTime = 0;

  void gatherResults();
  bool verifyEventOrder() const;
  void execute();
  void decodingSetup();
  void compareResults();
  static void resetSpeeduino();
  void showTriggerlog();

  decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */