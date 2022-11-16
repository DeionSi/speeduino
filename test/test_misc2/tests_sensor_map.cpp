#include "sensors.h"
#include "globals.h"
#include "unity.h"
#include "errors.h"

// A small hack to get Visual Studio intellisense to realize these variables exist
#ifndef UNIT_TEST
extern uint16_t unitTestMAPinput;
extern uint16_t unitTestEMAPinput;
extern unsigned long MAPsamplingRunningValue;
extern unsigned long EMAPsamplingRunningValue;
extern bool instantaneousMAP;
extern bool instantaneousEMAP;
extern unsigned int MAPsamplingCount;
extern unsigned int EMAPsamplingCount;
extern uint32_t MAPsamplingNext;
#endif

enum MAPsensors { testMAP, testEMAP };

#define countof(arr) sizeof(arr) / sizeof(arr[0])

struct sensorMAP_testdata {
  const byte startRevolutions;
  const byte ignitionCount;
  const byte RPMdiv100;
  const uint16_t testInput;
  const int16_t expected;
  const long expectedADC;
  const byte errMAP;
  const int16_t expectedLast;
  const bool engineProtectStatus;
} const * sensorMAP_testdata_current;

struct sensorMAP_testsetting {
  const MAPsensors whichSensor;
  const int8_t sensorMin; // kPa At 0.0 Volts, range -100 to 127
  const uint16_t sensorMax; // kPa At 5.0 Volts, range 0 to 25500
  const byte mapSample; // MAP sampling method (0=Instantaneous, 1=Cycle Average, 2=Cycle Minimum, 3=Ign. event average)
  const sensorMAP_testdata * const testdata;
  const byte testdataCount;
} const * sensorMAP_testsetting_current;

const sensorMAP_testdata sensorMAP_testdata_instantaneous[] = {
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 2, .testInput = 100,  .expected = 2400, .expectedADC = 100, .errMAP = 0, .expectedLast = 0, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 1, .RPMdiv100 = 1, .testInput = 0,  .expected = -1, .expectedADC = 100, .errMAP = ERR_MAP_LOW, .expectedLast = 2400, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 2, .testInput = 750,  .expected = 17375, .expectedADC = 699, .errMAP = 0, .expectedLast = -1, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 3, .RPMdiv100 = 0, .testInput = 1024,  .expected = 25500, .expectedADC = 699, .errMAP = ERR_MAP_HIGH, .expectedLast = 17375, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 0, .RPMdiv100 = 1, .testInput = 1022,  .expected = 24800, .expectedADC = 996, .errMAP = 0, .expectedLast = 25500, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 0, .RPMdiv100 = 3, .testInput = 1022,  .expected = 25375, .expectedADC = 1019, .errMAP = 0, .expectedLast = 24800, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 0, .RPMdiv100 = 1, .testInput = 500,  .expected = 13400, .expectedADC = 540, .errMAP = 0, .expectedLast = 25375, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 0, .RPMdiv100 = 3, .testInput = 250,  .expected = 6700, .expectedADC = 272, .errMAP = 0, .expectedLast = 13400, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 0, .testInput = 2,  .expected = 475, .expectedADC = 23, .errMAP = 0, .expectedLast = 6700, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 3, .RPMdiv100 = 2, .testInput = 2,  .expected = 0, .expectedADC = 3, .errMAP = 0, .expectedLast = 475, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 0, .RPMdiv100 = 2, .testInput = 400,  .expected = 9100, .expectedADC = 368, .errMAP = 0, .expectedLast = 0, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 4, .RPMdiv100 = 0, .testInput = 370,  .expected = 9125, .expectedADC = 369, .errMAP = 0, .expectedLast = 9100, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 3, .RPMdiv100 = 0, .testInput = 370,  .expected = 9125, .expectedADC = 369, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 4, .testInput = 350,  .expected = 8675, .expectedADC = 351, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 3, .RPMdiv100 = 2, .testInput = 310,  .expected = 7725, .expectedADC = 313, .errMAP = 0, .expectedLast = 8675, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 0, .testInput = 220,  .expected = 5575, .expectedADC = 227, .errMAP = 0, .expectedLast = 7725, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 4, .testInput = 110,  .expected = 2875, .expectedADC = 119, .errMAP = 0, .expectedLast = 5575, .engineProtectStatus = 1 },
  { .startRevolutions = 4, .ignitionCount = 3, .RPMdiv100 = 1, .testInput = 50,  .expected = 1275, .expectedADC = 55, .errMAP = 0, .expectedLast = 2875, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 3, .RPMdiv100 = 4, .testInput = 50,  .expected = 1150, .expectedADC = 50, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 1 },
  { .startRevolutions = 4, .ignitionCount = 4, .RPMdiv100 = 4, .testInput = 150,  .expected = 3450, .expectedADC = 142, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 1, .RPMdiv100 = 0, .testInput = 280,  .expected = 6625, .expectedADC = 269, .errMAP = 0, .expectedLast = 3450, .engineProtectStatus = 1 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 2, .testInput = 370,  .expected = 8950, .expectedADC = 362, .errMAP = 0, .expectedLast = 6625, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 1, .testInput = 370,  .expected = 9125, .expectedADC = 369, .errMAP = 0, .expectedLast = 8950, .engineProtectStatus = 1 },
  { .startRevolutions = 0, .ignitionCount = 1, .RPMdiv100 = 1, .testInput = 360,  .expected = 8900, .expectedADC = 360, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 0, .RPMdiv100 = 2, .testInput = 350,  .expected = 8650, .expectedADC = 350, .errMAP = 0, .expectedLast = 8900, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 0, .RPMdiv100 = 0, .testInput = 300,  .expected = 7475, .expectedADC = 303, .errMAP = 0, .expectedLast = 8650, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 4, .RPMdiv100 = 0, .testInput = 100,  .expected = 2775, .expectedADC = 115, .errMAP = 0, .expectedLast = 7475, .engineProtectStatus = 1 },
  { .startRevolutions = 4, .ignitionCount = 1, .RPMdiv100 = 1, .testInput = 200,  .expected = 4725, .expectedADC = 193, .errMAP = 0, .expectedLast = 2775, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 4, .RPMdiv100 = 2, .testInput = 100,  .expected = 2575, .expectedADC = 107, .errMAP = 0, .expectedLast = 4725, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 1, .RPMdiv100 = 2, .testInput = 50,  .expected = 1250, .expectedADC = 54, .errMAP = 0, .expectedLast = 2575, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 3, .testInput = 100,  .expected = 2300, .expectedADC = 96, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 1 },
  { .startRevolutions = 0, .ignitionCount = 0, .RPMdiv100 = 3, .testInput = 100,  .expected = 2375, .expectedADC = 99, .errMAP = 0, .expectedLast = 2300, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 2, .RPMdiv100 = 4, .testInput = 75,  .expected = 1800, .expectedADC = 76, .errMAP = 0, .expectedLast = 2375, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 1, .RPMdiv100 = 0, .testInput = 75,  .expected = 1775, .expectedADC = 75, .errMAP = 0, .expectedLast = 1800, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 1, .RPMdiv100 = 3, .testInput = 200,  .expected = 4650, .expectedADC = 190, .errMAP = 0, .expectedLast = 1775, .engineProtectStatus = 1 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 4, .testInput = 210,  .expected = 5100, .expectedADC = 208, .errMAP = 0, .expectedLast = 4650, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 2, .RPMdiv100 = 0, .testInput = 210,  .expected = 5125, .expectedADC = 209, .errMAP = 0, .expectedLast = 5100, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 2, .testInput = 250,  .expected = 6050, .expectedADC = 246, .errMAP = 0, .expectedLast = 5125, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 3, .RPMdiv100 = 1, .testInput = 280,  .expected = 6825, .expectedADC = 277, .errMAP = 0, .expectedLast = 6050, .engineProtectStatus = 1 },
  { .startRevolutions = 2, .ignitionCount = 3, .RPMdiv100 = 0, .testInput = 300,  .expected = 7350, .expectedADC = 298, .errMAP = 0, .expectedLast = 6825, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 3, .testInput = 280,  .expected = 6925, .expectedADC = 281, .errMAP = 0, .expectedLast = 7350, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 1, .testInput = 290,  .expected = 7125, .expectedADC = 289, .errMAP = 0, .expectedLast = 6925, .engineProtectStatus = 0 },
};

const sensorMAP_testdata sensorMAP_testdata_cycle_average[] = {
  { .startRevolutions = 0, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 0, .engineProtectStatus = 1 },
  { .startRevolutions = 0, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 350,  .expected = 8675, .expectedADC = 351, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 310,  .expected = 7725, .expectedADC = 313, .errMAP = 0, .expectedLast = 8675, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 220,  .expected = 5575, .expectedADC = 227, .errMAP = 0, .expectedLast = 7725, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 110,  .expected = 2875, .expectedADC = 119, .errMAP = 0, .expectedLast = 5575, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 50,  .expected = 1275, .expectedADC = 55, .errMAP = 0, .expectedLast = 2875, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 50,  .expected = 1150, .expectedADC = 50, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 150,  .expected = 3450, .expectedADC = 142, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 280,  .expected = 6625, .expectedADC = 269, .errMAP = 0, .expectedLast = 3450, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 370,  .expected = 8950, .expectedADC = 362, .errMAP = 0, .expectedLast = 6625, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 370,  .expected = 9125, .expectedADC = 369, .errMAP = 0, .expectedLast = 8950, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 360,  .expected = 5175, .expectedADC = 360, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 350,  .expected = 5175, .expectedADC = 350, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 300,  .expected = 5175, .expectedADC = 303, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 5175, .expectedADC = 115, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 200,  .expected = 5175, .expectedADC = 193, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 100,  .expected = 5175, .expectedADC = 107, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 50,  .expected = 5175, .expectedADC = 54, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 100,  .expected = 5175, .expectedADC = 96, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 100,  .expected = 5175, .expectedADC = 99, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 75,  .expected = 5175, .expectedADC = 76, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 75,  .expected = 4275, .expectedADC = 75, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 0 },
  { .startRevolutions = 6, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 200,  .expected = 4275, .expectedADC = 190, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 210,  .expected = 4275, .expectedADC = 208, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 210,  .expected = 4275, .expectedADC = 209, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 1 },
  { .startRevolutions = 7, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 250,  .expected = 4275, .expectedADC = 246, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 1 },
  { .startRevolutions = 7, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 280,  .expected = 4275, .expectedADC = 277, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 0 },
  { .startRevolutions = 7, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 300,  .expected = 4275, .expectedADC = 298, .errMAP = 0, .expectedLast = 5175, .engineProtectStatus = 0 },
  { .startRevolutions = 8, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 280,  .expected = 5250, .expectedADC = 281, .errMAP = 0, .expectedLast = 4275, .engineProtectStatus = 0 },
  { .startRevolutions = 8, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 290,  .expected = 5250, .expectedADC = 289, .errMAP = 0, .expectedLast = 4275, .engineProtectStatus = 1 },
  { .startRevolutions = 9, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 3,  .expected = 5250, .expectedADC = 25, .errMAP = 0, .expectedLast = 4275, .engineProtectStatus = 0 },
  { .startRevolutions = 9, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 4,  .expected = 5250, .expectedADC = 5, .errMAP = 0, .expectedLast = 4275, .engineProtectStatus = 1 },
  { .startRevolutions = 9, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 10,  .expected = 5250, .expectedADC = 9, .errMAP = 0, .expectedLast = 4275, .engineProtectStatus = 0 },
  { .startRevolutions = 10, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 0,  .expected = 2925, .expectedADC = 9, .errMAP = ERR_MAP_LOW, .expectedLast = 5250, .engineProtectStatus = 1 },
  { .startRevolutions = 10, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 250,  .expected = 2925, .expectedADC = 231, .errMAP = 0, .expectedLast = 5250, .engineProtectStatus = 0 },
  { .startRevolutions = 11, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 750,  .expected = 2925, .expectedADC = 709, .errMAP = 0, .expectedLast = 5250, .engineProtectStatus = 1 },
  { .startRevolutions = 11, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 0,  .expected = 2925, .expectedADC = 709, .errMAP = ERR_MAP_LOW, .expectedLast = 5250, .engineProtectStatus = 1 },
  { .startRevolutions = 11, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 900,  .expected = 2925, .expectedADC = 885, .errMAP = 0, .expectedLast = 5250, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1021,  .expected = 15100, .expectedADC = 1010, .errMAP = 0, .expectedLast = 2925, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1024,  .expected = 15100, .expectedADC = 1010, .errMAP = ERR_MAP_HIGH, .expectedLast = 2925, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 900,  .expected = 15100, .expectedADC = 908, .errMAP = 0, .expectedLast = 2925, .engineProtectStatus = 1 },
  { .startRevolutions = 13, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 500,  .expected = 15100, .expectedADC = 531, .errMAP = 0, .expectedLast = 2925, .engineProtectStatus = 0 },
  { .startRevolutions = 13, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 0,  .expected = 15100, .expectedADC = 531, .errMAP = ERR_MAP_LOW, .expectedLast = 2925, .engineProtectStatus = 0 },
  { .startRevolutions = 14, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 20300, .expectedADC = 133, .errMAP = 0, .expectedLast = 15100, .engineProtectStatus = 1 },
  { .startRevolutions = 14, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1024,  .expected = 20300, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 15100, .engineProtectStatus = 0 },
  { .startRevolutions = 14, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 1024,  .expected = 20300, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 15100, .engineProtectStatus = 1 },
  { .startRevolutions = 15, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 0,  .expected = 20300, .expectedADC = 133, .errMAP = ERR_MAP_LOW, .expectedLast = 15100, .engineProtectStatus = 0 },
  { .startRevolutions = 15, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 20300, .expectedADC = 471, .errMAP = 0, .expectedLast = 15100, .engineProtectStatus = 0 },
  { .startRevolutions = 15, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 20300, .expectedADC = 128, .errMAP = 0, .expectedLast = 15100, .engineProtectStatus = 1 },
  { .startRevolutions = 16, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 500,  .expected = 6000, .expectedADC = 470, .errMAP = 0, .expectedLast = 20300, .engineProtectStatus = 0 },
  { .startRevolutions = 17, .ignitionCount = 2, .RPMdiv100 = 11, .testInput = 700,  .expected = 16950, .expectedADC = 682, .errMAP = 0, .expectedLast = 6000, .engineProtectStatus = 1 },
  { .startRevolutions = 18, .ignitionCount = 3, .RPMdiv100 = 11, .testInput = 800,  .expected = 19650, .expectedADC = 790, .errMAP = 0, .expectedLast = 16950, .engineProtectStatus = 1 },
  { .startRevolutions = 19, .ignitionCount = 1, .RPMdiv100 = 11, .testInput = 1000,  .expected = 24475, .expectedADC = 983, .errMAP = 0, .expectedLast = 19650, .engineProtectStatus = 1 },
  { .startRevolutions = 20, .ignitionCount = 3, .RPMdiv100 = 11, .testInput = 900,  .expected = 22550, .expectedADC = 906, .errMAP = 0, .expectedLast = 24475, .engineProtectStatus = 1 },
  { .startRevolutions = 21, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 700,  .expected = 17800, .expectedADC = 716, .errMAP = 0, .expectedLast = 22550, .engineProtectStatus = 1 },
  { .startRevolutions = 22, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 20175, .expectedADC = 516, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 1 },
  { .startRevolutions = 23, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 600,  .expected = 20175, .expectedADC = 593, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 0 },
  { .startRevolutions = 24, .ignitionCount = 0, .RPMdiv100 = 10, .testInput = 700,  .expected = 17175, .expectedADC = 691, .errMAP = 0, .expectedLast = 20175, .engineProtectStatus = 1 },
  { .startRevolutions = 25, .ignitionCount = 4, .RPMdiv100 = 10, .testInput = 800,  .expected = 19675, .expectedADC = 791, .errMAP = 0, .expectedLast = 17175, .engineProtectStatus = 1 },
  { .startRevolutions = 26, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 900,  .expected = 18425, .expectedADC = 891, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 0 },
  { .startRevolutions = 27, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 18425, .expectedADC = 161, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 1 },
  { .startRevolutions = 28, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 200,  .expected = 13050, .expectedADC = 196, .errMAP = 0, .expectedLast = 18425, .engineProtectStatus = 1 },
  { .startRevolutions = 29, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 300,  .expected = 13050, .expectedADC = 291, .errMAP = 0, .expectedLast = 18425, .engineProtectStatus = 1 },
  { .startRevolutions = 30, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 0,  .expected = 5975, .expectedADC = 291, .errMAP = ERR_MAP_LOW, .expectedLast = 13050, .engineProtectStatus = 1 },
  { .startRevolutions = 31, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 500,  .expected = 5975, .expectedADC = 483, .errMAP = 0, .expectedLast = 13050, .engineProtectStatus = 1 },
  { .startRevolutions = 32, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1021,  .expected = 11975, .expectedADC = 978, .errMAP = 0, .expectedLast = 5975, .engineProtectStatus = 1 },
  { .startRevolutions = 33, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 11975, .expectedADC = 537, .errMAP = 0, .expectedLast = 5975, .engineProtectStatus = 1 },
  { .startRevolutions = 34, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 0,  .expected = 18825, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 11975, .engineProtectStatus = 0 },
  { .startRevolutions = 35, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 0,  .expected = 18825, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 11975, .engineProtectStatus = 1 },
  { .startRevolutions = 36, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 0,  .expected = -1, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 18825, .engineProtectStatus = 1 },
  { .startRevolutions = 37, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 500,  .expected = 12450, .expectedADC = 502, .errMAP = 0, .expectedLast = -1, .engineProtectStatus = 0 },

};

const sensorMAP_testdata sensorMAP_testdata_cycle_minimum[] = {
  { .startRevolutions = 0, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 0, .engineProtectStatus = 1 },
  { .startRevolutions = 0, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 350,  .expected = 8675, .expectedADC = 351, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 310,  .expected = 7725, .expectedADC = 313, .errMAP = 0, .expectedLast = 8675, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 220,  .expected = 5575, .expectedADC = 227, .errMAP = 0, .expectedLast = 7725, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 110,  .expected = 2875, .expectedADC = 119, .errMAP = 0, .expectedLast = 5575, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 50,  .expected = 1275, .expectedADC = 55, .errMAP = 0, .expectedLast = 2875, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 50,  .expected = 1150, .expectedADC = 50, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 150,  .expected = 3450, .expectedADC = 142, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 280,  .expected = 6625, .expectedADC = 269, .errMAP = 0, .expectedLast = 3450, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 370,  .expected = 8950, .expectedADC = 362, .errMAP = 0, .expectedLast = 6625, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 370,  .expected = 9125, .expectedADC = 369, .errMAP = 0, .expectedLast = 8950, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 360,  .expected = 1150, .expectedADC = 360, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 350,  .expected = 1150, .expectedADC = 350, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 300,  .expected = 1150, .expectedADC = 303, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 1150, .expectedADC = 115, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 200,  .expected = 1150, .expectedADC = 193, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 100,  .expected = 1150, .expectedADC = 107, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 50,  .expected = 1150, .expectedADC = 54, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 100,  .expected = 1150, .expectedADC = 96, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 100,  .expected = 1150, .expectedADC = 99, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 0 },
  { .startRevolutions = 5, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 75,  .expected = 1150, .expectedADC = 76, .errMAP = 0, .expectedLast = 9125, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 75,  .expected = 1250, .expectedADC = 75, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 0 },
  { .startRevolutions = 6, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 200,  .expected = 1250, .expectedADC = 190, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 210,  .expected = 1250, .expectedADC = 208, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 6, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 210,  .expected = 1250, .expectedADC = 209, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 7, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 250,  .expected = 1250, .expectedADC = 246, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 1 },
  { .startRevolutions = 7, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 280,  .expected = 1250, .expectedADC = 277, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 0 },
  { .startRevolutions = 7, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 300,  .expected = 1250, .expectedADC = 298, .errMAP = 0, .expectedLast = 1150, .engineProtectStatus = 0 },
  { .startRevolutions = 8, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 280,  .expected = 1775, .expectedADC = 281, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 0 },
  { .startRevolutions = 8, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 290,  .expected = 1775, .expectedADC = 289, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 1 },
  { .startRevolutions = 9, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 3,  .expected = 1775, .expectedADC = 25, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 0 },
  { .startRevolutions = 9, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 4,  .expected = 1775, .expectedADC = 5, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 1 },
  { .startRevolutions = 9, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 10,  .expected = 1775, .expectedADC = 9, .errMAP = 0, .expectedLast = 1250, .engineProtectStatus = 0 },
  { .startRevolutions = 10, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 0,  .expected = 25, .expectedADC = 9, .errMAP = ERR_MAP_LOW, .expectedLast = 1775, .engineProtectStatus = 1 },
  { .startRevolutions = 10, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 250,  .expected = 25, .expectedADC = 231, .errMAP = 0, .expectedLast = 1775, .engineProtectStatus = 0 },
  { .startRevolutions = 11, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 750,  .expected = 25, .expectedADC = 709, .errMAP = 0, .expectedLast = 1775, .engineProtectStatus = 1 },
  { .startRevolutions = 11, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 0,  .expected = 25, .expectedADC = 709, .errMAP = ERR_MAP_LOW, .expectedLast = 1775, .engineProtectStatus = 1 },
  { .startRevolutions = 11, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 900,  .expected = 25, .expectedADC = 885, .errMAP = 0, .expectedLast = 1775, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1021,  .expected = 5675, .expectedADC = 1010, .errMAP = 0, .expectedLast = 25, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1024,  .expected = 5675, .expectedADC = 1010, .errMAP = ERR_MAP_HIGH, .expectedLast = 25, .engineProtectStatus = 0 },
  { .startRevolutions = 12, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 900,  .expected = 5675, .expectedADC = 908, .errMAP = 0, .expectedLast = 25, .engineProtectStatus = 1 },
  { .startRevolutions = 13, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 500,  .expected = 5675, .expectedADC = 531, .errMAP = 0, .expectedLast = 25, .engineProtectStatus = 0 },
  { .startRevolutions = 13, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 0,  .expected = 5675, .expectedADC = 531, .errMAP = ERR_MAP_LOW, .expectedLast = 25, .engineProtectStatus = 0 },
  { .startRevolutions = 14, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 13175, .expectedADC = 133, .errMAP = 0, .expectedLast = 5675, .engineProtectStatus = 1 },
  { .startRevolutions = 14, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1024,  .expected = 13175, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 14, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 1024,  .expected = 13175, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 5675, .engineProtectStatus = 1 },
  { .startRevolutions = 15, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 0,  .expected = 13175, .expectedADC = 133, .errMAP = ERR_MAP_LOW, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 15, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 13175, .expectedADC = 471, .errMAP = 0, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 15, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 13175, .expectedADC = 128, .errMAP = 0, .expectedLast = 5675, .engineProtectStatus = 1 },
  { .startRevolutions = 16, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 500,  .expected = 3100, .expectedADC = 470, .errMAP = 0, .expectedLast = 13175, .engineProtectStatus = 0 },
  { .startRevolutions = 17, .ignitionCount = 2, .RPMdiv100 = 11, .testInput = 700,  .expected = 16950, .expectedADC = 682, .errMAP = 0, .expectedLast = 3100, .engineProtectStatus = 1 },
  { .startRevolutions = 18, .ignitionCount = 3, .RPMdiv100 = 11, .testInput = 800,  .expected = 19650, .expectedADC = 790, .errMAP = 0, .expectedLast = 16950, .engineProtectStatus = 1 },
  { .startRevolutions = 19, .ignitionCount = 1, .RPMdiv100 = 11, .testInput = 1000,  .expected = 24475, .expectedADC = 983, .errMAP = 0, .expectedLast = 19650, .engineProtectStatus = 1 },
  { .startRevolutions = 20, .ignitionCount = 3, .RPMdiv100 = 11, .testInput = 900,  .expected = 22550, .expectedADC = 906, .errMAP = 0, .expectedLast = 24475, .engineProtectStatus = 1 },
  { .startRevolutions = 21, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 700,  .expected = 17800, .expectedADC = 716, .errMAP = 0, .expectedLast = 22550, .engineProtectStatus = 1 },
  { .startRevolutions = 22, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 17800, .expectedADC = 516, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 1 },
  { .startRevolutions = 23, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 600,  .expected = 17800, .expectedADC = 593, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 0 },
  { .startRevolutions = 24, .ignitionCount = 0, .RPMdiv100 = 10, .testInput = 700,  .expected = 17175, .expectedADC = 691, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 1 },
  { .startRevolutions = 25, .ignitionCount = 4, .RPMdiv100 = 10, .testInput = 800,  .expected = 19675, .expectedADC = 791, .errMAP = 0, .expectedLast = 17175, .engineProtectStatus = 1 },
  { .startRevolutions = 26, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 900,  .expected = 17175, .expectedADC = 891, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 0 },
  { .startRevolutions = 27, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 100,  .expected = 17175, .expectedADC = 161, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 1 },
  { .startRevolutions = 28, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 200,  .expected = 3925, .expectedADC = 196, .errMAP = 0, .expectedLast = 17175, .engineProtectStatus = 1 },
  { .startRevolutions = 29, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 300,  .expected = 3925, .expectedADC = 291, .errMAP = 0, .expectedLast = 17175, .engineProtectStatus = 1 },
  { .startRevolutions = 30, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 0,  .expected = 4800, .expectedADC = 291, .errMAP = ERR_MAP_LOW, .expectedLast = 3925, .engineProtectStatus = 1 },
  { .startRevolutions = 31, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 500,  .expected = 4800, .expectedADC = 483, .errMAP = 0, .expectedLast = 3925, .engineProtectStatus = 1 },
  { .startRevolutions = 32, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 1021,  .expected = 11975, .expectedADC = 978, .errMAP = 0, .expectedLast = 4800, .engineProtectStatus = 1 },
  { .startRevolutions = 33, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 500,  .expected = 11975, .expectedADC = 537, .errMAP = 0, .expectedLast = 4800, .engineProtectStatus = 1 },
  { .startRevolutions = 34, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 0,  .expected = 13325, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 11975, .engineProtectStatus = 0 },
  { .startRevolutions = 35, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 0,  .expected = 13325, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 11975, .engineProtectStatus = 1 },
  { .startRevolutions = 36, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 0,  .expected = -1, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 13325, .engineProtectStatus = 1 },
  { .startRevolutions = 37, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 500,  .expected = 12450, .expectedADC = 502, .errMAP = 0, .expectedLast = -1, .engineProtectStatus = 0 },
};

const sensorMAP_testdata sensorMAP_testdata_ignition_average[] = {
  { .startRevolutions = 4, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 0, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 0, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 370,  .expected = 9150, .expectedADC = 370, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 1, .RPMdiv100 = 15, .testInput = 350,  .expected = 8675, .expectedADC = 351, .errMAP = 0, .expectedLast = 9150, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 310,  .expected = 7725, .expectedADC = 313, .errMAP = 0, .expectedLast = 8675, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 220,  .expected = 5575, .expectedADC = 227, .errMAP = 0, .expectedLast = 7725, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 110,  .expected = 2875, .expectedADC = 119, .errMAP = 0, .expectedLast = 5575, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 2, .RPMdiv100 = 15, .testInput = 50,  .expected = 1275, .expectedADC = 55, .errMAP = 0, .expectedLast = 2875, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 50,  .expected = 4350, .expectedADC = 50, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 150,  .expected = 4350, .expectedADC = 142, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 280,  .expected = 4350, .expectedADC = 269, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 370,  .expected = 4350, .expectedADC = 362, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 3, .RPMdiv100 = 15, .testInput = 370,  .expected = 4350, .expectedADC = 369, .errMAP = 0, .expectedLast = 1275, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 360,  .expected = 5850, .expectedADC = 360, .errMAP = 0, .expectedLast = 4350, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 350,  .expected = 5850, .expectedADC = 350, .errMAP = 0, .expectedLast = 4350, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 4, .RPMdiv100 = 15, .testInput = 300,  .expected = 5850, .expectedADC = 303, .errMAP = 0, .expectedLast = 4350, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 100,  .expected = 8325, .expectedADC = 115, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 200,  .expected = 8325, .expectedADC = 193, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 100,  .expected = 8325, .expectedADC = 107, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 50,  .expected = 8325, .expectedADC = 54, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 100,  .expected = 8325, .expectedADC = 96, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 100,  .expected = 8325, .expectedADC = 99, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 5, .RPMdiv100 = 15, .testInput = 75,  .expected = 8325, .expectedADC = 76, .errMAP = 0, .expectedLast = 5850, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 6, .RPMdiv100 = 15, .testInput = 75,  .expected = 2525, .expectedADC = 75, .errMAP = 0, .expectedLast = 8325, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 6, .RPMdiv100 = 15, .testInput = 200,  .expected = 2525, .expectedADC = 190, .errMAP = 0, .expectedLast = 8325, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 6, .RPMdiv100 = 15, .testInput = 210,  .expected = 2525, .expectedADC = 208, .errMAP = 0, .expectedLast = 8325, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 6, .RPMdiv100 = 15, .testInput = 210,  .expected = 2525, .expectedADC = 209, .errMAP = 0, .expectedLast = 8325, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 7, .RPMdiv100 = 15, .testInput = 250,  .expected = 4150, .expectedADC = 246, .errMAP = 0, .expectedLast = 2525, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 7, .RPMdiv100 = 15, .testInput = 280,  .expected = 4150, .expectedADC = 277, .errMAP = 0, .expectedLast = 2525, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 7, .RPMdiv100 = 15, .testInput = 300,  .expected = 4150, .expectedADC = 298, .errMAP = 0, .expectedLast = 2525, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 8, .RPMdiv100 = 15, .testInput = 280,  .expected = 6725, .expectedADC = 281, .errMAP = 0, .expectedLast = 4150, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 8, .RPMdiv100 = 15, .testInput = 290,  .expected = 6725, .expectedADC = 289, .errMAP = 0, .expectedLast = 4150, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 9, .RPMdiv100 = 15, .testInput = 3,  .expected = 7025, .expectedADC = 25, .errMAP = 0, .expectedLast = 6725, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 9, .RPMdiv100 = 15, .testInput = 4,  .expected = 7025, .expectedADC = 5, .errMAP = 0, .expectedLast = 6725, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 9, .RPMdiv100 = 15, .testInput = 10,  .expected = 7025, .expectedADC = 9, .errMAP = 0, .expectedLast = 6725, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 10, .RPMdiv100 = 15, .testInput = 0,  .expected = 225, .expectedADC = 9, .errMAP = ERR_MAP_LOW, .expectedLast = 7025, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 10, .RPMdiv100 = 15, .testInput = 250,  .expected = 225, .expectedADC = 231, .errMAP = 0, .expectedLast = 7025, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 11, .RPMdiv100 = 15, .testInput = 750,  .expected = 5675, .expectedADC = 709, .errMAP = 0, .expectedLast = 225, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 11, .RPMdiv100 = 15, .testInput = 0,  .expected = 5675, .expectedADC = 709, .errMAP = ERR_MAP_LOW, .expectedLast = 225, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 11, .RPMdiv100 = 15, .testInput = 900,  .expected = 5675, .expectedADC = 885, .errMAP = 0, .expectedLast = 225, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 12, .RPMdiv100 = 15, .testInput = 1021,  .expected = 19825, .expectedADC = 1010, .errMAP = 0, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 12, .RPMdiv100 = 15, .testInput = 1024,  .expected = 19825, .expectedADC = 1010, .errMAP = ERR_MAP_HIGH, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 12, .RPMdiv100 = 15, .testInput = 900,  .expected = 19825, .expectedADC = 908, .errMAP = 0, .expectedLast = 5675, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 13, .RPMdiv100 = 15, .testInput = 500,  .expected = 23875, .expectedADC = 531, .errMAP = 0, .expectedLast = 19825, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 13, .RPMdiv100 = 15, .testInput = 0,  .expected = 23875, .expectedADC = 531, .errMAP = ERR_MAP_LOW, .expectedLast = 19825, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 14, .RPMdiv100 = 15, .testInput = 100,  .expected = 13175, .expectedADC = 133, .errMAP = 0, .expectedLast = 23875, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 14, .RPMdiv100 = 15, .testInput = 1024,  .expected = 13175, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 23875, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 14, .RPMdiv100 = 15, .testInput = 1024,  .expected = 13175, .expectedADC = 133, .errMAP = ERR_MAP_HIGH, .expectedLast = 23875, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 15, .RPMdiv100 = 15, .testInput = 0,  .expected = 3225, .expectedADC = 133, .errMAP = ERR_MAP_LOW, .expectedLast = 13175, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 15, .RPMdiv100 = 15, .testInput = 500,  .expected = 3225, .expectedADC = 471, .errMAP = 0, .expectedLast = 13175, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 15, .RPMdiv100 = 15, .testInput = 100,  .expected = 3225, .expectedADC = 128, .errMAP = 0, .expectedLast = 13175, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 16, .RPMdiv100 = 15, .testInput = 500,  .expected = 7375, .expectedADC = 470, .errMAP = 0, .expectedLast = 3225, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 17, .RPMdiv100 = 11, .testInput = 700,  .expected = 16950, .expectedADC = 682, .errMAP = 0, .expectedLast = 7375, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 18, .RPMdiv100 = 11, .testInput = 800,  .expected = 19650, .expectedADC = 790, .errMAP = 0, .expectedLast = 16950, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 19, .RPMdiv100 = 11, .testInput = 1000,  .expected = 24475, .expectedADC = 983, .errMAP = 0, .expectedLast = 19650, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 20, .RPMdiv100 = 11, .testInput = 900,  .expected = 22550, .expectedADC = 906, .errMAP = 0, .expectedLast = 24475, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 21, .RPMdiv100 = 15, .testInput = 700,  .expected = 22550, .expectedADC = 716, .errMAP = 0, .expectedLast = 22550, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 22, .RPMdiv100 = 15, .testInput = 500,  .expected = 17800, .expectedADC = 516, .errMAP = 0, .expectedLast = 22550, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 23, .RPMdiv100 = 15, .testInput = 600,  .expected = 12800, .expectedADC = 593, .errMAP = 0, .expectedLast = 17800, .engineProtectStatus = 0 },
  { .startRevolutions = 0, .ignitionCount = 24, .RPMdiv100 = 10, .testInput = 700,  .expected = 17175, .expectedADC = 691, .errMAP = 0, .expectedLast = 12800, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 25, .RPMdiv100 = 10, .testInput = 800,  .expected = 19675, .expectedADC = 791, .errMAP = 0, .expectedLast = 17175, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 26, .RPMdiv100 = 15, .testInput = 900,  .expected = 19675, .expectedADC = 891, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 27, .RPMdiv100 = 15, .testInput = 100,  .expected = 22175, .expectedADC = 161, .errMAP = 0, .expectedLast = 19675, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 28, .RPMdiv100 = 15, .testInput = 200,  .expected = 3925, .expectedADC = 196, .errMAP = 0, .expectedLast = 22175, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 29, .RPMdiv100 = 15, .testInput = 300,  .expected = 4800, .expectedADC = 291, .errMAP = 0, .expectedLast = 3925, .engineProtectStatus = 0 },
  { .startRevolutions = 1, .ignitionCount = 30, .RPMdiv100 = 15, .testInput = 0,  .expected = 7175, .expectedADC = 291, .errMAP = ERR_MAP_LOW, .expectedLast = 4800, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 31, .RPMdiv100 = 15, .testInput = 500,  .expected = 11975, .expectedADC = 483, .errMAP = 0, .expectedLast = 7175, .engineProtectStatus = 0 },
  { .startRevolutions = 3, .ignitionCount = 32, .RPMdiv100 = 15, .testInput = 1021,  .expected = 11975, .expectedADC = 978, .errMAP = 0, .expectedLast = 11975, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 33, .RPMdiv100 = 15, .testInput = 500,  .expected = 24350, .expectedADC = 537, .errMAP = 0, .expectedLast = 11975, .engineProtectStatus = 0 },
  { .startRevolutions = 4, .ignitionCount = 34, .RPMdiv100 = 15, .testInput = 0,  .expected = 13325, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 24350, .engineProtectStatus = 0 },
  { .startRevolutions = 2, .ignitionCount = 35, .RPMdiv100 = 15, .testInput = 0,  .expected = -1, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = 13325, .engineProtectStatus = 1 },
  { .startRevolutions = 3, .ignitionCount = 36, .RPMdiv100 = 15, .testInput = 0,  .expected = -1, .expectedADC = 537, .errMAP = ERR_MAP_LOW, .expectedLast = -1, .engineProtectStatus = 1 },
  { .startRevolutions = 1, .ignitionCount = 37, .RPMdiv100 = 15, .testInput = 500,  .expected = 12450, .expectedADC = 502, .errMAP = 0, .expectedLast = -1, .engineProtectStatus = 0 },
};

const sensorMAP_testsetting sensorMAP_testsettings[] = {
  // Instantaneous
  { .whichSensor = testMAP,  .sensorMin = -100, .sensorMax = 25500, .mapSample = 0, .testdata = sensorMAP_testdata_instantaneous, countof(sensorMAP_testdata_instantaneous) },
  { .whichSensor = testEMAP, .sensorMin = -100, .sensorMax = 25500, .mapSample = 0, .testdata = sensorMAP_testdata_instantaneous, countof(sensorMAP_testdata_instantaneous) },
  // Cycle average
  { .whichSensor = testMAP,  .sensorMin = -100, .sensorMax = 25500, .mapSample = 1, .testdata = sensorMAP_testdata_cycle_average, countof(sensorMAP_testdata_cycle_average) },
  { .whichSensor = testEMAP, .sensorMin = -100, .sensorMax = 25500, .mapSample = 1, .testdata = sensorMAP_testdata_cycle_average, countof(sensorMAP_testdata_cycle_average) },
  // Cycle minimum
  { .whichSensor = testMAP,  .sensorMin = -100, .sensorMax = 25500, .mapSample = 2, .testdata = sensorMAP_testdata_cycle_minimum, countof(sensorMAP_testdata_cycle_minimum) },
  { .whichSensor = testEMAP, .sensorMin = -100, .sensorMax = 25500, .mapSample = 2, .testdata = sensorMAP_testdata_cycle_minimum, countof(sensorMAP_testdata_cycle_minimum) },
  // Ignition average
  { .whichSensor = testMAP,  .sensorMin = -100, .sensorMax = 25500, .mapSample = 3, .testdata = sensorMAP_testdata_ignition_average, countof(sensorMAP_testdata_ignition_average) },
  { .whichSensor = testEMAP, .sensorMin = -100, .sensorMax = 25500, .mapSample = 3, .testdata = sensorMAP_testdata_ignition_average, countof(sensorMAP_testdata_ignition_average) },
};

void testSensorMAP_RunTestMAP() {
  TEST_ASSERT_EQUAL_MESSAGE(sensorMAP_testdata_current->expectedADC, currentStatus.mapADC, "ADC");

  int16_t expectedCorrected = sensorMAP_testdata_current->expected;
  if (expectedCorrected == -1) { expectedCorrected = ERR_DEFAULT_MAP_LOW; }
  TEST_ASSERT_EQUAL_MESSAGE(expectedCorrected, currentStatus.MAP, "MAP");

  int16_t expectedLastCorrected = sensorMAP_testdata_current->expectedLast;
  if (expectedLastCorrected == -1) { expectedLastCorrected = ERR_DEFAULT_MAP_LOW; }
  TEST_ASSERT_EQUAL_MESSAGE(expectedLastCorrected, MAPlast, "MAPlast");

  byte error = 0;
  if ( hasError(ERR_MAP_LOW) ) { error += ERR_MAP_LOW; }
  if ( hasError(ERR_MAP_HIGH) ) { error += ERR_MAP_HIGH; }

  TEST_ASSERT_EQUAL_MESSAGE(sensorMAP_testdata_current->errMAP, error, "hasError");
}

void testSensorMAP_RunTestEMAP() {
  TEST_ASSERT_EQUAL_MESSAGE(sensorMAP_testdata_current->expectedADC, currentStatus.EMAPADC, "EMAPADC");

  int16_t expectedCorrected = sensorMAP_testdata_current->expected;
  if (expectedCorrected == -1) { expectedCorrected = ERR_DEFAULT_EMAP_LOW; }
  TEST_ASSERT_EQUAL_MESSAGE(expectedCorrected, currentStatus.EMAP, "EMAP");
  
  byte error = 0;
  if ( hasError(ERR_EMAP_LOW) ) { error += ERR_EMAP_LOW; }
  if ( hasError(ERR_EMAP_HIGH) ) { error += ERR_EMAP_HIGH; }
  
  byte checkForError = 0;
  if (sensorMAP_testdata_current->errMAP > 0) { checkForError = sensorMAP_testdata_current->errMAP + 2; } // the testdata is for MAP and the EMAP errors are offset by 2

  TEST_ASSERT_EQUAL_MESSAGE(checkForError, error, "hasError");
}

void testSensorMAP_RunTestData(byte testSettingNumber, byte testSettingData) {
  const byte testNameLength = 200;
  char testName[testNameLength];

  currentStatus.startRevolutions = sensorMAP_testdata_current->startRevolutions;
  ignitionCount = sensorMAP_testdata_current->ignitionCount;
  currentStatus.RPMdiv100 = sensorMAP_testdata_current->RPMdiv100;
  currentStatus.engineProtectStatus = sensorMAP_testdata_current->engineProtectStatus;

  if (sensorMAP_testsetting_current->whichSensor == testMAP) {
    unitTestMAPinput = sensorMAP_testdata_current->testInput;
    readManifoldPressures();
    snprintf(testName, testNameLength, "MAP/setting%u/data%u/rev%lu/ign%u/", testSettingNumber, testSettingData, currentStatus.startRevolutions, ignitionCount);
    UnityDefaultTestRun(testSensorMAP_RunTestMAP, testName, __LINE__);
  }
  else if (sensorMAP_testsetting_current->whichSensor == testEMAP) {
    unitTestEMAPinput = sensorMAP_testdata_current->testInput;
    readManifoldPressures();
    snprintf(testName, testNameLength, "EMAP/setting%u/data%u/rev%lu/ign%u/", testSettingNumber, testSettingData, currentStatus.startRevolutions, ignitionCount);
    UnityDefaultTestRun(testSensorMAP_RunTestEMAP, testName, __LINE__);
  }
}

void testSensorMAP_RunTestSettings(byte testSettingNumber) {

  if (sensorMAP_testsetting_current->whichSensor == testMAP) {
    configPage2.mapMin = sensorMAP_testsetting_current->sensorMin;
    configPage2.mapMax = sensorMAP_testsetting_current->sensorMax;
    configPage6.useEMAP = false;
  }
  else if (sensorMAP_testsetting_current->whichSensor == testEMAP) {
    configPage2.EMAPMin = sensorMAP_testsetting_current->sensorMin;
    configPage2.EMAPMax = sensorMAP_testsetting_current->sensorMax;
    configPage6.useEMAP = true;
  }
  configPage2.mapSample = sensorMAP_testsetting_current->mapSample;

  for (int i = 0; i < sensorMAP_testsetting_current->testdataCount; i++) {
    sensorMAP_testdata_current = &sensorMAP_testsetting_current->testdata[i];
    testSensorMAP_RunTestData(testSettingNumber, i);
    initialisationComplete = true; // First call is with this off (call from readBaro)
  }
  
}

void testSensorMAP_ResetTest() {
  initialisationComplete = false;
  unitTestMAPinput = 0;
  unitTestEMAPinput = 0;
  configPage2.mapMin = 0;
  configPage2.mapMax = 0;
  configPage2.EMAPMin = 0;
  configPage2.EMAPMax = 0;
  currentStatus.MAP = 0;
  currentStatus.mapADC = 0;
  currentStatus.EMAP = 0;
  currentStatus.EMAPADC = 0;
  clearError(ERR_MAP_HIGH);
  clearError(ERR_MAP_LOW);
  clearError(ERR_EMAP_HIGH);
  clearError(ERR_EMAP_LOW);
  MAPlast = 0;
  MAPlast_time = 0;
  MAP_time = 0;
  instantaneousMAP = true;
  instantaneousEMAP = true;
  configPage6.useEMAP = false;
  ignitionCount = 0;
  currentStatus.RPMdiv100 = 0;
  configPage2.mapSample = 0;
  currentStatus.startRevolutions = 0;
  MAPsamplingNext = 0;
  MAPsamplingRunningValue = 0;
  MAPsamplingCount = 0;
  EMAPsamplingRunningValue  = 0;
  currentStatus.engineProtectStatus = 0;
}

void testSensorMAP() {

  // No need to test different values for these
  configPage2.mapSwitchPoint = 12;
  configPage4.ADCFILTER_MAP = 20;
  configPage2.strokes = 0; // 4-stroke

  byte i = 0;
  for (auto testSetting : sensorMAP_testsettings) {
    sensorMAP_testsetting_current = &testSetting;
    testSensorMAP_RunTestSettings(i);
    testSensorMAP_ResetTest();
    i++;
  }

}