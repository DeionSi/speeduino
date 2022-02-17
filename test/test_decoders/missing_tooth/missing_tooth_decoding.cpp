#include "Arduino.h"
#include "unity.h"
#include "init.h"
#include "globals.h"
#include "decoders.h"
#include "missing_tooth_decoding_testdata.h"

const byte unityMessageLength = 200;
char unityMessage[unityMessageLength];

//TODO: What are the expected decoder outputs?

void testMissingToothDecoding() {
  // Perform the test
  for (auto testData : decodingTests) {
    testData.execute();
    testData.run_tests();
    testData.reset_decoding();
  }
}