#ifndef TEST_DECODING_H
#define TEST_DECODING_H

enum timedTestType {
  ttt_CRANKANGLE,
  ttt_SYNC,
  ttt_HALFSYNC,
  ttt_SYNCLOSSCOUNT,
  ttt_RPM,
  ttt_REVCOUNT
};

struct testParams {

  const timedTestType type;
  const uint16_t expected;
  const uint8_t delta = 0;
  uint16_t result = 0;

  void execute();
  static void run_test();
  testParams(const timedTestType, const uint16_t);
  testParams(const timedTestType, const uint16_t, const uint8_t);

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