#ifndef TEST_DECODING_SHARED_H
#define TEST_DECODING_SHARED_H

#include "program.h"

const testParams unsynced_0loss_tp[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 0 },
};
testGroupEntry(unsynced_0loss);

const testParams unsynced_1loss_tp[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 1 },
};
testGroupEntry(unsynced_1loss);

const testParams unsynced_2loss_tp[] = {
  { testParams::SYNC, 0 },
  { testParams::HALFSYNC, 0 },
  { testParams::SYNCLOSSCOUNT, 2 },
};
testGroupEntry(unsynced_2loss);

#endif // TEST_DECODING_SHARED_H