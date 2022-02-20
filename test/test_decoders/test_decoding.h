#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) testEntry, countof(testEntry), new uint32_t[countof(testEntry)]


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
      STALLTIME,
      CRANKANGLE,
      ENUMEND,
    };

  private:
    static const char* const friendlyNames[];
    const timedTestType type;
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
    const timedEventType type;
    const byte testCount;
    uint32_t* const results;

    void preTestsCommands();
    static void delayUntil(uint32_t time);

  public:
    const uint32_t time;
    const testParams* const tests;
    const uint32_t crankMilliDegrees = 0;
    uint32_t triggeredAt = 0;

    void trigger();
    static void runTests();

    timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, uint32_t* const a_results);
    timedEvent(const timedEventType a_type, const uint32_t a_time, const testParams* const a_tests, const byte a_testCount, uint32_t* const a_results, const uint32_t crankMilliDegrees);
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
    uint32_t startTime = 0;
    void execute();
    void showTriggerlog();
    decodingTest(const char* const name, void (*const decoderSetup)(), timedEvent* const events, const byte eventCount);
};

#endif /* TEST_DECODING_H */