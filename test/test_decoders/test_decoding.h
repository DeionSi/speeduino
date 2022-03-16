#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) testEntry, countof(testEntry), new testResults[countof(testEntry)]

//TODO: Rename structures to be more logical

class decodingTest;

struct testTooth {
  uint16_t angle;
  uint16_t degrees;
};

struct testResults {
  int32_t value;
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
    const int32_t expected;
    const uint16_t delta = 0;

  public:

    int32_t getResult() const;
    bool hasSyncOrHalfsync() const;
    void runTest(testResults* result) const;
    static void runTestWrapper();
    const char* name() const;
    testParams(const timedTestType, const int32_t);
    testParams(const timedTestType, const int32_t, const uint16_t);

};

class timedEvent {
  public:
    enum timedEventType {
      PRITRIG,
      TEST,
      STALL,
    };

  private:
    const byte testCount = 0;
    testResults* const results = nullptr;

    void preTestsCommands();
    bool hasSyncOrHalfsync();
    void runTests(uint32_t testStartTime);
    static void runTestsWrapper();

  public:
    const timedEventType type;
    const uint32_t time;
    const testParams* const tests = nullptr;
    const testTooth* const tooth = nullptr;
    uint32_t triggeredAt = 0;
    static testResults* wrapperResult;
    static const testParams* wrapperTest;
    static timedEvent* wrapperEvent;
    static decodingTest* wrapperDecodingTest;

    void trigger(decodingTest* currentDecodingTest);
    void run(decodingTest* currentDecodingTest);

    timedEvent(const timedEventType type, const uint32_t time, const testParams* const tests, const byte testCount, testResults* const results, const testTooth* const tooth);
    timedEvent(const timedEventType type, const uint32_t time, const testParams* const tests, const byte testCount, testResults* const results);
    timedEvent(const timedEventType type, const uint32_t time, const testTooth* const tooth);
    timedEvent(const timedEventType type, const uint32_t time);
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

  public:
    uint32_t startTime = 0;
    
    void execute();
    void showTriggerlog();
    void stallCleanup();
    static void resetTest();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */