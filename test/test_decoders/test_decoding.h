#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) testEntry, countof(testEntry), new testResults[countof(testEntry)]

struct testTooth {
  uint16_t angle;
  uint16_t degrees;
};

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
      REVTIME_c,
      TOOTHANGLECORRECT,
      TOOTHANGLE_c,
      LASTTOOTHTIME_c,
      LASTTOOTHTIMEMINUSONE_c,
      RPM,
      RPM_c_deltaPerThousand,
      STALLTIME_c,
      CRANKANGLE_c,
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
    const testTooth* const tooth = nullptr;
    uint32_t triggeredAt = 0;

    void trigger();
    static void runTests();

    timedEvent(const timedEventType type, const uint32_t time, const testParams* const tests, const byte testCount, testResults* const results, const testTooth* const tooth);
    timedEvent(const timedEventType type, const uint32_t time, const testParams* const tests, const byte testCount, testResults* const results);
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
    static float testLastUsPerDegree;
    static uint16_t testLastToothDegrees;
    static uint32_t testToothOneTime;
    static uint32_t testRevolutionTime;
    void execute();
    void showTriggerlog();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */