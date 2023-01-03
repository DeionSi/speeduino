#include <Arduino.h>
#include <globals.h>
#include <unity.h>
#include "missing_tooth.h"

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    //UNITY_BEGIN();    // IMPORTANT LINE!
    UnityBegin("decodingTests");

    //testMissingTooth();
    //testDualWheel();
    testDecodingMissingTooth();

    UNITY_END(); // stop unit testing
}

void loop()
{
    // Blink to indicate end of test
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
}