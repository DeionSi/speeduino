#ifndef DEBUG_HPP
#define DEBUG_HPP

enum PROFILING_SIGNAL : char {
  INIT,
  LOOP1,
  LOOP2,
  LOOP3,
  LOOP4,
  LOOP5,
  LOOP6,
  LOOP7,
  LOOP8,
  TRIGPRI,
  ONEMS,
  END_OF_ENUM
};

void initiateProfilingPins();
void setProfilingSignal(PROFILING_SIGNAL signal, bool inInterrupt);
void profilingPrimaryTrigger();

#endif