#ifndef TEST_DECODING_H
#define TEST_DECODING_H

#define countof(arr) sizeof(arr) / sizeof(arr[0])
#define timedEventArrayTestEntry(testEntry) .test = testEntry, .testCount = countof(testEntry)

enum timedTestType {
  ttt_SYNC,
  ttt_HALFSYNC,
  ttt_SYNCLOSSCOUNT,
  ttt_REVCOUNT,
  ttt_TOOTHANGLECORRECT,
  ttt_TOOTHANGLE,
  ttt_LASTTOOTHTIME,
  ttt_LASTTOOTHTIMEMINUSONE,
  ttt_RPM,
  ttt_STALLTIME,
  ttt_CRANKANGLE,
  ttt_ENUMEND,
  ttt_IGNORE,  // special identifier causing a test to be ignored
};

struct testParams {

  const timedTestType type = ttt_IGNORE;
  const uint32_t expected = 0;
  const uint16_t delta = 0;
  uint32_t result = 0;

  void execute();
  static void run_test();
  testParams();
  testParams(const timedTestType);
  testParams(const timedTestType, const uint32_t);
  testParams(const timedTestType, const uint32_t, const uint16_t);

};

//TODO: What are the expected decoder outputs?
// easy: sync, halfsync, synclosscount, revolutioncount
// medium: toothLastToothTime, toothLastMinusOneToothTime, rpm, triggerToothAngle, triggerToothAngleIsCorrect
// hard: crankangle, MAX_STALL_TIME

enum timedEventType {
  tet_PRITRIG,
  tet_TEST,
};

struct timedEvent {
  void execute();
  void run_test();

  const timedEventType type;
  const uint32_t time;
  testParams *test;
  const byte testCount;
  
  timedEvent(const timedEventType type, const uint32_t time, testParams* test, const byte testCount);
};

struct decodingTest {
  const char *name;
  void (*decodingSetup)();
  timedEvent *events;
  const byte eventCount;

  void execute();
  void run_tests();
  static void reset_decoding();
};

#endif /* TEST_DECODING_H */