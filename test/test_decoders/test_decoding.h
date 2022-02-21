#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) testEntry, countof(testEntry), new testResults[countof(testEntry)]

struct testResults {
  uint32_t value;
  uint32_t retrievedAt;
};

class testParams {
  public:
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
      STALLTIME, // TODO: Are there any rules set on stalltime, is it up to each decoder to set?
      CRANKANGLE,
      ENUMEND,
    };

  private:
    const timedTestType type;
    static const char* const friendlyNames[];
    const uint32_t expected;
    const uint16_t delta = 0;

  public:

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


class timedEvent {
  public:
    enum timedEventType {
      PRITRIG,
      TEST,
    };

  private:
    const byte testCount;
    testResults* const results;

    void preTestsCommands();
    static void delayUntil(uint32_t time);

  public:
    const timedEventType type;
    const uint32_t time;
    const testParams* const tests;
    const int16_t crankDegrees = 0;
    uint32_t triggeredAt = 0;

    void trigger();
    static void runTests();

    timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, testResults* const a_results, const uint16_t crankDegrees);
};

class decodingTest {
  private:
    const char* const name;
    void (*const decoderSetup)();
    timedEvent* const events;
    const byte eventCount;

    void gatherResults();
    bool verifyEventOrder() const;
    void decodingSetup();
    void compareResults();
    static void resetSpeeduino();

  public:
    static uint32_t startTime;
    static uint32_t testLastToothTime;
    static uint32_t testLastToothMinusOneTime;
    void execute();
    void showTriggerlog();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */