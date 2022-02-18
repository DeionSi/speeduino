#ifndef TEST_DECODING_H
#define TEST_DECODING_H

enum timedTestType {
  ttt_IGNORE,
  ttt_CRANKANGLE,
  ttt_SYNC,
  ttt_HALFSYNC,
  ttt_SYNCLOSSCOUNT,
  ttt_RPM,
  ttt_REVCOUNT
};

struct testParams {

  const timedTestType type;
  const uint32_t expected = 0;
  const uint16_t delta = 0;
  uint32_t result = 0;

  void execute();
  static void run_test();
  testParams(const timedTestType);
  testParams(const timedTestType, const uint32_t);
  testParams(const timedTestType, const uint32_t, const uint16_t);

};

//TODO: What are the expected decoder outputs?
// easy: sync, halfsync, synclosscount, revolutioncount, triggerToothAngle, triggerToothAngleIsCorrect
// medium: toothLastToothTime, toothLastMinusOneToothTime, rpm
// hard: crankangle, MAX_STALL_TIME

struct testState {
  testParams sync;
  testParams halfSync;
  testParams syncLossCount;
  testParams revCount;
  testParams toothAngleCorrect;
  testParams toothAngle;
  testParams lastToothTime;
  testParams lastToothTimeMinusOne;
  testParams rpm;
  testParams stallTime;
  testParams crankAngle;

  void execute();
  static void run_test();
};

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
  testState *state;
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