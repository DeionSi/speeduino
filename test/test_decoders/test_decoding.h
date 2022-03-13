#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) testEntry, countof(testEntry), new testResults[countof(testEntry)]

class decodingTest;

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
      REVCOUNT_c,
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
    bool hasSyncOrHalfsync() const;
    void runTest(testResults* result) const;
    static void runTestWrapper();
    const char* name() const;
    testParams(const timedTestType, const uint32_t);
    testParams(const timedTestType, const uint32_t, const uint16_t);

};

class timedEvent {
  public:
    enum timedEventType {
      PRITRIG,
      TEST,
      STALL,
    };

  private:
    const byte testCount;
    testResults* const results;

    void preTestsCommands();
    bool hasSyncOrHalfsync();

  public:
    const timedEventType type;
    const uint32_t time;
    const testParams* const tests;
    const testTooth* const tooth = nullptr;
    uint32_t triggeredAt = 0;
    static testResults* wrapperResult;
    static const testParams* wrapperTest;

    void trigger(decodingTest* currentDecodingTest);
    void runTests(uint32_t testStartTime);
    static void runTestsWrapper();

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
    static void resetTest();

  public:
    uint32_t startTime = 0;
    static timedEvent* wrapperEvent;
    static decodingTest* wrapperDecodingTest;
    
    void execute();
    void showTriggerlog();
    void stallCleanup();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */