#include <debug.hpp>
#include <arduino.h>
#include "globals.h"
#include "decoders.h"
#include <util/atomic.h>

const byte rts_pin = 29;
volatile uint8_t * rts_pin_port;
volatile uint8_t rts_pin_mask;
int (*getCrankAngleReal)();

void initiateProfilingPins() {
  pinMode(rts_pin, OUTPUT); // enable port
  pinMode(30, OUTPUT); // PORTC7
  pinMode(31, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(36, OUTPUT);
  pinMode(37, OUTPUT); // PORTC0

  rts_pin_port = portOutputRegister(digitalPinToPort(rts_pin));
  rts_pin_mask = digitalPinToBitMask(rts_pin);

  digitalWrite(rts_pin, LOW);
}

void profilingPrimaryTrigger() {
  setProfilingSignal(TRIGPRI);
  triggerHandler();
  setProfilingSignal(TRIGPRI);
}

void profilingSecondaryTrigger() {
  setProfilingSignal(TRIGSEC);
  triggerSecondaryHandler();
  setProfilingSignal(TRIGSEC);
}

void profilingTertriaryTrigger() {
  setProfilingSignal(TRIGTER);
  triggerTertiaryHandler();
  setProfilingSignal(TRIGTER);
}

int profilingGetCrankAngle() {
  setProfilingSignal(PS_getCrankAngle);
  int crankAngle = getCrankAngleReal();
  setProfilingSignal(PS_getCrankAngle);
  return crankAngle;
}

void setProfilingSignal(PROFILING_SIGNAL signal) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

    // Set which signal
    PORTC = signal;

    // Say that the signal changed
    if (*rts_pin_port & rts_pin_mask) {
      *rts_pin_port &= ~(rts_pin_mask);
    }
    else {
      *rts_pin_port |= (rts_pin_mask);
    }

  }
}