#include <debug.hpp>
#include <arduino.h>
#include "globals.h"
#include "decoders.h"

const byte rts_pin = 29;
volatile uint8_t * rts_pin_port;
volatile uint8_t rts_pin_mask;

const byte continue_pin = 43;
volatile uint8_t * continue_pin_port;
volatile uint8_t continue_pin_mask;

void initiateProfilingPins() {
  pinMode(rts_pin, OUTPUT); // enable port
  pinMode(continue_pin, INPUT_PULLUP); // wait for pin
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

  continue_pin_port = portInputRegister(digitalPinToPort(continue_pin));
  continue_pin_mask = digitalPinToBitMask(continue_pin);

  digitalWrite(rts_pin, LOW);
}

void sendProfilingSignal() {
  if (*rts_pin_port & rts_pin_mask) {
    *rts_pin_port &= ~(rts_pin_mask);
  }
  else {
    *rts_pin_port |= (rts_pin_mask);
  }
}

void profilingPrimaryTrigger() {
  setProfilingSignal(TRIGPRI, true);
  triggerHandler();
  setProfilingSignal(TRIGPRI, true);
}

void profilingSecondaryTrigger() {
  setProfilingSignal(TRIGSEC, true);
  triggerSecondaryHandler();
  setProfilingSignal(TRIGSEC, true);
}

void profilingTertriaryTrigger() {
  setProfilingSignal(TRIGTER, true);
  triggerTertiaryHandler();
  setProfilingSignal(TRIGTER, true);
}

void setProfilingSignal(PROFILING_SIGNAL signal, bool inInterrupt) {
  noInterrupts();

  //uint16_t testCounter = 0;
  do {
    bool rtsB = (*rts_pin_port & rts_pin_mask);
    bool continueB = (*continue_pin_port & continue_pin_mask);
    if(rtsB == continueB) { break; }
    /*testCounter++;
    if(testCounter == 65000) { 
      sendProfilingSignal();
    }*/
  } while(1);
  //profilingContinueLast = (*continue_pin_port & continue_pin_mask);

  PORTC = signal;
  sendProfilingSignal();

  if (!inInterrupt) { interrupts(); }
}