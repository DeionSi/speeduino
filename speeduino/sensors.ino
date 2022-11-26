/*
Speeduino - Simple engine management for the Arduino Mega 2560 platform
Copyright (C) Josh Stewart
A full copy of the license may be found in the projects root directory
*/
/** @file
 * Read sensors with appropriate timing / scheduling.
 */
#include "sensors.h"
#include "crankMaths.h"
#include "globals.h"
#include "maths.h"
#include "storage.h"
#include "comms.h"
#include "idle.h"
#include "errors.h"
#include "corrections.h"
#include "pages.h"
#include "decoders.h"
#include "utilities.h"
#include "limits.h"

// Global variables
unsigned long MAPsamplingRunningValue; //Used for tracking either the total of all MAP readings in this cycle (Event average) or the lowest value detected in this cycle (event minimum)
unsigned long EMAPsamplingRunningValue; //As above but for EMAP
unsigned int MAPsamplingCount; //Number of samples taken in the current MAP cycle
unsigned int EMAPsamplingCount; //Number of samples taken in the current EMAP cycle
uint32_t MAPsamplingNext; // On which revolution/ignition the next MAP value will be set for sampling methods.
bool auxIsEnabled;
uint16_t MAPlast; /**< The previous MAP reading */
unsigned long MAP_time; //The time the MAP sample was taken
unsigned long MAPlast_time; //The time the previous MAP sample was taken


volatile unsigned long vssTimes[VSS_SAMPLES];
volatile byte vssIndex;

volatile byte flexCounter;
volatile unsigned long flexStartTime;
volatile unsigned long flexPulseWidth;

volatile byte knockCounter;
volatile uint16_t knockAngle;

instantaneousManifoldPressure instantaneous { .MAP = true, .EMAP = true };

#ifdef UNIT_TEST
uint16_t unitTestMAPinput;
uint16_t unitTestEMAPinput;
#endif

//These variables are used for tracking the number of running sensors values that appear to be errors. Once a threshold is reached, the sensor reading will go to default value and assume the sensor is faulty
byte iatErrorCount;
byte cltErrorCount;

/** Init all ADC conversions by setting resolutions, etc.
 */
void initialiseADC(void)
{
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this

  #if defined(ANALOG_ISR)
    //This sets the ADC (Analog to Digital Converter) to run at 250KHz, greatly reducing analog read times (MAP/TPS)
    //the code on ISR run each conversion every 25 ADC clock, conversion run about 100KHz effectively
    //making a 6250 conversions/s on 16 channels and 12500 on 8 channels devices.
    noInterrupts(); //Interrupts should be turned off when playing with any of these registers

    ADCSRB = 0x00; //ADC Auto Trigger Source is in Free Running mode
    ADMUX = 0x40;  //Select AREF as reference, ADC Left Adjust Result, Starting at channel 0

    //All of the below is the longhand version of: ADCSRA = 0xEE;
    #define ADFR 5 //Why the HELL isn't this defined in the same place as everything else (wiring.h)?!?!
    BIT_SET(ADCSRA,ADFR); //Set free running mode
    BIT_SET(ADCSRA,ADIE); //Set ADC interrupt enabled
    BIT_CLEAR(ADCSRA,ADIF); //Clear interrupt flag

    // Set ADC clock to 125KHz (Prescaler = 128)
    BIT_SET(ADCSRA,ADPS2);
    BIT_SET(ADCSRA,ADPS1);
    BIT_SET(ADCSRA,ADPS0);

    BIT_SET(ADCSRA,ADEN); //Enable ADC

    interrupts();
    BIT_SET(ADCSRA,ADSC); //Start conversion

  #else
    //This sets the ADC (Analog to Digital Converter) to run at 1Mhz, greatly reducing analog read times (MAP/TPS) when using the standard analogRead() function
    //1Mhz is the fastest speed permitted by the CPU without affecting accuracy
    //Please see chapter 11 of 'Practical Arduino' (books.google.com.au/books?id=HsTxON1L6D4C&printsec=frontcover#v=onepage&q&f=false) for more detail
     BIT_SET(ADCSRA,ADPS2);
     BIT_CLEAR(ADCSRA,ADPS1);
     BIT_CLEAR(ADCSRA,ADPS0);
  #endif
#elif defined(ARDUINO_ARCH_STM32) //STM32GENERIC core and ST STM32duino core, change analog read to 12 bit
  analogReadResolution(10); //use 10bits for analog reading on STM32 boards
#endif
  MAPsamplingNext = 0;
  MAPsamplingCount = 0;
  EMAPsamplingCount = 0;
  MAPsamplingRunningValue = 0;
  EMAPsamplingRunningValue = 0;

  //The following checks the aux inputs and initialises pins if required
  auxIsEnabled = false;
  for (byte AuxinChan = 0; AuxinChan <16 ; AuxinChan++)
  {
    currentStatus.current_caninchannel = AuxinChan;                   
    if (((configPage9.caninput_sel[currentStatus.current_caninchannel]&12) == 4)
    && ((configPage9.enable_secondarySerial == 1) || ((configPage9.enable_intcan == 1) && (configPage9.intcan_available == 1))))
    { //if current input channel is enabled as external input in caninput_selxb(bits 2:3) and secondary serial or internal canbus is enabled(and is mcu supported)                 
      //currentStatus.canin[14] = 22;  Dev test use only!
      auxIsEnabled = true;     
    }
    else if ((((configPage9.enable_secondarySerial == 1) || ((configPage9.enable_intcan == 1) && (configPage9.intcan_available == 1))) && (configPage9.caninput_sel[currentStatus.current_caninchannel]&12) == 8)
            || (((configPage9.enable_secondarySerial == 0) && ( (configPage9.enable_intcan == 1) && (configPage9.intcan_available == 0) )) && (configPage9.caninput_sel[currentStatus.current_caninchannel]&3) == 2)  
            || (((configPage9.enable_secondarySerial == 0) && (configPage9.enable_intcan == 0)) && ((configPage9.caninput_sel[currentStatus.current_caninchannel]&3) == 2)))  
    {  //if current input channel is enabled as analog local pin check caninput_selxb(bits 2:3) with &12 and caninput_selxa(bits 0:1) with &3
      byte pinNumber = pinTranslateAnalog(configPage9.Auxinpina[currentStatus.current_caninchannel]&63);
      if( pinIsUsed(pinNumber) )
      {
        //Do nothing here as the pin is already in use.
        BIT_SET(currentStatus.engineProtectStatus, PROTECT_IO_ERROR); //Tell user that there is problem by lighting up the I/O error indicator
      }
      else
      {
        //Channel is active and analog
        pinMode( pinNumber, INPUT);
        //currentStatus.canin[14] = 33;  Dev test use only!
        auxIsEnabled = true;
      }  
    }
    else if ((((configPage9.enable_secondarySerial == 1) || ((configPage9.enable_intcan == 1) && (configPage9.intcan_available == 1))) && (configPage9.caninput_sel[currentStatus.current_caninchannel]&12) == 12)
            || (((configPage9.enable_secondarySerial == 0) && ( (configPage9.enable_intcan == 1) && (configPage9.intcan_available == 0) )) && (configPage9.caninput_sel[currentStatus.current_caninchannel]&3) == 3)
            || (((configPage9.enable_secondarySerial == 0) && (configPage9.enable_intcan == 0)) && ((configPage9.caninput_sel[currentStatus.current_caninchannel]&3) == 3)))
    {  //if current input channel is enabled as digital local pin check caninput_selxb(bits 2:3) with &12 and caninput_selxa(bits 0:1) with &3
       byte pinNumber = (configPage9.Auxinpinb[currentStatus.current_caninchannel]&63) + 1;
       if( pinIsUsed(pinNumber) )
       {
         //Do nothing here as the pin is already in use.
        BIT_SET(currentStatus.engineProtectStatus, PROTECT_IO_ERROR); //Tell user that there is problem by lighting up the I/O error indicator
       }
       else
       {
         //Channel is active and digital
         pinMode( pinNumber, INPUT);
         //currentStatus.canin[14] = 44;  Dev test use only!
         auxIsEnabled = true;
       }  

    }
  } //For loop iterating through aux in lines

  //Sanity checks to ensure none of the filter values are set above 240 (Which would include the 255 value which is the default on a new arduino)
  //If an invalid value is detected, it's reset to the default the value and burned to EEPROM. 
  //Each sensor has it's own default value
  if(configPage4.ADCFILTER_TPS  > 240) { configPage4.ADCFILTER_TPS   = ADCFILTER_TPS_DEFAULT;   writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_CLT  > 240) { configPage4.ADCFILTER_CLT   = ADCFILTER_CLT_DEFAULT;   writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_IAT  > 240) { configPage4.ADCFILTER_IAT   = ADCFILTER_IAT_DEFAULT;   writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_O2   > 240) { configPage4.ADCFILTER_O2    = ADCFILTER_O2_DEFAULT;    writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_BAT  > 240) { configPage4.ADCFILTER_BAT   = ADCFILTER_BAT_DEFAULT;   writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_MAP  > 240) { configPage4.ADCFILTER_MAP   = ADCFILTER_MAP_DEFAULT;   writeConfig(ignSetPage); }
  if(configPage4.ADCFILTER_BARO > 240) { configPage4.ADCFILTER_BARO  = ADCFILTER_BARO_DEFAULT;  writeConfig(ignSetPage); }
  if(configPage4.FILTER_FLEX    > 240) { configPage4.FILTER_FLEX     = FILTER_FLEX_DEFAULT;     writeConfig(ignSetPage); }

  flexStartTime = micros();

  vssIndex = 0;
}

/* readMAP changes:
When readMAP is called for running value methods when the cycle/event ends.
- The map sensor value was not used, it is now read and stored.
- The ADC value was overwritten with the cycle/event calculation result, now the previous ADC filter result is used instead.
When switching from instantaneous to running value, the last instantaneous value was used as the running value result until the first full cycle/event. Now instantaneous continues to be used until a full cycle/event has been completed.
EMAPsamplingCount has been added. It was possible for MAPsamplingCount to be incorrect for EMAP if an invalid MAP or EMAP value was read.
EMAP added to event minimum and ignition average.
2-stroke now samples over one revolution, 4-stroke continues to use two
Default MAP value for values over VALID_MAP_MAX is now the configured max for the MAP sensor rather than ERR_DEFAULT_MAP_MAX.
*/

//#include "unity.h"

inline byte validateEMAPadc(uint16_t &EMAPadc)
{
  byte resultError = 0;

  //Error checks
  if (EMAPadc < VALID_MAP_MIN)
  {
    setError(ERR_EMAP_LOW);
    resultError = ERR_EMAP_LOW;
  }
  else if (errorCount > 0) {
    clearError(ERR_EMAP_LOW);
  }

  if (EMAPadc > VALID_MAP_MAX)
  {
    setError(ERR_EMAP_HIGH);
    resultError = ERR_EMAP_HIGH;
  }
  else if (errorCount > 0) {
    clearError(ERR_EMAP_HIGH);
  }

  return resultError;
}

// This function should/must only be called from readMAP()
void readEMAP(bool applyFilter)
{
  // Comments are the same as in the readMAP function
  
  uint16_t tempReading;

  #ifndef UNIT_TEST
    tempReading = analogRead(pinEMAP);
    tempReading = analogRead(pinEMAP);
  #else
    tempReading = unitTestEMAPinput;
  #endif

  byte EMAPADCerror = validateEMAPadc(tempReading);


  if ( EMAPADCerror == 0 ) {
    if ( applyFilter == true ) { currentStatus.EMAPADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_MAP, currentStatus.EMAPADC); }
    else { currentStatus.EMAPADC = tempReading; }
  }

  /*******************************************/

  switch (configPage2.mapSample)
  {
    case 0: //Instantaneous MAP readings
      instantaneous.EMAP = true;
      break;

    case 1: //Average of a cycle
      
      if ( currentStatus.startRevolutions >= MAP_SAMPLING_COUNT_START ) {
        
        if ( currentStatus.startRevolutions >= MAPsamplingNext ) {

          if ( EMAPsamplingRunningValue > 0 && EMAPsamplingCount > 0 && currentStatus.RPMdiv100 >= configPage2.mapSwitchPoint ) {
            instantaneous.EMAP = false;

            currentStatus.EMAP = fastMap10Bit( ldiv(EMAPsamplingRunningValue, EMAPsamplingCount).quot, configPage2.EMAPMin, configPage2.EMAPMax );
          }
          else {
            instantaneous.EMAP = true;
          }

          EMAPsamplingRunningValue = 0;
          EMAPsamplingCount = 0;
        }
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.EMAP = true;
        }

        if( EMAPADCerror == 0 )
        {
          EMAPsamplingRunningValue += currentStatus.EMAPADC;
          EMAPsamplingCount++;
          if (EMAPsamplingCount == UINT_MAX) { EMAPsamplingCount /= 2; EMAPsamplingRunningValue /= 2; }
        }
      }
      else {
        EMAPsamplingRunningValue = 0;
        EMAPsamplingCount = 0;
        instantaneous.EMAP = true;
      }

      break;

    case 2: //Minimum reading in a cycle
      
      if ( currentStatus.startRevolutions >= MAP_SAMPLING_COUNT_START ) {
        
        if ( currentStatus.startRevolutions >= MAPsamplingNext ) {

          if ( EMAPsamplingRunningValue < 1023 && EMAPsamplingCount > 0 && currentStatus.RPMdiv100 > configPage2.mapSwitchPoint ) {
            instantaneous.EMAP = false;

            currentStatus.EMAP = fastMap10Bit( EMAPsamplingRunningValue, configPage2.EMAPMin, configPage2.EMAPMax );
          }
          else {
            instantaneous.EMAP = true;
          }

          EMAPsamplingRunningValue = 1023;
          EMAPsamplingCount = 0;
        }
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.EMAP = true;
        }

        if( EMAPADCerror == 0 )
        {
          if ( (unsigned long)currentStatus.EMAPADC < EMAPsamplingRunningValue ) { EMAPsamplingRunningValue = (unsigned long)currentStatus.EMAPADC; }
          EMAPsamplingCount = 1;
        }

      }
      else {
        EMAPsamplingRunningValue = 1023;
        EMAPsamplingCount = 0;
        instantaneous.EMAP = true;
      }

      break;

    case 3: //Average of an ignition event
      // TODO: test based in engine protect status
      if ( currentStatus.engineProtectStatus == 0 && ignitionCount >= MAP_SAMPLING_COUNT_START ) {

        if ( ignitionCount >= MAPsamplingNext ) {
          
          if ( EMAPsamplingRunningValue > 0 && EMAPsamplingCount > 0 && currentStatus.RPMdiv100 > configPage2.mapSwitchPoint ) {
            instantaneous.EMAP = false;

            currentStatus.EMAP = fastMap10Bit( ldiv(EMAPsamplingRunningValue, EMAPsamplingCount).quot, configPage2.EMAPMin, configPage2.EMAPMax );
          }
          else {
            instantaneous.EMAP = true;
          }
          
          EMAPsamplingRunningValue = 0;
          EMAPsamplingCount = 0;
        }
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.EMAP = true;
        }

        if( EMAPADCerror == 0 )
        {
          EMAPsamplingRunningValue += currentStatus.EMAPADC;
          EMAPsamplingCount++;
          if (EMAPsamplingCount == UINT_MAX) { EMAPsamplingCount /= 2; EMAPsamplingRunningValue /= 2; }
        }

      }
      else {
        EMAPsamplingRunningValue = 0;
        EMAPsamplingCount = 0;
        instantaneous.EMAP = true;
      }

      break; 

    default: //Instantaneous MAP readings (Just in case)
      instantaneous.EMAP = true;
      break;
  }

  if (instantaneous.EMAP == true) {

    if( EMAPADCerror == 0 ) {
      currentStatus.EMAP = fastMap10Bit(currentStatus.EMAPADC, configPage2.EMAPMin, configPage2.EMAPMax);
    }
    else if (EMAPADCerror == ERR_EMAP_LOW) {
      currentStatus.EMAP = ERR_DEFAULT_EMAP_LOW;
    }
    else if (EMAPADCerror == ERR_EMAP_HIGH) {
      currentStatus.EMAP = configPage2.EMAPMax;
    }
  }

  if ( currentStatus.EMAP < 0 ) { currentStatus.EMAP = 0; }

  //char debugMessage[200];
  //snprintf(debugMessage, 200, "EMAPsamplingRunningValue %lu EMAPsamplingCount %u   ", EMAPsamplingRunningValue, EMAPsamplingCount);
  //snprintf(debugMessage, 200, "mapADC %d validMAPadc %d  ", currentStatus.EMAPADC, validMAPadc);
  //snprintf(debugMessage, 200, "EMAPsamplingRunningValue %lu EMAPsamplingCount %u mapADC %d MAPADCerror %u tempReading %d errorCount %u  ", EMAPsamplingRunningValue, EMAPsamplingCount, currentStatus.EMAPADC, MAPADCerror, tempReading, errorCount);
  //UnityPrint(debugMessage);

}

inline byte validateMAPadc(uint16_t &MAPadc)
{
  byte resultError = 0;

  //Error checks
  if (MAPadc < VALID_MAP_MIN)
  {
    setError(ERR_MAP_LOW);
    resultError = ERR_MAP_LOW;
  }
  else if (errorCount > 0) {
    clearError(ERR_MAP_LOW);
  }

  if (MAPadc > VALID_MAP_MAX)
  {
    setError(ERR_MAP_HIGH);
    resultError = ERR_MAP_HIGH;
  }
  else if (errorCount > 0) {
    clearError(ERR_MAP_HIGH);
  }

  return resultError;
}

inline void readMAP(bool applyFilter)
{
  // readEMAP needs to be called first as readMAP updates MAPsamplingNext which is used by both
  if(configPage6.useEMAP == true) {
    readEMAP(applyFilter);
  }

  // Read the sensor and set mapADC

  uint16_t tempReading;

  #ifndef UNIT_TEST
    noInterrupts();
    tempReading = swfMAPlastInterval;
    interrupts();
    // 110 kPa = max freq 162,453hz = 6155,6us between triggers (one per hertz)
    // 0 kPa = min freq 77,533hz = 12897,7us between triggers (one per hertz)
    tempReading = (1000000UL*100) / tempReading; // Convert from us-between-triggers to frequency*100
    tempReading = map(tempReading, 7753, 16245, 0, 1023); // From input is frequency*100
  #else
    tempReading = unitTestMAPinput;
  #endif

  byte MAPADCerror = validateMAPadc(tempReading);
  
  if ( MAPADCerror == 0 ) {
    //During initialisation a call is made to get the baro reading from the MAP sensor. In this case, we can't apply the ADC filter
    if ( applyFilter == true ) { currentStatus.mapADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_MAP, currentStatus.mapADC); }
    else { currentStatus.mapADC = tempReading; } //Baro reading (No filter)
  }

  /*******************************************/

  switch (configPage2.mapSample) // Select MAP sampling system
  {
    case 0: //Instantaneous MAP readings
      instantaneous.MAP = true;
      break;

    // The comments for case 1 also apply to case 2 and 3.
    case 1: //Average of a cycle
      
      // Only start collecting values after second revolution so we get exactly full cycles and startup has sort of stabilised. startRevolutions > 1 implies we have sync. Otherwise reset variables and use instantaneous values
      if ( currentStatus.startRevolutions >= MAP_SAMPLING_COUNT_START ) {
        
        // Only try to calculate the average if one cycle has passed since the last calculation
        if ( currentStatus.startRevolutions >= MAPsamplingNext ) {

          // This if-else makes sure we have data to average over and switchpoint has been reached, otherwise revert to instantaneous values
          if ( MAPsamplingRunningValue > 0 && MAPsamplingCount > 0 && currentStatus.RPMdiv100 >= configPage2.mapSwitchPoint ) {
            instantaneous.MAP = false;

            // Update the calculation times and last value. These are used by the MAP based Accel enrich
            MAPlast = currentStatus.MAP;
            MAPlast_time = MAP_time;
            MAP_time = micros();

            currentStatus.MAP = fastMap10Bit( ldiv(MAPsamplingRunningValue, MAPsamplingCount).quot, configPage2.mapMin, configPage2.mapMax );
          }
          else {
            instantaneous.MAP = true;
          }

          // Reset average calculation variables for next cycle
          MAPsamplingNext = currentStatus.startRevolutions + ( configPage2.strokes == FOUR_STROKE ? 2 : 1); // One cycle is two revolutions for 4-stroke, one revolution for 2-stroke
          MAPsamplingRunningValue = 0;
          MAPsamplingCount = 0;
        }
        // Instantaneously switch to instantaneous MAP if we dip below the switch point during a cycle
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.MAP = true;
        }

        // Always collect (valid) values for averaging. It means we can switch to average as soon as the full cycle is concluded when switchpoint has been reached.
        if( MAPADCerror == 0 )
        {
          MAPsamplingRunningValue += currentStatus.mapADC; //Add the current reading onto the total
          MAPsamplingCount++;
          if (MAPsamplingCount == UINT_MAX) { MAPsamplingCount /= 2; MAPsamplingRunningValue /= 2; } // Prevent overflow if we for some reason read too many values, this should never happen
        }
      }
      else {
        MAPsamplingNext = 0;
        MAPsamplingRunningValue = 0;
        MAPsamplingCount = 0;
        instantaneous.MAP = true;
      }

      break;

    case 2: //Minimum reading in a cycle
      
      if ( currentStatus.startRevolutions >= MAP_SAMPLING_COUNT_START ) {
        
        if ( currentStatus.startRevolutions >= MAPsamplingNext ) {

          if ( MAPsamplingRunningValue < 1023 && MAPsamplingCount > 0 && currentStatus.RPMdiv100 > configPage2.mapSwitchPoint ) {
            instantaneous.MAP = false;

            MAPlast = currentStatus.MAP;
            MAPlast_time = MAP_time;
            MAP_time = micros();

            currentStatus.MAP = fastMap10Bit( MAPsamplingRunningValue, configPage2.mapMin, configPage2.mapMax );
          }
          else {
            instantaneous.MAP = true;
          }

          MAPsamplingNext = currentStatus.startRevolutions + ( configPage2.strokes == FOUR_STROKE ? 2 : 1); // One cycle is two revolutions for 4-stroke, one revolution for 2-stroke
          MAPsamplingRunningValue = 1023; //Reset to a large invalid value (> VALID_MAP_MAX) so the next reading will always be lower
          MAPsamplingCount = 0;
        }
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.MAP = true;
        }

        if( MAPADCerror == 0 )
        {
          if ( (unsigned long)currentStatus.mapADC < MAPsamplingRunningValue ) { MAPsamplingRunningValue = (unsigned long)currentStatus.mapADC; } // Update if the current reading is lower than the running minimum
          MAPsamplingCount = 1; // MAPsamplingCount only needs to reach 1, it's only checked for being greater than 0. Prevents possibility of unlikely overflow.
        }

      }
      else {
        MAPsamplingNext = 0;
        MAPsamplingRunningValue = 1023;
        MAPsamplingCount = 0;
        instantaneous.MAP = true;
      }

      break;

    case 3: //Average of an ignition event
      // TODO: test based in engine protect status
      if ( currentStatus.engineProtectStatus == 0 && ignitionCount >= MAP_SAMPLING_COUNT_START ) {

        if ( ignitionCount >= MAPsamplingNext ) {
          
          if ( MAPsamplingRunningValue > 0 && MAPsamplingCount > 0 && currentStatus.RPMdiv100 > configPage2.mapSwitchPoint ) {
            instantaneous.MAP = false;

            MAPlast = currentStatus.MAP;
            MAPlast_time = MAP_time;
            MAP_time = micros();

            currentStatus.MAP = fastMap10Bit( ldiv(MAPsamplingRunningValue, MAPsamplingCount).quot, configPage2.mapMin, configPage2.mapMax );
          }
          else {
            instantaneous.MAP = true;
          }
          
          MAPsamplingNext = ignitionCount + 1;
          MAPsamplingRunningValue = 0;
          MAPsamplingCount = 0;
        }
        else if (currentStatus.RPMdiv100 < configPage2.mapSwitchPoint) {
          instantaneous.MAP = true;
        }

        if( MAPADCerror == 0 )
        {
          MAPsamplingRunningValue += currentStatus.mapADC;
          MAPsamplingCount++;
          if (MAPsamplingCount == UINT_MAX) { MAPsamplingCount /= 2; MAPsamplingRunningValue /= 2; }
        }

      }
      else {
        MAPsamplingNext = 0;
        MAPsamplingRunningValue = 0;
        MAPsamplingCount = 0;
        instantaneous.MAP = true;
      }

      break; 

    default: //Instantaneous MAP readings (Just in case)
      instantaneous.MAP = true;
      break;
  }

  // Set MAP directly from mapADC if the result of an averaging method is not used
  if (instantaneous.MAP == true) {
    //Update the calculation times and last value. These are used by the MAP based Accel enrich
    MAPlast = currentStatus.MAP;
    MAPlast_time = MAP_time;
    MAP_time = micros();

    if( MAPADCerror == 0 ) { // Valid ADC value, use it
      currentStatus.MAP = fastMap10Bit(currentStatus.mapADC, configPage2.mapMin, configPage2.mapMax);
    }
    else if (MAPADCerror == ERR_MAP_LOW) { // Too low ADC value, use the default LOW. Failed sensor or open circuit.
      currentStatus.MAP = ERR_DEFAULT_MAP_LOW;
    }
    else if (MAPADCerror == ERR_MAP_HIGH) { // Too high ADC value, use the configured max value.
      currentStatus.MAP = configPage2.mapMax;
    }
  }

  if ( currentStatus.MAP < 0 ) { currentStatus.MAP = 0; } // Pressure can never be negative

  //char debugMessage[200];
  //snprintf(debugMessage, 200, "MAPsamplingRunningValue %lu MAPsamplingCount %u   ", MAPsamplingRunningValue, MAPsamplingCount);
  //snprintf(debugMessage, 200, "mapADC %d validMAPadc %d  ", currentStatus.mapADC, validMAPadc);
  //snprintf(debugMessage, 200, "MAPsamplingRunningValue %lu MAPsamplingCount %u mapADC %d MAPADCerror %u tempReading %d errorCount %u  ", MAPsamplingRunningValue, MAPsamplingCount, currentStatus.mapADC, MAPADCerror, tempReading, errorCount);
  //UnityPrint(debugMessage);
}

void readTPS(bool useFilter)
{
  currentStatus.TPSlast = currentStatus.TPS;
  #if defined(ANALOG_ISR)
    byte tempTPS = fastMap1023toX(AnChannel[pinTPS-A0], 255); //Get the current raw TPS ADC value and map it into a byte
  #else
    analogRead(pinTPS);
    byte tempTPS = fastMap1023toX(analogRead(pinTPS), 255); //Get the current raw TPS ADC value and map it into a byte
  #endif
  //The use of the filter can be overridden if required. This is used on startup to disable priming pulse if flood clear is wanted
  if(useFilter == true) { currentStatus.tpsADC = ADC_FILTER(tempTPS, configPage4.ADCFILTER_TPS, currentStatus.tpsADC); }
  else { currentStatus.tpsADC = tempTPS; }
  byte tempADC = currentStatus.tpsADC; //The tempADC value is used in order to allow TunerStudio to recover and redo the TPS calibration if this somehow gets corrupted

  if(configPage2.tpsMax > configPage2.tpsMin)
  {
    //Check that the ADC values fall within the min and max ranges (Should always be the case, but noise can cause these to fluctuate outside the defined range).
    if (currentStatus.tpsADC < configPage2.tpsMin) { tempADC = configPage2.tpsMin; }
    else if(currentStatus.tpsADC > configPage2.tpsMax) { tempADC = configPage2.tpsMax; }
    currentStatus.TPS = map(tempADC, configPage2.tpsMin, configPage2.tpsMax, 0, 200); //Take the raw TPS ADC value and convert it into a TPS% based on the calibrated values
  }
  else
  {
    //This case occurs when the TPS +5v and gnd are wired backwards, but the user wishes to retain this configuration.
    //In such a case, tpsMin will be greater then tpsMax and hence checks and mapping needs to be reversed

    tempADC = 255 - currentStatus.tpsADC; //Reverse the ADC values
    uint16_t tempTPSMax = 255 - configPage2.tpsMax;
    uint16_t tempTPSMin = 255 - configPage2.tpsMin;

    //All checks below are reversed from the standard case above
    if (tempADC > tempTPSMax) { tempADC = tempTPSMax; }
    else if(tempADC < tempTPSMin) { tempADC = tempTPSMin; }
    currentStatus.TPS = map(tempADC, tempTPSMin, tempTPSMax, 0, 200);
  }

  //Check whether the closed throttle position sensor is active
  if(configPage2.CTPSEnabled == true)
  {
    if(configPage2.CTPSPolarity == 0) { currentStatus.CTPSActive = !digitalRead(pinCTPS); } //Normal mode (ground switched)
    else { currentStatus.CTPSActive = digitalRead(pinCTPS); } //Inverted mode (5v activates closed throttle position sensor)
  }
  else { currentStatus.CTPSActive = 0; }
}

void readCLT(bool useFilter)
{
  unsigned int tempReading;
  #if defined(ANALOG_ISR)
    tempReading = fastMap1023toX(AnChannel[pinCLT-A0], 511); //Get the current raw CLT value
  #else
    tempReading = analogRead(pinCLT);
    tempReading = analogRead(pinCLT);
    //tempReading = fastMap1023toX(analogRead(pinCLT), 511); //Get the current raw CLT value
  #endif
  //The use of the filter can be overridden if required. This is used on startup so there can be an immediately accurate coolant value for priming
  if(useFilter == true) { currentStatus.cltADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_CLT, currentStatus.cltADC); }
  else { currentStatus.cltADC = tempReading; }
  
  currentStatus.coolant = table2D_getValue(&cltCalibrationTable, currentStatus.cltADC) - CALIBRATION_TEMPERATURE_OFFSET; //Temperature calibration values are stored as positive bytes. We subtract 40 from them to allow for negative temperatures
}

void readIAT(void)
{
  unsigned int tempReading;
  #if defined(ANALOG_ISR)
    tempReading = fastMap1023toX(AnChannel[pinIAT-A0], 511); //Get the current raw IAT value
  #else
    tempReading = analogRead(pinIAT);
    tempReading = analogRead(pinIAT);
    //tempReading = fastMap1023toX(analogRead(pinIAT), 511); //Get the current raw IAT value
  #endif
  currentStatus.iatADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_IAT, currentStatus.iatADC);
  currentStatus.IAT = table2D_getValue(&iatCalibrationTable, currentStatus.iatADC) - CALIBRATION_TEMPERATURE_OFFSET;
}

// When using MAP sensor for baro readings, readMAP needs to be called before to update currentStatus.MAP
void readBaro(bool applyFilter)
{
  if ( configPage6.useExtBaro != 0 )
  {
    int tempReading;
    // readings
    #if defined(ANALOG_ISR_MAP)
      tempReading = AnChannel[pinBaro-A0];
    #else
      tempReading = analogRead(pinBaro);
      tempReading = analogRead(pinBaro);
    #endif

    if ( applyFilter == true ) { currentStatus.baroADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_BARO, currentStatus.baroADC); }//Very weak filter
    else { currentStatus.baroADC = tempReading; } //Baro reading (No filter)

    currentStatus.baro = fastMap10Bit(currentStatus.baroADC, configPage2.baroMin, configPage2.baroMax); //Get the current MAP value
  }
  else
  {
    /*
    * If no dedicated baro sensor is available, attempt to get a reading from the MAP sensor. This can only be done if the engine is not running. 
    * 1. Verify that the engine is not running
    * 2. Verify that the reading from the MAP sensor is within the possible physical limits
    */

    //Attempt to use the last known good baro reading from EEPROM as a starting point
    byte lastBaro = readLastBaro();
    if ((lastBaro >= BARO_MIN) && (lastBaro <= BARO_MAX)) //Make sure it's not invalid (Possible on first run etc)
    { currentStatus.baro = lastBaro; } //last baro correction
    else { currentStatus.baro = 100; } //Fall back position.

    //Verify the engine isn't running by confirming RPM is 0 and it has been at least 1 second since the last tooth was detected
    unsigned long timeToLastTooth = (micros() - toothLastToothTime);
    if((currentStatus.RPM == 0) && (timeToLastTooth > 1000000UL) && currentStatus.hasSync == false && BIT_CHECK(currentStatus.status3, BIT_STATUS3_HALFSYNC) == false )
    {
      /* 
      * The highest sea-level pressure on Earth occurs in Siberia, where the Siberian High often attains a sea-level pressure above 105 kPa;
      * with record highs close to 108.5 kPa.
      * The lowest possible baro reading is based on an altitude of 3500m above sea level.
      */
      if ( hasError(ERR_MAP_LOW) == false && hasError(ERR_MAP_HIGH) == false && // Make sure we are not using a default MAP value
        currentStatus.MAP >= BARO_MIN && currentStatus.MAP <= BARO_MAX ) //Safety check to ensure the baro reading is within the physical limits
      {
        currentStatus.baro = currentStatus.MAP;
        storeLastBaro(currentStatus.baro);
      }
    }
  }
}

void readO2(void)
{
  //An O2 read is only performed if an O2 sensor type is selected. This is to prevent potentially dangerous use of the O2 readings prior to proper setup/calibration
  if(configPage6.egoType > 0)
  {
    unsigned int tempReading;
    #if defined(ANALOG_ISR)
      tempReading = fastMap1023toX(AnChannel[pinO2-A0], 511); //Get the current O2 value.
    #else
      tempReading = analogRead(pinO2);
      tempReading = analogRead(pinO2);
      //tempReading = fastMap1023toX(analogRead(pinO2), 511); //Get the current O2 value.
    #endif
    currentStatus.O2ADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_O2, currentStatus.O2ADC);
    //currentStatus.O2 = o2CalibrationTable[currentStatus.O2ADC];
    currentStatus.O2 = table2D_getValue(&o2CalibrationTable, currentStatus.O2ADC);
  }
  else
  {
    currentStatus.O2ADC = 0;
    currentStatus.O2 = 0;
  }
  
}

void readO2_2(void)
{
  //Second O2 currently disabled as its not being used
  //Get the current O2 value.
  unsigned int tempReading;
  #if defined(ANALOG_ISR)
    tempReading = fastMap1023toX(AnChannel[pinO2_2-A0], 511); //Get the current O2 value.
  #else
    tempReading = analogRead(pinO2_2);
    tempReading = analogRead(pinO2_2);
    //tempReading = fastMap1023toX(analogRead(pinO2_2), 511); //Get the current O2 value.
  #endif
  currentStatus.O2_2ADC = ADC_FILTER(tempReading, configPage4.ADCFILTER_O2, currentStatus.O2_2ADC);
  currentStatus.O2_2 = table2D_getValue(&o2CalibrationTable, currentStatus.O2_2ADC);
}

void readBat(void)
{
  int tempReading;
  #if defined(ANALOG_ISR)
    tempReading = fastMap1023toX(AnChannel[pinBat-A0], 245); //Get the current raw Battery value. Permissible values are from 0v to 24.5v (245)
  #else
    tempReading = analogRead(pinBat);
    tempReading = fastMap1023toX(analogRead(pinBat), 245); //Get the current raw Battery value. Permissible values are from 0v to 24.5v (245)
  #endif

  //Apply the offset calibration value to the reading
  tempReading += configPage4.batVoltCorrect;
  if(tempReading < 0){
    tempReading=0;
  }  //with negative overflow prevention


  //The following is a check for if the voltage has jumped up from under 5.5v to over 7v.
  //If this occurs, it's very likely that the system has gone from being powered by USB to being powered from the 12v power source.
  //Should that happen, we re-trigger the fuel pump priming and idle homing (If using a stepper)
  if( (currentStatus.battery10 < 55) && (tempReading > 70) && (currentStatus.RPM == 0) )
  {
    //Re-prime the fuel pump
    fpPrimeTime = currentStatus.secl;
    fpPrimed = false;
    FUEL_PUMP_ON();

    //Redo the stepper homing
    if( (configPage6.iacAlgorithm == IAC_ALGORITHM_STEP_CL) || (configPage6.iacAlgorithm == IAC_ALGORITHM_STEP_OL) )
    {
      initialiseIdle(true);
    }
  }

  currentStatus.battery10 = ADC_FILTER(tempReading, configPage4.ADCFILTER_BAT, currentStatus.battery10);
}

/**
 * @brief Returns the VSS pulse gap for a given history point
 * 
 * @param historyIndex The gap number that is wanted. EG:
 * historyIndex = 0 = Latest entry
 * historyIndex = 1 = 2nd entry entry
 */
uint32_t vssGetPulseGap(byte historyIndex)
{
  uint32_t tempGap = 0;
  
  noInterrupts();
  int8_t tempIndex = vssIndex - historyIndex;
  if(tempIndex < 0) { tempIndex += VSS_SAMPLES; }

  if(tempIndex > 0) { tempGap = vssTimes[tempIndex] - vssTimes[tempIndex - 1]; }
  else { tempGap = vssTimes[0] - vssTimes[(VSS_SAMPLES-1)]; }
  interrupts();

  return tempGap;
}

uint16_t getSpeed(void)
{
  uint16_t tempSpeed = 0;

  if(configPage2.vssMode == 1)
  {
    //VSS mode 1 is (Will be) CAN
  }
  // Interrupt driven mode
  else if(configPage2.vssMode > 1)
  {
    uint32_t pulseTime = 0;
    uint32_t vssTotalTime = 0;

    //Add up the time between the teeth. Note that the total number of gaps is equal to the number of samples minus 1
    for(byte x = 0; x<(VSS_SAMPLES-1); x++)
    {
      vssTotalTime += vssGetPulseGap(x);
    }

    pulseTime = vssTotalTime / (VSS_SAMPLES - 1);
    if ( (micros() - vssTimes[0]) > 1000000UL ) { tempSpeed = 0; } // Check that the car hasn't come to a stop
    else 
      {
        tempSpeed = 3600000000UL / (pulseTime * configPage2.vssPulsesPerKm); //Convert the pulse gap into km/h
        tempSpeed = ADC_FILTER(tempSpeed, configPage2.vssSmoothing, currentStatus.vss); //Apply speed smoothing factor
      }
    if(tempSpeed > 1000) { tempSpeed = currentStatus.vss; } //Safety check. This usually occurs when there is a hardware issue

  }
  return tempSpeed;
}

byte getGear(void)
{
  byte tempGear = 0; //Unknown gear
  if(currentStatus.vss > 0)
  {
    //If the speed is non-zero, default to the last calculated gear
    tempGear = currentStatus.gear;

    uint16_t pulsesPer1000rpm = (currentStatus.vss * 10000UL) / currentStatus.RPM; //Gives the current pulses per 1000RPM, multiplied by 10 (10x is the multiplication factor for the ratios in TS)
    //Begin gear detection
    if( (pulsesPer1000rpm > (configPage2.vssRatio1 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio1 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 1; }
    else if( (pulsesPer1000rpm > (configPage2.vssRatio2 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio2 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 2; }
    else if( (pulsesPer1000rpm > (configPage2.vssRatio3 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio3 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 3; }
    else if( (pulsesPer1000rpm > (configPage2.vssRatio4 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio4 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 4; }
    else if( (pulsesPer1000rpm > (configPage2.vssRatio5 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio5 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 5; }
    else if( (pulsesPer1000rpm > (configPage2.vssRatio6 - VSS_GEAR_HYSTERESIS)) && (pulsesPer1000rpm < (configPage2.vssRatio6 + VSS_GEAR_HYSTERESIS)) ) { tempGear = 6; }
  }
  
  return tempGear;
}

byte getFuelPressure(void)
{
  int16_t tempFuelPressure = 0;
  uint16_t tempReading;

  if(configPage10.fuelPressureEnable > 0)
  {
    //Perform ADC read
    tempReading = analogRead(pinFuelPressure);
    tempReading = analogRead(pinFuelPressure);

    tempFuelPressure = fastMap10Bit(tempReading, configPage10.fuelPressureMin, configPage10.fuelPressureMax);
    tempFuelPressure = ADC_FILTER(tempFuelPressure, ADCFILTER_PSI_DEFAULT, currentStatus.fuelPressure); //Apply smoothing factor
    //Sanity checks
    if(tempFuelPressure > configPage10.fuelPressureMax) { tempFuelPressure = configPage10.fuelPressureMax; }
    if(tempFuelPressure < 0 ) { tempFuelPressure = 0; } //prevent negative values, which will cause problems later when the values aren't signed.
  }

  return (byte)tempFuelPressure;
}

byte getOilPressure(void)
{
  int16_t tempOilPressure = 0;
  uint16_t tempReading;

  if(configPage10.oilPressureEnable > 0)
  {
    //Perform ADC read
    tempReading = analogRead(pinOilPressure);
    tempReading = analogRead(pinOilPressure);


    tempOilPressure = fastMap10Bit(tempReading, configPage10.oilPressureMin, configPage10.oilPressureMax);
    tempOilPressure = ADC_FILTER(tempOilPressure, ADCFILTER_PSI_DEFAULT, currentStatus.oilPressure); //Apply smoothing factor
    //Sanity check
    if(tempOilPressure > configPage10.oilPressureMax) { tempOilPressure = configPage10.oilPressureMax; }
    if(tempOilPressure < 0 ) { tempOilPressure = 0; } //prevent negative values, which will cause problems later when the values aren't signed.
  }


  return (byte)tempOilPressure;
}

/*
 * The interrupt function for reading the flex sensor frequency and pulse width
 * flexCounter value is incremented with every pulse and reset back to 0 once per second
 */
void flexPulse(void)
{
  if(READ_FLEX() == true)
  {
    unsigned long tempPW = (micros() - flexStartTime); //Calculate the pulse width
    flexPulseWidth = ADC_FILTER(tempPW, configPage4.FILTER_FLEX, flexPulseWidth);
    ++flexCounter;
  }
  else
  {
    flexStartTime = micros(); //Start pulse width measurement.
  }
}

void swfMAPpulse(void)
{
  uint32_t swfMAPcurrentTime = micros();
  swfMAPlastInterval = swfMAPcurrentTime - swfMAPlastTime;
  swfMAPlastTime = swfMAPcurrentTime;
}

/*
 * The interrupt function for pulses from a knock conditioner / controller
 * 
 */
void knockPulse(void)
{
  //Check if this the start of a knock. 
  if(knockCounter == 0)
  {
    //knockAngle = crankAngle + fastTimeToAngle( (micros() - lastCrankAngleCalc) ); 
    knockStartTime = micros();
    knockCounter = 1;
  }
  else { ++knockCounter; } //Knock has already started, so just increment the counter for this

}

/**
 * @brief The ISR function for VSS pulses
 * 
 */
void vssPulse(void)
{
  //TODO: Add basic filtering here
  vssIndex++;
  if(vssIndex == VSS_SAMPLES) { vssIndex = 0; }

  vssTimes[vssIndex] = micros();
}

uint16_t readAuxanalog(uint8_t analogPin)
{
  //read the Aux analog value for pin set by analogPin 
  unsigned int tempReading;
  #if defined(ANALOG_ISR)
    tempReading = fastMap1023toX(AnChannel[analogPin-A0], 1023); //Get the current raw Auxanalog value
  #else
    tempReading = analogRead(analogPin);
    tempReading = analogRead(analogPin);
    //tempReading = fastMap1023toX(analogRead(analogPin), 511); Get the current raw Auxanalog value
  #endif
  return tempReading;
} 

uint16_t readAuxdigital(uint8_t digitalPin)
{
  //read the Aux digital value for pin set by digitalPin 
  unsigned int tempReading;
  tempReading = digitalRead(digitalPin); 
  return tempReading;
} 
