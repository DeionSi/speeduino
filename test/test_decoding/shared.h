#ifndef TEST_DECODING_SHARED_H
#define TEST_DECODING_SHARED_H

#include "program.h"

const testParams unsynced_0loss[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
};
const testParams unsynced_1loss[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 1 },
};
const testParams unsynced_2loss[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 2 },
};

#endif // TEST_DECODING_SHARED_H