#ifndef DEBUG_HPP
#define DEBUG_HPP

enum PROFILING_SIGNAL : char {
  INIT,
  LOOP_start,
  LOOP_looptimers,
  LOOP_idlefueladvance,
  LOOP_mainloop_fuelcalcs,
  LOOP_mainloop_injectiontiming,
  LOOP_mainloop_igncalcs,
  LOOP_mainloop_fuelschedules,
  LOOP_mainloop_ignschedules,
  LOOP_mainloop_other,
  PS_pwfunction,
  PS_correctionsFuel,
  PS_docrankspeedcalcs,
  TRIGPRI,
  TRIGSEC,
  TRIGTER,
  ONEMS
};

void initiateProfilingPins();
void setProfilingSignal(PROFILING_SIGNAL signal, bool inInterrupt);
void profilingPrimaryTrigger();
void profilingSecondaryTrigger();
void profilingTertriaryTrigger();

#endif