#ifndef TEST_DECODING_PROGRAM_H
#define TEST_DECODING_PROGRAM_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define testGroupEntry(testEntry) new testGroup { testEntry, countof(testEntry), new testResults[countof(testEntry)] }

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

struct testGroup {
  const testParams* const tests;
  const byte testCount;
  testResults* const results;
};

class timedEvent {
  public:
    enum timedEventType {
      PRITRIG,
      TEST,
      STALL,
    };

  private:

    void preTestsCommands();
    bool hasSyncOrHalfsync();
    void runTests();
    static void runTestsWrapper();

  public:
    const timedEventType type;
    const uint32_t time;
    testGroup* const tests = nullptr;
    const testTooth* const tooth = nullptr;
    static testResults* wrapperResult;
    static const testParams* wrapperTest;
    static timedEvent* wrapperEvent;
    static decodingTest* wrapperDecodingTest;

    void trigger(decodingTest* currentDecodingTest);
    void calculateExpected();

    timedEvent(const timedEventType type, const uint32_t time, testGroup* const tests, const testTooth* const tooth);
    timedEvent(const timedEventType type, const uint32_t time, testGroup* const tests);
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

  public:
    void execute();
    void showTriggerlog();
    void stallCleanup();
    static void resetTest();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_PROGRAM_H */