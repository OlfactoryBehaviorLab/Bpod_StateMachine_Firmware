/*
  ----------------------------------------------------------------------------

  This file is part of the Sanworks Bpod_Gen2 repository
  Copyright (C) 2022 Sanworks LLC, Rochester, New York, USA

  ----------------------------------------------------------------------------

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3.

  This program is distributed  WITHOUT ANY WARRANTY and without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
// ##############################################
// # Bpod Finite State Machine Firmware Ver. 23 #
// ##############################################
//
// SYSTEM SETUP:
//
// IF COMPILING FOR Arduino DUE, Requires a small modification to Arduino, for useful PWM control of light intensity with small (1-10ms) pulses:
// change the PWM_FREQUENCY and TC_FREQUENCY constants to 50000 in the file /Arduino/hardware/arduino/sam/variants/arduino_due_x/variant.h
// or in Windows, \Users\Username\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.7\variants\arduino_due_x\variant.h
//
// **NOTE** previous versions of this firmware required dependencies and modifications to the Teensy core files. As of firmware v23, these are no longer necessary.
// **NOTE** Requires Arduino 1.8.19 or newer, and Teensyduino 1.57 or newer

//////////////////////////////////////////
// Set hardware version (0.5, 1.0, etc)  /
//////////////////////////////////////////
// Machine Type: 1 = FSM 0.5 (Arduino Due); 2 = FSM 0.7-1.0 (Arduino Due); 3 = FSM 2.0-2.4 (Teensy 3.6) or FSM 2.5 (Teensy 4.1) 4 = FSM 2+ (Teensy 4.1)
// Machine Build: All new machines are MACHINE_BUILD = 0. When a design revision changes the microcontroller pinout without changing the state machine I/O channels, Machine build is incremented.
// Configure as follows:
// FSM 0.5:     MACHINE_TYPE 1, MACHINE_BUILD 0
// FSM 0.7-1.0: MACHINE_TYPE 2, MACHINE_BUILD 0
// FSM 2.0-2.4: MACHINE_TYPE 3, MACHINE_BUILD 0
// FSM 2.5:     MACHINE_TYPE 3, MACHINE_BUILD 1
// FSM 2+ r1.0: MACHINE_TYPE 4, MACHINE_BUILD 0

#define MACHINE_TYPE 3
#define MACHINE_BUILD 1

//////////////////////////////////////////
//    State Machine Feature Profile      /
//////////////////////////////////////////
// Profile of internal state machine features (max numbers of global timers, counters and conditions).
// Declaring more of these requires more MCU sRAM, often in ways that are non-linear. It is safe to add 
// profiles here to reallocate memory as needed, at the line beginning: #if SM_FEATURE_PROFILE == 0 
// 0 = Bpod Native on HW 0.5-1.0 (5,5,5)
// 1 = Bpod Native on HW 2.0-2.3 and 2+ (16,8,16)
// 2 = Bpod for BControl on HW 0.7-1.0 (8,2,8) 
// 3 = Bpod for Bcontrol on HW 2.0-2.3 (20,2,20)

#define SM_FEATURE_PROFILE 1

#if SM_FEATURE_PROFILE == 0
  #define MAX_GLOBAL_TIMERS 5
  #define MAX_GLOBAL_COUNTERS 5
  #define MAX_CONDITIONS 5
#elif SM_FEATURE_PROFILE == 1
  #define MAX_GLOBAL_TIMERS 16
  #define MAX_GLOBAL_COUNTERS 8
  #define MAX_CONDITIONS 16
#elif SM_FEATURE_PROFILE == 2
  #define MAX_GLOBAL_TIMERS 8
  #define MAX_GLOBAL_COUNTERS 2
  #define MAX_CONDITIONS 8
#elif SM_FEATURE_PROFILE == 3
  #define MAX_GLOBAL_TIMERS 20
  #define MAX_GLOBAL_COUNTERS 2
  #define MAX_CONDITIONS 20
#endif

//////////////////////////////////////////
//          Set Firmware Version         /
//////////////////////////////////////////
// Current firmware version (single firmware file, compiles for MachineTypes set above).

#define FIRMWARE_VERSION_MAJOR 23 // Incremented with each stable release (master branch)
#define FIRMWARE_VERSION_MINOR 10 // Incremented with each push on develop branch, reset with major firmware version increment

//////////////////////////////////////////
//      Live Timestamp Transmission      /
//////////////////////////////////////////
// 0 to return timestamps after trial, 1 to return timestamps during trial along with event byte-codes 

#define LIVE_TIMESTAMPS 1

//////////////////////////////////////////
//    Ethernet Communication Option      /
//////////////////////////////////////////
// Defines the state machine's communication channel to the PC. 0 = USB (default), 1 = Ethernet (w/ Bpod Ethernet Module)
// IMPORTANT: PC via Ethernet requires State Machine v2.0 or newer.

#define ETHERNET_COM 0

//////////////////////////////////////////
//          Board configuration          /
//////////////////////////////////////////

#if MACHINE_TYPE < 3
  #include "DueTimer.h"
#endif
#include <SPI.h>
#include "ArCOM.h" // ArCOM is a serial interface wrapper developed by Sanworks, 
                   // to streamline transmission of datatypes and arrays over USB and UART
#if ETHERNET_COM == 1
  #include "ArCOMvE.h" // Special variant of ArCOM specialized for transmission via Bpod Ethernet module
#endif

#if MACHINE_TYPE == 4
  #include "AD5592R.h" // Library for the AD5592R Analog + Digital interface IC, used for FlexI/O channels
#endif 

#if MACHINE_TYPE == 1
  ArCOM PC(SerialUSB); // Creates an ArCOM object called PC, wrapping SerialUSB (or in some cases, a different serial channel with an Ethernet link to the PC)
  ArCOM Module1(Serial1); // Creates an ArCOM object called Module1, wrapping Serial1
  ArCOM Module2(Serial2); 
#elif MACHINE_TYPE == 2
  ArCOM PC(SerialUSB); 
  ArCOM Module1(Serial1); 
  ArCOM Module2(Serial2);
  ArCOM Module3(Serial3);
#elif MACHINE_TYPE == 3
  #if !defined(USB_DUAL_SERIAL)
    #error Error! You must choose 'Dual Serial' from the 'Tools' > 'USB Type' menu in the Arduino application before uploading this firmware.
  #endif
  #if ETHERNET_COM == 0 
    ArCOM PC(SerialUSB);
    ArCOM PC1(SerialUSB1); 
  #else
    #if MACHINE_BUILD == 0
      ArCOMvE PC(Serial5);
      ArCOM PC1(SerialUSB1);  
    #elif MACHINE_BUILD == 1
      ArCOMvE PC(Serial8); 
      ArCOM PC1(SerialUSB1); 
    #endif
  #endif
  ArCOM Module1(Serial1);
  #if MACHINE_BUILD == 0
    ArCOM Module2(Serial3); 
    ArCOM Module3(Serial2); 
    ArCOM Module4(Serial4); 
  #elif MACHINE_BUILD == 1
    ArCOM Module2(Serial2); 
    ArCOM Module3(Serial6); 
    ArCOM Module4(Serial7);
  #endif
  #if ETHERNET_COM == 0 
    #if MACHINE_BUILD == 0
      ArCOM Module5(Serial5); 
    #elif MACHINE_BUILD == 1
      ArCOM Module5(Serial8);
    #endif
  #endif
#elif MACHINE_TYPE == 4
  #if !defined(USB_TRIPLE_SERIAL)
    #error Error! You must choose 'Triple Serial' from the 'Tools' > 'USB Type' menu in the Arduino application before uploading this firmware.
  #endif
  ArCOM PC(SerialUSB);
  ArCOM PC1(SerialUSB1); 
  ArCOM PC2(SerialUSB2);  
  ArCOM Module1(Serial1); 
  ArCOM Module2(Serial2);
  ArCOM Module3(Serial6);
#endif

////////////////////////////////////////
// State machine hardware description  /
////////////////////////////////////////
// Two pairs of arrays describe the hardware as it appears to the state machine: inputHW / inputCh, and outputHW / outputCh.
// In these arrays, the first row codes for hardware type. U = UART, X = USB1, F = Flex I/O (AD5592r) S = SPI,  D = digital, 
//                                                         B = BNC (digital/inverted) W = Wire (digital/inverted), V = Valve (digital)
//                                                         P = port (digital channel if input, PWM channel if output). 
// Channels must be listed IN THIS ORDER (this constraint allows efficient code at runtime). 
// The digital,BNC or wire channel currently replaced by 'Y' is the sync channel (none by default).
// The second row lists the physical input and output channels on Arduino for B,W,P, and the SPI CS pin is listed for S. Use 0 for UART, USB and Flex I/O.
// For Flex I/O channels, the second row lists their index among Flex I/O channels (e.g. 0, 1, 2, 3)

#if MACHINE_TYPE == 1 // Bpod state machine v0.5
    byte InputHW[] = {'U','U','X','B','B','W','W','W','W','P','P','P','P','P','P','P','P'};
    byte InputCh[] = {0,0,0,11,10,35,33,31,29,28,30,32,34,36,38,40,42};                                         
    byte OutputHW[] = {'U','U','X','B','B','W','W','W','W','P','P','P','P','P','P','P','P','V','V','V','V','V','V','V','V'};
    byte OutputCh[] = {0,0,0,25,24,43,41,39,37,9,8,7,6,5,4,3,2,22,22,22,22,22,22,22,22};   
#elif MACHINE_TYPE == 2 // Bpod State Machine r0.7+
    byte InputHW[] = {'U','U','U','X','B','B','W','W','P','P','P','P','P','P','P','P'};
    byte InputCh[] = {0,0,0,0,10,11,31,29,28,30,32,34,36,38,40,42};                                         
    byte OutputHW[] = {'U','U','U','X','B','B','W','W','W','P','P','P','P','P','P','P','P','V','V','V','V','V','V','V','V'};
    byte OutputCh[] = {0,0,0,0,25,24,43,41,39,9,8,7,6,5,4,3,2,22,22,22,22,22,22,22,22};  
#elif MACHINE_TYPE == 3 // Bpod State Machine r2.0-2.5
  #if ETHERNET_COM == 0
    #if MACHINE_BUILD == 0
      byte InputHW[] = {'U','U','U','U','U','X','Z','B','B','P','P','P','P'};
      byte InputCh[] = {0,0,0,0,0,0,0,6,5,39,38,18,15};                                         
      byte OutputHW[] = {'U','U','U','U','U','X','Z','B','B','P','P','P','P','V','V','V','V'};
      byte OutputCh[] = {0,0,0,0,0,0,0,4,3,37,14,16,17,23,22,20,21};  
    #elif MACHINE_BUILD == 1
      byte InputHW[] = {'U','U','U','U','U','X','Z','B','B','P','P','P','P'};
      byte InputCh[] = {0,0,0,0,0,0,0,6,5,39,38,17,16};                                         
      byte OutputHW[] = {'U','U','U','U','U','X','Z','B','B','P','P','P','P','V','V','V','V'};
      byte OutputCh[] = {0,0,0,0,0,0,0,4,3,37,14,15,18,23,22,20,21};  
    #endif
  #else
    #if MACHINE_BUILD == 0
      byte InputHW[] = {'U','U','U','U','X','Z','B','B','P','P','P','P'};
      byte InputCh[] = {0,0,0,0,0,0,6,5,39,38,18,15};                                         
      byte OutputHW[] = {'U','U','U','U','X','Z','B','B','P','P','P','P','V','V','V','V'};
      byte OutputCh[] = {0,0,0,0,0,0,4,3,37,14,16,17,23,22,20,21}; 
    #elif MACHINE_BUILD == 1
      byte InputHW[] = {'U','U','U','U','X','Z','B','B','P','P','P','P'};
      byte InputCh[] = {0,0,0,0,0,0,6,5,39,38,17,16};                                         
      byte OutputHW[] = {'U','U','U','U','X','Z','B','B','P','P','P','P','V','V','V','V'};
      byte OutputCh[] = {0,0,0,0,0,0,4,3,37,14,15,18,23,22,20,21}; 
    #endif
  #endif
#elif MACHINE_TYPE == 4 // Bpod State Machine 2+ r1.0
    byte InputHW[] = {'U','U','U','X','Z','F','F','F','F','B','B','P','P','P','P','P'};
    byte InputCh[] = {0,0,0,0,0,0,0,0,0,6,5,39,38,17,16,41};                                         
    byte OutputHW[] = {'U','U','U','X','Z','F','F','F','F','B','B','P','P','P','P','P','V','V','V','V','V'};
    byte OutputCh[] = {0,0,0,0,0,0,1,2,3,4,3,36,37,15,14,18,20,19,21,22,23};  
#endif

// State machine meta information
const byte nInputs = sizeof(InputHW);
const byte nOutputs = sizeof(OutputHW);
#if MACHINE_TYPE == 1 // State machine (Bpod 0.5)
  #define TEENSY_VERSION 0
  const byte nSerialChannels = 3; // Must match total of 'U', 'X' and 'Z' in InputHW (above)
  const byte maxSerialEvents = 30; // Must be a multiple of nSerialChannels
  const int MaxStates = 128;
  const int SerialBaudRate = 115200; // Transfer speed of hardware serial (Module) ports
  byte hardwareRevisionArray[5] = {44,45,46,47,48};
#elif MACHINE_TYPE == 2 // Bpod State Machine r0.7+
  #define TEENSY_VERSION 0
  const byte nSerialChannels = 4; 
  const byte maxSerialEvents = 60;
  const int MaxStates = 256;
  const int SerialBaudRate = 1312500;
  byte hardwareRevisionArray[5] = {23,26,27,35,37};
#elif MACHINE_TYPE == 3  // Teensy 3.6 based state machines (r2.0-2.3)
  #if ETHERNET_COM == 0
    const byte nSerialChannels = 7; 
    const byte maxSerialEvents = 105;
  #else
    const byte nSerialChannels = 6; 
    const byte maxSerialEvents = 90;
  #endif
  const int MaxStates = 256;
  const int SerialBaudRate = 1312500; // Default Bpod Baudrate
  const int MFCBaudRate = 57600; // RS232 Baudrate
  int current_baudrates[5] = {1312500, 1312500, 1312500, 1312500, 1312500}; // List of Baudrates
  #if MACHINE_BUILD == 0
    #define TEENSY_VERSION 3
    byte hardwareRevisionArray[5] = {25,26,27,28,29};
  #elif MACHINE_BUILD == 1
    #define TEENSY_VERSION 4
    byte hardwareRevisionArray[5] = {32,31,30,27,9};
  #endif
#elif MACHINE_TYPE == 4  // Teensy 4.1 based state machines (r2+ v1.0)
  #define TEENSY_VERSION 4
  const byte nSerialChannels = 5; 
  const byte maxSerialEvents = 75;
  const int MaxStates = 256;
  const int SerialBaudRate = 1312500;
  byte hardwareRevisionArray[5] = {27,28,29,30,31};
#endif

uint16_t timerPeriod = 100; // Hardware timer period, in microseconds (state machine refresh period)

#if MAX_GLOBAL_TIMERS > 16
  #define GLOBALTIMER_TRIG_BYTEWIDTH 4
#elif MAX_GLOBAL_TIMERS > 8
  #define GLOBALTIMER_TRIG_BYTEWIDTH 2
#else
  #define GLOBALTIMER_TRIG_BYTEWIDTH 1
#endif

// Vars to hold number of timers, counters and conditions actually used in the current state matrix
byte nGlobalTimersUsed = MAX_GLOBAL_TIMERS;
byte nGlobalCountersUsed = MAX_GLOBAL_COUNTERS;
byte nConditionsUsed = MAX_CONDITIONS;

// flexIO vars
#if MACHINE_TYPE == 4
  const byte nFlexIO = 4;
  AD5592R FlexIO(9, 32); // Create FlexIO, an AD5592R object with CS on Teensy pin 9 and BusyPin on Teensy pin 32
  volatile byte flexIOChannelType[nFlexIO] = {4,4,4,4}; // Set defaults similar to wire terminals on r0.5-1.0. Channel types are: 0 = DI, 1 = DO, 2 = ADC, 3 = DAC, 4 = Tri-State
  uint16_t flexIOValues[nFlexIO] = {0}; // Stores values read from or to be written to FlexIO channels
  boolean flexIO_updateDIflag = false; // Flags if updates are required for each channel type
  boolean flexIO_updateDOflag = false;
  boolean flexIO_updateAIflag = false;
  boolean flexIO_updateAOflag = false;
  uint32_t flexIO_ADC_Sample_Interval = 10; // Units = state machine cycles
  uint32_t flexIO_ADC_Sample_Clock = 0; // Counts cycles
#else
  const byte nFlexIO = 0;
#endif

// Serial buffers
#if MACHINE_TYPE > 2
  byte HWSerialBuf1[192] = {0};
  byte HWSerialBuf2[192] = {0};
  byte HWSerialBuf3[192] = {0};
#endif
#if MACHINE_TYPE == 3
  byte HWSerialBuf4[192] = {0};
  byte HWSerialBuf5[192] = {0};
#endif
                         
// Other hardware pin mapping
#if MACHINE_TYPE == 1
  byte GreenLEDPin = 14;
  byte RedLEDPin = 13;
  byte BlueLEDPin = 12;
  byte valveCSChannel = 22;
#elif MACHINE_TYPE == 2
  byte GreenLEDPin = 33;
  byte RedLEDPin = 13;
  byte BlueLEDPin = 12;
  byte valveCSChannel = 22;
#elif MACHINE_TYPE == 3
  #if MACHINE_BUILD == 0
    byte GreenLEDPin = 2;
    byte RedLEDPin = 36;
    byte BlueLEDPin = 35;
    byte ValveEnablePin = 19;
    byte valveCSChannel = 0;
  #else
    byte GreenLEDPin = 2;
    byte RedLEDPin = 36;
    byte BlueLEDPin = 33;
    byte ValveEnablePin = 19;
    byte valveCSChannel = 0;
  #endif
#elif MACHINE_TYPE == 4
  byte GreenLEDPin = 35;
  byte RedLEDPin = 2;
  byte BlueLEDPin = 33;
  byte valveCSChannel = 0;
#endif

// fRAM vars
byte fRAMcs = 1;
byte fRAMhold = A3;
byte SyncRegisterLatch = 23;

//psRAM vars
#if MACHINE_TYPE == 4
  // PSRAM setup (for bench testing only, PSRAM is not used in current firmware)
  extern "C" uint8_t external_psram_size;
  uint32_t *memory_begin, *memory_end;
  boolean memOK = false;
#endif

// Settings for version-specific hardware (initialized in setup)
boolean usesFRAM = false;
boolean usesSPISync = false;
boolean usesSPIValves = false;
boolean usesUARTInputs = false;
boolean usesFlexIO = false;
boolean isolatorHigh = 0;
boolean isolatorLow = 0;

// Bookmarks (positions of channel types in hardware description vectors, to be calculated in setup)
byte USBInputPos = 0;
byte BNCInputPos = 0;
byte WireInputPos = 0;
byte PortInputPos = 0;
byte FlexInputPos = 0;
byte USBOutputPos = 0;
byte SPIOutputPos = 0;
byte BNCOutputPos = 0;
byte WireOutputPos = 0;
byte PortOutputPos = 0;
byte FlexOutputPos = 0;
byte ValvePos = 0;
byte DigitalInputPos = 0; // Beginning of digital input channels

// Parameters

const unsigned long ModuleIDTimeout = 100; // timeout for modules to respond to byte 255 (units = 100us cycles). Byte 255 polls for module hardware information
boolean statusLEDEnabled = true; // Set to false to disable status LED
uint16_t trialCounter = 0; // Current trial number (reset with serial op code)

// Initialize system state vars: 
byte outputState[nOutputs] = {0}; // State of outputs
byte inputState[nInputs+MAX_GLOBAL_TIMERS] = {0}; // Current state of inputs
byte lastInputState[nInputs+MAX_GLOBAL_TIMERS] = {0}; // State of inputs on previous cycle
byte inputOverrideState[nInputs] = {0}; // Set to 1 if user created a virtual event, to prevent hardware reads until user returns low
byte outputOverrideState[nOutputs] = {0}; // Set to 1 when overriding a digital output line. This prevents state changes from affecting the line until it is manually reset.
byte inputEnabled[nInputs] = {0}; // 0 if input disabled, 1 if enabled
byte logicHigh[nInputs] = {0}; // 1 if standard, 0 if inverted input (calculated in setup for each channel, depending on its type)
byte logicLow[nInputs] = {0}; // Inverse of logicHigh (to save computation time)
const byte nDigitalInputs = nInputs - nSerialChannels; // Number of digital input channels
boolean MatrixFinished = false; // Has the system exited the matrix (final state)?
boolean MeaningfulStateTimer = false; // Does this state's timer get us to another state when it expires?
int CurrentState = 1; // What state is the state machine currently in? (State 0 is the final state)
int NewState = 1; // State the system determined it needs to enter next. If different from current state, transition logic proceeds.
int CurrentStateTEMP = 1; // Temporarily holds current state during transition
// Event vars
#if MACHINE_TYPE < 3
  const byte maxCurrentEvents = 10; // Max number of events that can be recorded during a single 100 microsecond cycle
#else
  const byte maxCurrentEvents = 100; // Max number of events that can be recorded during a single 100 microsecond cycle
#endif
byte CurrentEvent[maxCurrentEvents] = {0}; // What event code just happened and needs to be handled. Up to 10 can be acquired per 100us loop.
byte CurrentEventBuffer[maxCurrentEvents+6] = {0}; // Current events as packaged for rapid vector transmission 
byte nCurrentEvents = 0; // Index of current event
byte SoftEvent1 = 0; // What soft event code was just received on USB
byte SoftEvent2 = 0; // What soft event code was just received on USB2
// Trial / Event Sync Vars
byte SyncChannel = 255; // 255 if no sync codes, <255 to specify a channel to use for sync
boolean syncOn = false; // If true, sync codes are sent on sync channel
byte SyncChannelHW = 0; // Stores physical pin for sync channel
byte NewSyncChannel = 0; // New digital output channel to use for Sync. (old channel gets reset to its original hardware type)
byte SyncMode = 0; // 0 if low > high on trial start and high < low on trial end, 1 if line toggles with each state change, 2 if line toggles at 10Hz from trial start to trial end
byte SyncState = 0; // State of the sync line (0 = low, 1 = high)
uint32_t SyncTimer = 0; // Number of cycles since last sync state toggle (for mode 2)
// Others
boolean smaTransmissionConfirmed = false; // Set to true when the last state machine was successfully received, set to false when starting a transmission
boolean newSMATransmissionStarted = false; // Set to true when beginning a state machine transmission
boolean UARTrelayMode[nSerialChannels] = {false};
byte nModuleEvents[nSerialChannels] = {0}; // Stores number of behavior events assigned to each serial module (15 by default)
uint16_t stateMatrixNBytes = 0; // Number of bytes in the state matrix about to be transmitted
boolean using255BackSignal = 0; // If enabled, only 254 states can be used and going to "state 255" returns system to the previous state

// RS232 Configuration
int arg1 = 0;
int arg2 = 0;

//////////////////////////////////
// Initialize general use vars:  /
//////////////////////////////////
const byte discoveryByte = 222; // Unique byte to send to any serial port on first connection (for USB serial port auto-discovery)
const byte discoveryByteInterval = 100; // ms between discovery byte transmissions
boolean RunStateMatrixASAP = false; // On state matrix transmission, set to 1 if the new state matrix should auto-run after the trial ends
byte CommandByte = 0;  // Op code to specify handling of an incoming USB serial message
byte VirtualEventTarget = 0; // Op code to specify which virtual event type (Port, BNC, etc)
byte VirtualEventData = 0; // State of target
byte Byte1 = 0; byte Byte2 = 0; byte Byte3 = 0; byte Byte4 = 0; // Temporary storage of values read from serial port
byte nPossibleEvents = 0; // possible events in the state machine (computed in setup)
int nStates = 0; // Total number of states in the current state machine
int nEvents = 0; // Total number of events recorded while running current state machine
byte previousState = 0; // Previous state visited. Used if 255 is interpreted as a "back" signal (see using255BackSignal above)
int LEDBrightnessAdjustInterval = 5;
byte LEDBrightnessAdjustDirection = 1;
byte LEDBrightness = 0;
byte serialByteBuffer[4] = {0}; // Stores 1-3 byte messages transmitted as output actions of states

// Vars for state machine definitions. Each matrix relates each state to some inputs or outputs.
const byte InputMatrixSize = maxSerialEvents + (nDigitalInputs*2);
byte InputStateMatrix[MaxStates+1][InputMatrixSize] = {0}; // Matrix containing all of Bpod's digital inputs and corresponding state transitions
byte StateTimerMatrix[MaxStates+1] = {0}; // Matrix containing states to move to if the state timer elapses
const byte OutputMatrixSize = nOutputs;
#if MACHINE_TYPE == 4
  uint16_t OutputStateMatrix[MaxStates+1][OutputMatrixSize] = {0}; // Hardware states for outputs. Serial channels > Digital outputs > Virtual (global timer trigger, global timer cancel, global counter reset)
#else
  byte OutputStateMatrix[MaxStates+1][OutputMatrixSize] = {0};
#endif
byte smGlobalCounterReset[MaxStates+1] = {0}; // For each state, global counter to reset.
#if MACHINE_TYPE == 4
  byte analogThreshEnable[MaxStates+1] = {0}; // Bits indicate analog thresholds to enable in each state
  byte analogThreshDisable[MaxStates+1] = {0}; // Bits indicate analog thresholds to disable in each state
#endif
byte GlobalTimerStartMatrix[MaxStates+1][MAX_GLOBAL_TIMERS] = {0}; // Matrix contatining state transitions for global timer onset events
byte GlobalTimerEndMatrix[MaxStates+1][MAX_GLOBAL_TIMERS] = {0}; // Matrix contatining state transitions for global timer elapse events
byte GlobalCounterMatrix[MaxStates+1][MAX_GLOBAL_COUNTERS] = {0}; // Matrix contatining state transitions for global counter threshold events
byte ConditionMatrix[MaxStates+1][MAX_CONDITIONS] = {0}; // Matrix contatining state transitions for conditions
boolean GlobalTimersTriggered[MAX_GLOBAL_TIMERS] = {0}; // 0 if timer x was not yet triggered, 1 if it was triggered and had not elapsed.
boolean GlobalTimersActive[MAX_GLOBAL_TIMERS] = {0}; // 0 if timer x is inactive (e.g. not triggered, or during onset delay after trigger), 1 if it's active.
#if MACHINE_TYPE < 3
  const byte SerialMessageMaxBytes = 3;
#else
  const byte SerialMessageMaxBytes = 5;
#endif
byte SerialMessageMatrix[MaxStates+1][nSerialChannels][SerialMessageMaxBytes]; // Stores a 5-byte serial message for each message byte on each port
byte SerialMessage_nBytes[MaxStates+1][nSerialChannels] = {1}; // Stores the length of each serial message
boolean ModuleConnected[nSerialChannels] = {false}; // true for each channel if a module is connected, false otherwise
byte SyncChannelOriginalType = 0; // Stores sync channel's original hardware type

// Global timer triggers (data type dependent on number of global timers; using fewer = faster SM switching, extra memory for other configs)
#if GLOBALTIMER_TRIG_BYTEWIDTH == 1
  uint8_t GlobalTimerOnsetTriggers[MAX_GLOBAL_TIMERS] = {0}; // Bits indicate other global timers to trigger when timer turns on (after delay)
  uint8_t smGlobalTimerTrig[MaxStates+1] = {0}; // For each state, global timers to trigger. Bits indicate timers.
  uint8_t smGlobalTimerCancel[MaxStates+1] = {0}; // For each state, global timers to cancel. Bits indicate timers.
#elif GLOBALTIMER_TRIG_BYTEWIDTH == 2
  uint16_t GlobalTimerOnsetTriggers[MAX_GLOBAL_TIMERS] = {0};
  uint16_t smGlobalTimerTrig[MaxStates+1] = {0};
  uint16_t smGlobalTimerCancel[MaxStates+1] = {0};
#elif GLOBALTIMER_TRIG_BYTEWIDTH == 4
  uint32_t GlobalTimerOnsetTriggers[MAX_GLOBAL_TIMERS] = {0};
  uint32_t smGlobalTimerTrig[MaxStates+1] = {0};
  uint32_t smGlobalTimerCancel[MaxStates+1] = {0};
#endif

// Positions of input matrix parts
byte GlobalTimerStartPos = InputMatrixSize; // First global timer event code
byte GlobalTimerEndPos = GlobalTimerStartPos + MAX_GLOBAL_TIMERS;
byte GlobalCounterPos = GlobalTimerEndPos + MAX_GLOBAL_TIMERS; // First global counter event code
byte ConditionPos = GlobalCounterPos + MAX_GLOBAL_COUNTERS; // First condition event code
byte TupPos = ConditionPos+MAX_CONDITIONS; // First Jump event code

byte GlobalTimerChannel[MAX_GLOBAL_TIMERS] = {254}; // Channel code for global timer onset/offset.
#if MACHINE_TYPE == 4
  uint16_t GlobalTimerOnMessage[MAX_GLOBAL_TIMERS] = {254}; // Message to send when global timer is active (if channel is serial).
  uint16_t GlobalTimerOffMessage[MAX_GLOBAL_TIMERS] = {254}; // Message to send when global timer elapses (if channel is serial).
#else
  byte GlobalTimerOnMessage[MAX_GLOBAL_TIMERS] = {254}; 
  byte GlobalTimerOffMessage[MAX_GLOBAL_TIMERS] = {254}; 
#endif
unsigned long GlobalTimerStart[MAX_GLOBAL_TIMERS] = {0}; // Future Times when active global timers will start measuring time
unsigned long GlobalTimerEnd[MAX_GLOBAL_TIMERS] = {0}; // Future Times when active global timers will elapse
unsigned long GlobalTimers[MAX_GLOBAL_TIMERS] = {0}; // Timers independent of states
unsigned long GlobalTimerOnsetDelays[MAX_GLOBAL_TIMERS] = {0}; // Onset delay following global timer trigger
unsigned long GlobalTimerLoopIntervals[MAX_GLOBAL_TIMERS] = {0}; // Configurable delay between global timer loop iterations
byte GlobalTimerLoop[MAX_GLOBAL_TIMERS] = {0}; // Number of loop iterations. 0 = no loop. 1 = loop until shut-off. 2-255 = number of loops to execute
byte GlobalTimerLoopCount[MAX_GLOBAL_TIMERS] = {0}; // When GlobalTimerLoop > 1, counts the number of loops elapsed
boolean GTUsingLoopCounter[MAX_GLOBAL_TIMERS] = {false}; // If this timer's GlobalTimerLoop > 1 (i.e. terminated by loop counter)
byte SendGlobalTimerEvents[MAX_GLOBAL_TIMERS] = {0}; // true if events are returned to the state machine (especially useful to disable in loop mode)
unsigned long GlobalCounterCounts[MAX_GLOBAL_COUNTERS] = {0}; // Event counters
byte GlobalCounterAttachedEvents[MAX_GLOBAL_COUNTERS] = {254}; // Event each event counter is attached to
unsigned long GlobalCounterThresholds[MAX_GLOBAL_COUNTERS] = {0}; // Event counter thresholds (trigger events if crossed)
boolean GlobalCounterHandled[MAX_GLOBAL_COUNTERS] = {false};
byte ConditionChannels[MAX_CONDITIONS] = {254}; // Event each channel a condition is attached to
byte ConditionValues[MAX_CONDITIONS] = {0}; // The value of each condition
const int MaxTimestamps = 10000;
#if MACHINE_TYPE == 1
  unsigned long Timestamps[MaxTimestamps] = {0};
#endif
unsigned long StateTimers[MaxStates+1] = {0}; // Timers for each state
unsigned long StartTime = 0; // System Start Time
uint64_t MatrixStartTimeMicros = 0; // Start time of state matrix (in us)
uint64_t MatrixEndTimeMicros = 0; // End time of state matrix (in us)
volatile uint32_t currentTimeMicros = 0; // Current time (for detecting 32-bit clock rollovers)
volatile uint32_t lastTimeMicros = 0; // Last time read (for detecting  32-bit clock rollovers)
unsigned long nCyclesCompleted= 0; // Number of HW timer cycles since state matrix started
unsigned long StateStartTime = 0; // Session Start Time
unsigned long NextLEDBrightnessAdjustTime = 0; // Used to fade blue light when disconnected
unsigned long LEDTime = 0; // Used to determine when to fade the indicator light
unsigned long CurrentTime = 0; // Current time (units = timer cycles since start; used to control state transitions)
unsigned long TimeFromStart = 0;
uint64_t sessionStartTimeMicros = 0;
uint32_t nMicrosRollovers = 0; // Number of micros() clock rollovers since session start
boolean cycleMonitoring = 0; // if 1, measures time between hardware timer callbacks when state transitions occur
boolean getModuleInfo = false; // If retrieving module info
unsigned long nBytes = 0; // Number of bytes to read (when transmitting module info)
unsigned long CallbackStartTime = 0; // For self-monitoring to detect hardware timer overruns
unsigned long DiscoveryByteTime = 0; // Last time a discovery byte was sent (for USB serial port auto-discovery)
unsigned long nCycles = 0; // Number of cycles measured since event X
byte connectionState = 0; // 1 if connected to MATLAB
byte RunningStateMatrix = 0; // 1  if state matrix is running
byte firstLoop= 0; // 1 if first timer callback in state matrix
int Ev = 0; // Index of current event
byte overrideChan = 0; // Output channel being manually overridden
uint16_t overrideChanState = 0; // State of channel being manually overridden (16-bit to hold PWM value = 256 on Teensy if necessary)
byte nOverrides = 0; // Number of overrides on a line of the state matrix (for compressed transmission scheme)
uint16_t col = 0; uint16_t val = 0; // col and val are used in state matrix compression scheme
const uint16_t StateMatrixBufferSize = 50000;
#if MACHINE_TYPE > 1
  byte StateMatrixBuffer[StateMatrixBufferSize] = {0}; // Stores next trial's state matrix
#endif
const uint16_t SerialRelayBufferSize = 256;
byte SerialRelayBuffer[SerialRelayBufferSize] = {0}; // Stores bytes to be transmitted to a serial device (i.e. module, USB)
uint16_t bufferPos = 0;
boolean smaPending = false; // If a state matrix is ready to read into the serial buffer (from USB)
boolean smaReady2Load = false; // If a state matrix was read into the serial buffer and is ready to read into sma vars with LoadStateMatrix()
boolean runFlag = false; // True if a command to run a state matrix arrives while an SM transmission is ongoing. Set to false once new SM starts.
uint16_t nSMBytesRead = 0; // For state matrix transmission during trial, where bytes transmitted must be tracked across timer interrupts
boolean startFlag = false; // True if a USB 'R' code was received in a timer callback. Used to start the state matrix from the main loop.
boolean firstTrialFlag = false; // True if running the first trial of the session
boolean acquiringAnalog = false; // True if analog samples are being measured and returned to PC
byte flexChannelToSet = 0; // For configuring flex channels
byte thresholdIndexToSet = 0; // For configuring flex channels
byte thisAnalogEnable = 0; // For configuring analog thresholds
byte adcBufferPos = 1;
byte hardwareRevision = 0;
byte ModuleProbeBytes[2] = {255,255};

#if MACHINE_TYPE == 4
  // Analog Input Threshold Vars
  uint16_t analogThreshold1[nFlexIO] = {4095}; // Analog value of threshold# 1
  uint16_t analogThreshold2[nFlexIO] = {4095}; // Analog value of threshold# 2
  byte analogThreshold1Polarity[nFlexIO] = {0}; // 0: Trig if val > T, 1: Trig if val < T
  byte analogThreshold2Polarity[nFlexIO] = {0}; 
  byte analogThreshold1Enabled[nFlexIO] = {0}; // Only enabled thresholds generate events. Thresholds are disabled after being triggered.
  byte analogThreshold2Enabled[nFlexIO] = {0};
  byte analogThresholdMode[nFlexIO] = {0}; // 0: T1 and T2 are Independent thresholds with manual reset, 1: Crossing T2 resets T1, Crossing T1 resets T2
#endif

union {
  byte Bytes[maxCurrentEvents*4];
  uint32_t Uint32[maxCurrentEvents];
} CurrentTimeStamps; // For trial timestamp conversion

union {
    byte byteArray[4];
    uint16_t uint16;
    uint32_t uint32;
} typeBuffer; // For general purpose type conversion

union {
    byte byteArray[16];
    uint32_t uint32[4];
    uint64_t uint64[2];
} timeBuffer; // For time transmission on trial end

#if MACHINE_TYPE == 4 
union {
    byte byteArray[10];
    uint16_t uint16[5];
} adcBuffer; // For transmitting ADC data
#endif

#if MACHINE_TYPE > 2
  IntervalTimer hardwareTimer;
  elapsedMicros teensyMicros;
#endif


void setup() {
  // Resolve hardware peripherals from machine type
  if (MACHINE_TYPE == 1) {
      usesFRAM = false;
      usesSPISync = true;
      usesSPIValves = true;
      usesUARTInputs = false;
      usesFlexIO = false;
      isolatorHigh = 1;
      isolatorLow = 0;
  } else if (MACHINE_TYPE == 2) {
      usesFRAM = true;
      usesSPISync = false;
      usesSPIValves = true;
      usesUARTInputs = true;
      usesFlexIO = false;
      isolatorHigh = 0;
      isolatorLow = 1;
  } else if (MACHINE_TYPE == 3) {
      usesFRAM = false;
      usesSPISync = false;
      usesSPIValves = false;
      usesUARTInputs = true;
      usesFlexIO = false;
      isolatorHigh = 0;
      isolatorLow = 1;
  } else if (MACHINE_TYPE == 4) {
      usesFRAM = false;
      usesSPISync = false;
      usesSPIValves = false;
      usesUARTInputs = true;
      usesFlexIO = true;
      isolatorHigh = 0;
      isolatorLow = 1;
  }
  // Find Bookmarks (positions of channel types in hardware description vectors)
  for (int i = 0; i < nInputs; i++) {
    if ((InputHW[i] == 'X') && (USBInputPos == 0)) {
      USBInputPos = i;
    }
    if ((InputHW[i] == 'B') && (BNCInputPos == 0)) {
      BNCInputPos = i;
    }
    if ((InputHW[i] == 'W') && (WireInputPos == 0)) {
      WireInputPos = i;
    }
    if ((InputHW[i] == 'P') && (PortInputPos == 0)) {
      PortInputPos = i;
    }
    if ((InputHW[i] == 'F') && (FlexInputPos == 0)) {
      FlexInputPos = i;
    }
  }
  DigitalInputPos = BNCInputPos;
  #if MACHINE_TYPE == 4
      DigitalInputPos = FlexInputPos;
  #endif
  
  for (int i = 0; i < nOutputs; i++) {
    if ((OutputHW[i] == 'X') && (USBOutputPos == 0)) {
      USBOutputPos = i;
    }
    if ((OutputHW[i] == 'B') && (BNCOutputPos == 0)) {
      BNCOutputPos = i;
    }
    if ((OutputHW[i] == 'W') && (WireOutputPos == 0)) {
      WireOutputPos = i;
    }
    if ((OutputHW[i] == 'P') && (PortOutputPos == 0)) {
      PortOutputPos = i;
    }
    if ((OutputHW[i] == 'V') && (ValvePos == 0)) {
      ValvePos = i;
    }
    if ((OutputHW[i] == 'F') && (FlexOutputPos == 0)) {
      FlexOutputPos = i;
    }
  }
  
  SPI.begin();
  #if MACHINE_TYPE == 4
    FlexIO.nReadsPerMeasurement = 3; // Configure 3x oversampling for analog inputs by default
    setFlexIOChannelTypes();
    memory_begin = (uint32_t *)(0x70000000); // PSRAM start address
    memory_end = (uint32_t *)(0x70000000 + external_psram_size * 1048576); // PSRAM end address
  #endif
  
  // Configure input channels
  Byte1 = 0;
  for (int i = 0; i < nInputs; i++) {
    switch (InputHW[i]) {
      case 'D':
      case 'F':
        inputState[i] = 0;
        lastInputState[i] = 0;
        inputEnabled[i] = 1;
        logicHigh[i] = 1;
        logicLow[i] = 0;
      break;
      case 'B':
      case 'W':
        pinMode(InputCh[i], INPUT);
        inputEnabled[i] = 1;
        #if MACHINE_TYPE > 1
          inputState[i] = 1;
          lastInputState[i] = 1;
        #endif
        logicHigh[i] = isolatorHigh;
        logicLow[i] = isolatorLow;
      break; 
      case 'P':
        pinMode(InputCh[i], INPUT_PULLUP);
        logicHigh[i] = 1;
      break;  
      case 'U': 
          switch(Byte1) {
            case 0:
              Serial1.begin(SerialBaudRate); Byte1++;
              #if MACHINE_TYPE > 2
                Serial1.addMemoryForRead(HWSerialBuf1, 192);
              #endif
            break;
            case 1:
              Serial2.begin(SerialBaudRate); Byte1++;
              #if MACHINE_TYPE > 2
                Serial2.addMemoryForRead(HWSerialBuf2, 192);
              #endif
            break;
            #if MACHINE_TYPE == 2 
                case 2:
                  Serial3.begin(SerialBaudRate); Byte1++;
                break;
            #elif MACHINE_TYPE == 3
                case 2:
                  #if MACHINE_BUILD == 0
                    Serial3.begin(SerialBaudRate); Byte1++;
                    Serial3.addMemoryForRead(HWSerialBuf3, 192);
                  #elif MACHINE_BUILD == 1
                    Serial6.begin(SerialBaudRate); Byte1++;
                    Serial6.addMemoryForRead(HWSerialBuf3, 192);
                  #endif
                break;
            #elif MACHINE_TYPE == 4
                case 2:
                  Serial6.begin(SerialBaudRate); Byte1++;
                  Serial6.addMemoryForRead(HWSerialBuf3, 192);
                break;
            #endif  
            #if MACHINE_TYPE == 3
              case 3:
                #if MACHINE_BUILD == 0
                  Serial4.begin(SerialBaudRate); Byte1++;
                  Serial4.addMemoryForRead(HWSerialBuf4, 192);
                #elif MACHINE_BUILD == 1
                  Serial7.begin(SerialBaudRate); Byte1++;
                  Serial7.addMemoryForRead(HWSerialBuf4, 192);
                #endif
              break;
              #if ETHERNET_COM == 0
                case 4:
                  #if MACHINE_BUILD == 0
                    Serial5.begin(SerialBaudRate); Byte1++;
                    Serial5.addMemoryForRead(HWSerialBuf5, 192);
                  #elif MACHINE_BUILD == 1
                    Serial8.begin(SerialBaudRate); Byte1++;
                    Serial8.addMemoryForRead(HWSerialBuf5, 192);
                  #endif
                break;
              #endif
            #endif
          }
      break;
    }
  }
  resetSerialMessages();
  Byte1 = 0;
  // Configure digital output channels
  for (int i = 0; i < nOutputs; i++) {
    switch (OutputHW[i]) {
      case 'D':
      case 'B':
      case 'W':
      case 'V':
        pinMode(OutputCh[i], OUTPUT);
        digitalWrite(OutputCh[i], LOW);
      break;
      case 'P':
        pinMode(OutputCh[i], OUTPUT);
        analogWrite(OutputCh[i], 0);
        #if MACHINE_TYPE > 2 
          analogWriteFrequency(OutputCh[i],50000); // Set PWM cycle frequency of channel (and others on same port) to 50kHz
        #endif
      break;
    }
  }
  
  // Start UART --> Ethernet
  #if ETHERNET_COM == 1 && MACHINE_TYPE == 3
     #if MACHINE_BUILD == 0
      Serial5.begin(2625000);
     #elif MACHINE_BUILD == 1
      Serial8.begin(2625000);
     #endif
     
  #endif  
  Byte1 = 0;
  pinMode(RedLEDPin, OUTPUT);
  pinMode(GreenLEDPin, OUTPUT);
  pinMode(BlueLEDPin, OUTPUT);
  #if MACHINE_TYPE == 3
    pinMode(ValveEnablePin, OUTPUT);
    digitalWrite(ValveEnablePin, LOW);
  #endif
  if (usesFRAM) {
    pinMode(fRAMcs, OUTPUT); // CS pin for the fRAM IC
    pinMode(fRAMhold, OUTPUT); // Hold pin for the fRAM IC
    digitalWrite(fRAMcs, HIGH);
    digitalWrite(fRAMhold, HIGH);
  }
  if (usesSPISync) {
     pinMode(SyncRegisterLatch, OUTPUT); // CS pin for sync shift register IC
     digitalWrite(SyncRegisterLatch, LOW);
  }
  // Read hardware revision from circuit board (an array of grounded pins indicates revision in binary, grounded = 1, floating = 0)
  hardwareRevision = 0;
  for (int i = 0; i < 5; i++) {
    pinMode(hardwareRevisionArray[i], INPUT_PULLUP);
    hardwareRevision += pow(2, i)*digitalRead(hardwareRevisionArray[i]);
    pinMode(hardwareRevisionArray[i], INPUT);
  }
  hardwareRevision = 31-hardwareRevision;
  CurrentEventBuffer[0] = 1;
  updateStatusLED(0); // Begin the blue light display ("Disconnected" state)
  #if MACHINE_TYPE < 3
    Timer3.attachInterrupt(handler); // Timer3 is Arduino Due's hardware timer, which will trigger the function "handler" precisely every (timerPeriod) us
    Timer3.start(timerPeriod); // Start HW timer
  #else
    hardwareTimer.begin(handler, timerPeriod); // hardwareTimer is Teensy 3.6's hardware timer
  #endif
}

void loop() {
  if (!RunningStateMatrix) {
    relayModuleBytes();
  }
  #if MACHINE_TYPE > 1
    if (smaPending) { // If a request to read a new state matrix arrived during a trial, read here at lower priority (in free time between timer interrupts)
      PC.readByteArray(StateMatrixBuffer, stateMatrixNBytes);
      smaTransmissionConfirmed = true;
      smaPending = false;
      if (RunningStateMatrix) {
        smaReady2Load = true;
      } else {
        loadStateMatrix();
        if (RunStateMatrixASAP) {
          RunStateMatrixASAP = false;
          startFlag = true; // Start the state matrix without waiting for an explicit 'R' command.
        }
      }
    }
  #endif
  #if MACHINE_TYPE < 3
    currentTimeMicros = micros();
    if (currentTimeMicros < lastTimeMicros) {
      nMicrosRollovers++;
    }
    lastTimeMicros = currentTimeMicros;
  #endif
  if (startFlag == true) {
     startSM();
     startFlag = false;
  }
}

void handler() { // This is the timer handler function, which is called every (timerPeriod) us
  #if MACHINE_TYPE > 2
    currentTimeMicros = teensyMicros;
    if (currentTimeMicros < lastTimeMicros) {
      nMicrosRollovers++;
    }
    lastTimeMicros = currentTimeMicros; 
  #endif
  if (connectionState == 0) { // If not connected to Bpod software
    if (millis() - DiscoveryByteTime > discoveryByteInterval) { // At a fixed interval, send discovery byte to any connected USB serial port
      #if ETHERNET_COM == 0
        #if MACHINE_TYPE > 2
          PC.writeByte(discoveryByte);
        #else
          if (SerialUSB.dtr()) {
            PC.writeByte(discoveryByte);
          }
        #endif
      #endif
      DiscoveryByteTime = millis();
    }
    updateStatusLED(1); // Update the indicator LED (cycles blue intensity)
  }
  if (runFlag) { // If an SM run command was delayed because an SM transmission is ongoing
    if (smaTransmissionConfirmed) {
      runFlag = false;
      startFlag == true;
    }
  }
  if (getModuleInfo) { // If a request was sent for connected modules to return self description (request = byte 255)
    nCycles++; // Count cycles since request was sent
    if (nCycles > ModuleIDTimeout) { // If modules have had time to reply
      getModuleInfo = false; 
      relayModuleInfo(Module1, 1); // Function transmits 0 if no module replied, 1 if found, followed by length of description(bytes), then description
      relayModuleInfo(Module2, 2);
      #if MACHINE_TYPE > 1
        relayModuleInfo(Module3, 3);
      #endif
      #if MACHINE_TYPE == 3
        relayModuleInfo(Module4, 4);
        #if ETHERNET_COM == 0
          relayModuleInfo(Module5, 5);
        #endif
      #endif
    }
  }
  if ((PC.available() > 0) && (smaPending == false)) { // If a message has arrived on the USB serial port
    CommandByte = PC.readByte();  // P for Program, R for Run, O for Override, 6 for Handshake, F for firmware version, etc
    switch (CommandByte) {
      case '6':  // Initialization handshake
        connectionState = 1;
        updateStatusLED(2);
        PC.writeByte(53);
        delayMicroseconds(100000);
        PC.flush();
        resetSessionClock();
        resetSerialMessages();
        disableModuleRelays();
        #if MACHINE_TYPE == 3
          digitalWrite(ValveEnablePin, HIGH); // Enable valve driver
        #endif
      break;
      case 'F':  // Return major firmware version and machine type
        PC.writeUint16(FIRMWARE_VERSION_MAJOR);
        PC.writeUint16(MACHINE_TYPE);
      break;
      case 'f': // Return minor firmware version
        PC.writeUint16(FIRMWARE_VERSION_MINOR);
      break;
      case 'v': // Return state machine circuit board revision
        PC.writeByte(hardwareRevision);
      break;
      case '{': // Send handshake response byte on SerialUSB1 (used for soft codes from a third-party app (e.g. Bonsai))
        #if MACHINE_TYPE > 2
          PC1.writeByte(222);
        #endif
      break;
      case '}': // Send handshake response byte on SerialUSB2 (used for analog data)
        #if MACHINE_TYPE == 4
          PC2.writeByte(223);
        #endif
      break;
      case '*': // Reset session clock
        resetSessionClock();
        trialCounter = 0;
        PC.writeByte(1);
      break;
      case ':': // Enable/Disable status LED
        statusLEDEnabled = PC.readByte();
        if (statusLEDEnabled) {
          if (connectionState == 1) {
            updateStatusLED(2);
          } else {
            updateStatusLED(1);
          }
        } else {
          updateStatusLED(0);
        }
        PC.writeByte(1);
      break;
      case '$': // Pause ongoing trial (We recommend using computer-side pauses between trials, to keep data uniform)
        RunningStateMatrix = PC.readByte();
      break;
      case 'G':  // Return timestamp transmission scheme
        PC.writeByte(LIVE_TIMESTAMPS);
      break;
      case 'H': // Return local hardware configuration
        PC.writeUint16(MaxStates);
        PC.writeUint16(timerPeriod);
        PC.writeByte(maxSerialEvents);
        PC.writeByte(SerialMessageMaxBytes);
        PC.writeByte(MAX_GLOBAL_TIMERS);
        PC.writeByte(MAX_GLOBAL_COUNTERS);
        PC.writeByte(MAX_CONDITIONS);
        PC.writeByte(nInputs);
        PC.writeByteArray(InputHW, nInputs);
        PC.writeByte(nOutputs);
        for (int i = 0; i < nOutputs; i++) {
          if (OutputHW[i] == 'Y') {
             PC.writeByte(SyncChannelOriginalType);
          } else {
             PC.writeByte(OutputHW[i]);
          }
        }
      break;
      case 'M': // Probe for connected modules and return module information
        while (Module1.available() > 0 ) {
           Module1.readByte();
        }
        while (Module2.available() > 0 ) {
           Module2.readByte();
        }
        #if MACHINE_TYPE > 1
          while (Module3.available() > 0 ) {
            Module3.readByte();
          }
        #endif
        #if MACHINE_TYPE == 3
          while (Module4.available() > 0 ) {
             Module4.readByte();
          }
          #if ETHERNET_COM == 0
            while (Module5.available() > 0 ) {
               Module5.readByte();
            }
          #endif
        #endif
        Module1.writeByteArray(ModuleProbeBytes, 2);
        Module2.writeByteArray(ModuleProbeBytes, 2);
        #if MACHINE_TYPE > 1
          Module3.writeByteArray(ModuleProbeBytes, 2);
        #endif
        #if MACHINE_TYPE == 3
          Module4.writeByteArray(ModuleProbeBytes, 2);
          #if ETHERNET_COM == 0
            Module5.writeByteArray(ModuleProbeBytes, 2);
          #endif
        #endif
        nCycles = 0; // Number of cycles since request was sent
        getModuleInfo = true; // Outgoing serial messages are not sent until after timer handler completes - so replies are forwarded to USB in the main loop.
      break;
      case '%': // Set number of events per module (this should be run from MATLAB after all modules event # requests are received by MATLAB and a determination is made how to allocate.
        PC.readByteArray(nModuleEvents, nSerialChannels);
        PC.writeByte(1);
      break;
      case 'E': // Enable ports
        for (int i = 0; i < nInputs; i++) {
          inputEnabled[i] = PC.readByte();
        }
        PC.writeByte(1);
      break;
      case 'J': // set serial module relay mode (when not running a state machine, relays one port's incoming bytes to MATLAB/Python
        disableModuleRelays();
        clearSerialBuffers();
        Byte1 = PC.readByte();
        Byte2 = PC.readByte();
        UARTrelayMode[Byte1] = Byte2;
      break;
      case 'K': // Set sync channel and mode
      NewSyncChannel = PC.readByte();
      SyncMode = PC.readByte();
      if (!usesSPISync) {
        if (NewSyncChannel != SyncChannel){ 
          if (NewSyncChannel == 255) {
            if (SyncChannel < nOutputs) {
              OutputHW[SyncChannel] = SyncChannelOriginalType;
            }
            syncOn = false;
          } else {
            if (NewSyncChannel < nOutputs) {
              if (SyncChannel < 255) {
                if (OutputHW[SyncChannel] == 'Y') {
                  OutputHW[SyncChannel] = SyncChannelOriginalType;
                }
              }
              SyncChannelOriginalType = OutputHW[NewSyncChannel];
              OutputHW[NewSyncChannel] = 'Y';
              syncOn = true;
              SyncChannelHW = OutputCh[NewSyncChannel];
            }
          }
          SyncChannel = NewSyncChannel;
        }
      }
      PC.writeByte(1);
      break;
      case 'Q': // Set FlexIO channel configuration
        #if MACHINE_TYPE == 4
          PC.readByteArray(flexIOChannelType, nFlexIO);
          setFlexIOChannelTypes();
          PC.writeByte(1);
        #endif
      break;   
      case 'q': // Return current FlexIO channel configuration
        #if MACHINE_TYPE == 4
          PC.writeByteArray(flexIOChannelType,nFlexIO);
        #endif
      break;
      #if MACHINE_TYPE == 4
        case '^': // Set FlexIO analog input sampling rate
            flexIO_ADC_Sample_Interval = PC.readUint32();
            flexIO_ADC_Sample_Clock = 0;
            PC.writeByte(1);
        break;
        case 'o': // Set FlexIO oversampling nSamples
          FlexIO.nReadsPerMeasurement = PC.readByte();
          PC.writeByte(1);
        break;
        case '.': // Return FlexIO oversampling nSamples
          PC.writeByte(FlexIO.nReadsPerMeasurement);
        break;
        case 't': // Set analog threshold value
          PC.readUint16Array(analogThreshold1, nFlexIO);
          PC.readUint16Array(analogThreshold2, nFlexIO);
          PC.writeByte(1);
        break;
        case 'p': // Set analog threshold polarity
          PC.readByteArray(analogThreshold1Polarity, nFlexIO);
          PC.readByteArray(analogThreshold2Polarity, nFlexIO);
          PC.writeByte(1);
        break;
        case 'm': // Set analog threshold mode
          PC.readByteArray(analogThresholdMode, nFlexIO);
          PC.writeByte(1);
        break; 
        case 'e': // Set analog threshold enabled/disabled
          flexChannelToSet = PC.readByte();
          thresholdIndexToSet = PC.readByte();
          if (flexChannelToSet < nFlexIO) {
            switch (thresholdIndexToSet) {
              case 0:
                analogThreshold1Enabled[flexChannelToSet] = PC.readByte();
                PC.writeByte(1);
              break;
              case 1:
                analogThreshold2Enabled[flexChannelToSet] = PC.readByte();
                PC.writeByte(1);
              break;
            }
          }
        break;
        case '_': // Test PSRAM
          // This memory test was adopted from PJRC's teensy41_psram_memtest repository : https://github.com/PaulStoffregen/teensy41_psram_memtest
          // Thanks Paul!
            PC.writeByte(external_psram_size);
            memOK = true;
            if (!check_fixed_pattern(0x55555555)) {memOK = false;}
            if (!check_fixed_pattern(0x33333333)) {memOK = false;}
            if (!check_fixed_pattern(0x0F0F0F0F)) {memOK = false;}
            if (!check_fixed_pattern(0x00FF00FF)) {memOK = false;}
            if (!check_fixed_pattern(0x0000FFFF)) {memOK = false;}
            if (!check_fixed_pattern(0xAAAAAAAA)) {memOK = false;}
            if (!check_fixed_pattern(0xCCCCCCCC)) {memOK = false;}
            if (!check_fixed_pattern(0xF0F0F0F0)) {memOK = false;}
            if (!check_fixed_pattern(0xFF00FF00)) {memOK = false;}
            if (!check_fixed_pattern(0xFFFF0000)) {memOK = false;}
            if (!check_fixed_pattern(0xFFFFFFFF)) {memOK = false;}
            if (!check_fixed_pattern(0x00000000)) {memOK = false;}
            PC.writeByte(memOK);
        break;
      #endif    

      case 'O':  // Override digital hardware state
        overrideChan = PC.readByte();
        overrideChanState = PC.readByte();
        switch (OutputHW[overrideChan]) {
          case 'D':
          case 'B':
          case 'W':
            digitalWriteDirect(OutputCh[overrideChan], overrideChanState);
            outputOverrideState[overrideChan] = overrideChanState;
          break;
          case 'V':
            if (usesSPIValves) {
              outputState[overrideChan] = overrideChanState;
              valveWrite();
            } else {
              digitalWriteDirect(OutputCh[overrideChan], overrideChanState);
            }
            outputOverrideState[overrideChan] = overrideChanState;
          break;
          case 'P':
            pwmWrite(OutputCh[overrideChan], overrideChanState);
            if (overrideChanState > 0) {
              outputOverrideState[overrideChan] = true;
            } else {
              outputOverrideState[overrideChan] = false;
            }
          break;
        }
      break;
      case 'I': // Read and return digital input line states (for debugging)
        Byte1 = PC.readByte();
        Byte2 = digitalReadDirect(InputCh[Byte1]);
        Byte2 = (Byte2 == logicHigh[Byte1]);
        PC.writeByte(Byte2);
      break;
      case 'Z':  // Cleanup - PC has closed the client program
        disableModuleRelays();
        connectionState = 0;
        PC.writeByte('1');
        DiscoveryByteTime = millis();
        NextLEDBrightnessAdjustTime = DiscoveryByteTime;
        updateStatusLED(0);
        #if MACHINE_TYPE == 3
          digitalWrite(ValveEnablePin, LOW); // Disable valve driver
        #endif
      break;
      case 'S': // Echo Soft code.
        VirtualEventData = PC.readByte();
        PC.writeByte(2);
        PC.writeByte(VirtualEventData);
      break;
      case 'T': // Receive bytes from USB and send to hardware serial channel 1-5
        Byte1 = PC.readByte() - 1; // Serial channel
        nBytes = PC.readUint8();
        switch (Byte1) {
          case 0:
            PC.readByteArray(SerialRelayBuffer, nBytes);
            Module1.writeByteArray(SerialRelayBuffer, nBytes);
          break;
          case 1:
            PC.readByteArray(SerialRelayBuffer, nBytes);
            Module2.writeByteArray(SerialRelayBuffer, nBytes);
          break;
          #if MACHINE_TYPE > 1
            case 2:
              PC.readByteArray(SerialRelayBuffer, nBytes);
              Module3.writeByteArray(SerialRelayBuffer, nBytes);
            break;
          #endif
          #if MACHINE_TYPE == 3
            case 3:
              PC.readByteArray(SerialRelayBuffer, nBytes);
              Module4.writeByteArray(SerialRelayBuffer, nBytes);
            break;
            #if ETHERNET_COM == 0
              case 4:
                  PC.readByteArray(SerialRelayBuffer, nBytes);
                  Module5.writeByteArray(SerialRelayBuffer, nBytes);
              break;
            #endif
          #endif
         }
      break;
      case 'U': // Recieve serial message index from USB and send corresponding message to hardware serial channel 1-5
        Byte1 = PC.readByte() - 1;
        Byte2 = PC.readByte();
        Byte3 = SerialMessage_nBytes[Byte2][Byte1];
        for (int i = 0; i < Byte3; i++) {
           serialByteBuffer[i] = SerialMessageMatrix[Byte2][Byte1][i];
        }
        switch (Byte1) {
          case 0:
            Module1.writeByteArray(serialByteBuffer, Byte3);
          break;
          case 1:
            Module2.writeByteArray(serialByteBuffer, Byte3);
          break;
          #if MACHINE_TYPE > 1
            case 2:
              Module3.writeByteArray(serialByteBuffer, Byte3);
            break;
          #endif
          #if MACHINE_TYPE == 3
            case 3:
              Module4.writeByteArray(serialByteBuffer, Byte3);
            break;
            #if ETHERNET_COM == 0
              case 4:
                Module5.writeByteArray(serialByteBuffer, Byte3);
              break;
            #endif
          #endif
        }
        break;
      case 'L': // Load serial message library
        Byte1 = PC.readByte(); // Serial Channel
        Byte2 = PC.readByte(); // nMessages arriving
        for (int i = 0; i < Byte2; i++) {
          Byte3 = PC.readByte(); // Message Index
          Byte4 = PC.readByte(); // Message Length
          SerialMessage_nBytes[Byte3][Byte1] = Byte4;
          for (int j = 0; j < Byte4; j++) {
            SerialMessageMatrix[Byte3][Byte1][j] = PC.readByte();
          }
        }
        PC.writeByte(1);
      break;
      case '>': // Reset serial messages to equivalent byte codes (i.e. message# 4 = one byte, 0x4)
        resetSerialMessages();
        PC.writeByte(1);
      break;
      case 'V': // Manual override: execute virtual event
        VirtualEventTarget = PC.readByte();
        VirtualEventData = PC.readByte();
        if (RunningStateMatrix) {
           inputState[VirtualEventTarget] = VirtualEventData;
           inputOverrideState[VirtualEventTarget] = true;
        }
      break;
      case '~': // USB soft code
          SoftEvent1 = PC.readByte();
          if (!RunningStateMatrix) {
            SoftEvent1 = 255; // 255 = No Event
          }
      break;
      case 'C': // Get new compressed state matrix from MATLAB/Python 
        newSMATransmissionStarted = true;
        smaTransmissionConfirmed = false;
        PC.readByteArray(typeBuffer.byteArray, 4);
        RunStateMatrixASAP = typeBuffer.byteArray[0];
        using255BackSignal = typeBuffer.byteArray[1];
        typeBuffer.byteArray[0] = typeBuffer.byteArray[2];
        typeBuffer.byteArray[1] = typeBuffer.byteArray[3];
        stateMatrixNBytes = typeBuffer.uint16;
        if (stateMatrixNBytes < StateMatrixBufferSize) {
          if (RunningStateMatrix) {
            #if MACHINE_TYPE > 1
              nSMBytesRead = 0;
              smaPending = true;
            #else
              PC.writeByte(0);
            #endif
          } else {
            #if TEENSY_VERSION == 4
              nSMBytesRead = 0;
              smaPending = true;
            #elif MACHINE_TYPE < 4
              #if MACHINE_TYPE > 1
                PC.readByteArray(StateMatrixBuffer, stateMatrixNBytes); // Read data in 1 batch operation (much faster than item-wise)
                smaTransmissionConfirmed = true;
              #endif
              loadStateMatrix(); // Loads the state matrix from the buffer into the relevant variables
              if (RunStateMatrixASAP) {
                RunStateMatrixASAP = false;
                startFlag = true; // Start the state matrix without waiting for an explicit 'R' command.
              }
            #endif
          }
        }
      break;
      case 'R':  // Run State Machine
        startFlag = true;
      break;
      case 'X':   // Exit state matrix and return data
        MatrixFinished = true;
        RunningStateMatrix = false;
        resetOutputs(); // Returns all lines to low by forcing final state
        acquiringAnalog = false;
      break;
      case 'B': // Switch baud rate of specified module port to be RS232 friendly
        arg1 = PC.readByte(); // Module port 1-5 or ? for baud probing
        arg2 = PC.readByte(); // 0 original baud, 1 reduced baud; or port no. for baud probing
        int baudrate = 0;
        int port = 0;

        if (arg1 == '?')
          if(1 =< arg2 <= 5)
            port = arg2;
          else
            PC.writeInt8(2); // Invalid port number
            break;
        else
          {
            if (arg2 == 1) 
              baudrate = MFCBaudRate;
            else if (arg2 == 0)
              baudrate = SerialBaudRate;
            else
              PC.writeInt8(3); // Invalid state
              break;
          }

        switch (arg1) {
          case 1:
            Serial1.end();
            Serial1.begin(baudrate);
            Serial1.addMemoryForRead(HWSerialBuf1, 192);
            current_baudrates[0] = baudrate;
            PC.writeUint8(1); // Success! 
          break;
          
          case 2:
            Serial2.end();
            Serial2.begin(baudrate);
            Serial2.addMemoryForRead(HWSerialBuf2, 192); 
            current_baudrates[1] = baudrate;
            PC.writeUint8(1); // Success!
          break;
          
          case 3:
            Serial6.end();
            Serial6.begin(baudrate);
            Serial6.addMemoryForRead(HWSerialBuf3, 192);
            current_baudrates[2] = baudrate;
            PC.writeUint8(1); // Success!
          break;
          
          case 4:
            Serial7.end();
            Serial7.begin(baudrate);
            Serial7.addMemoryForRead(HWSerialBuf4, 192);
            current_baudrates[3] = baudrate;  
            PC.writeUint8(1); // Success!          
          break;
          
          case 5:
            Serial8.end();
            Serial8.begin(baudrate);
            Serial8.addMemoryForRead(HWSerialBuf5, 192);  
            current_baudrates[4] = baudrate;
            PC.writeUint8(1); // Success!
          break;

          case '?':
            baudrate = current_baudrates[port-1];
            PC.writeUint8(baudrate);
          break;

          else:
            PC.writeUint8(4); // Invalid command
          break;
        }

        break;
      break;
    } // End switch commandbyte
  } // End SerialUSB.available
  #if MACHINE_TYPE > 2
    if (PC1.available()>0) {
      SoftEvent2 = PC1.readByte();
      if (!RunningStateMatrix) {
        SoftEvent2 = 255; // 255 = No Event
      }
    }
  #endif
  #if MACHINE_TYPE == 4
    if (acquiringAnalog) { 
      if (FlexIO.nADC > 0) {
        flexIO_ADC_Sample_Clock++;
        if (flexIO_ADC_Sample_Clock == flexIO_ADC_Sample_Interval) {
          flexIO_ADC_Sample_Clock = 0;          
          FlexIO.readADC();
          flexIO_updateAIflag = true;
        }
      }
    }
  #endif
  if (RunningStateMatrix) {
    if (firstLoop == 1) { 
      firstLoop = 0;
      #if MACHINE_TYPE == 4
        if (!flexIO_updateAIflag) {
          delayMicroseconds(1); // Delay so that the i/o + timing matches subsequent loops (which include ~1us of processing first)
        }
      #elif MACHINE_TYPE == 3
        #if MACHINE_BUILD == 0
          delayMicroseconds(5); // Delay so that the i/o + timing matches subsequent loops (which include ~5us of processing first)
        #else
          delayMicroseconds(1);
        #endif
      #else
        delayMicroseconds(25); // Delay so that the i/o + timing matches subsequent loops (which include ~25us of processing first)
      #endif
      trialCounter++;
      MatrixStartTimeMicros = sessionTimeMicros(); 
      timeBuffer.uint64[0] = MatrixStartTimeMicros;
      PC.writeByteArray(timeBuffer.byteArray,8); // Send trial-start timestamp (from micros() clock)
      SyncWrite(); 
      SyncTimer = 0;
      setStateOutputs(CurrentState); // Adjust outputs, global timers, serial codes and sync port for first state
      #if MACHINE_TYPE == 4
        updateFlexOutputs();
        if (firstTrialFlag) {
          flexIO_ADC_Sample_Clock = flexIO_ADC_Sample_Interval-1; // Start fist analog sample at session t=0
          acquiringAnalog = true; // Acquire analog if any channels are configued as AIN
        }
      #endif
      firstTrialFlag = false;
    } else {
      CurrentTime++; 
      SyncTimer++;
      for (int i = BNCInputPos; i < nInputs; i++) {
        if (inputEnabled[i] && !inputOverrideState[i]) {
          inputState[i] = digitalReadDirect(InputCh[i]); 
        } 
      }
      #if MACHINE_TYPE == 4 
        if (FlexIO.nDI > 0) {
          FlexIO.readDI();
          for (int i = 0; i < nFlexIO; i++) {
            if (flexIOChannelType[i] == 0) {
              if (inputEnabled[i+FlexInputPos] && !inputOverrideState[i+FlexInputPos]) {
                inputState[i+FlexInputPos] = FlexIO.getDI(i);
              }
            } 
          }
        }
      #endif
      
      // Determine if a handled condition occurred
      Ev = ConditionPos;
      for (int i = 0; i < nConditionsUsed; i++) {
        if (ConditionMatrix[CurrentState][i] != CurrentState) { // If this condition is handled
          if (inputState[ConditionChannels[i]] == ConditionValues[i]) {
            CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
          }
        }
        Ev++;
      }

      #if MACHINE_TYPE == 4 
        // Determine if an analog input threshold was crossed
        Ev = maxSerialEvents;
        for (int i = 0; i < nFlexIO; i++) {
         if (flexIOChannelType[i] == 2) {
            if (flexIO_updateAIflag) {
              uint16_t thisAnalogVal = FlexIO.adcReadout[i];
              if (analogThreshold1Enabled[i]) {
                if (analogThreshold1Polarity[i] == 0) {
                  if (thisAnalogVal > analogThreshold1[i]) {
                    CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
                    analogThreshold1Enabled[i] = 0;
                    if (analogThresholdMode[i] == 1) {
                      analogThreshold2Enabled[i] = 1;
                    }
                  }
                } else {
                  if (thisAnalogVal < analogThreshold1[i]) {
                    CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
                    analogThreshold1Enabled[i] = 0;
                    if (analogThresholdMode[i] == 1) {
                      analogThreshold2Enabled[i] = 1;
                    }
                  }
                }
              }
              Ev++;
              if (analogThreshold2Enabled[i]) {
                if (analogThreshold2Polarity[i] == 0) {
                  if (thisAnalogVal > analogThreshold2[i]) {
                    CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
                    analogThreshold2Enabled[i] = 0;
                    if (analogThresholdMode[i] == 1) {
                      analogThreshold1Enabled[i] = 1;
                    }
                  }
                } else {
                  if (thisAnalogVal < analogThreshold2[i]) {
                    CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
                    analogThreshold2Enabled[i] = 0;
                    if (analogThresholdMode[i] == 1) {
                      analogThreshold1Enabled[i] = 1;
                    }
                  }
                }
              }
              Ev++;
            }
          }
        }
      #endif
      
      // Determine if a digital low->high or high->low transition event occurred
      Ev = maxSerialEvents;
      for (int i = DigitalInputPos; i < nInputs; i++) {
          if (inputEnabled[i] == 1) {
              if ((inputState[i] == logicHigh[i]) && (lastInputState[i] == logicLow[i])) {
                lastInputState[i] = logicHigh[i]; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
              }
          }
          Ev++;
          if (inputEnabled[i] == 1) {
              if ((inputState[i] == logicLow[i]) && (lastInputState[i] == logicHigh[i])) {
                lastInputState[i] = logicLow[i]; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
              }
          }
          Ev++;
      }
      // Determine if a USB or hardware serial event occurred
      Ev = 0; Byte1 = 0;
      for (int i = 0; i < DigitalInputPos; i++) {
        switch(InputHW[i]) {
          case 'U':
          if (usesUARTInputs) {
            switch(Byte1) {
              case 0:
                if (Module1.available() > 0) {
                  Byte2 = Module1.readByte();
                  if (Byte2 <= nModuleEvents[0]) {
                    CurrentEvent[nCurrentEvents] = Byte2 + Ev-1; nCurrentEvents++; 
                  }
                }
                Byte1++;
              break;
              case 1:
                if (Module2.available() > 0) {
                  Byte2 = Module2.readByte();
                  if (Byte2 <= nModuleEvents[1]) {
                    CurrentEvent[nCurrentEvents] = Byte2 + Ev-1; nCurrentEvents++; 
                  }
                }
                Byte1++;
              break;
              case 2:
                #if MACHINE_TYPE > 1
                  if (Module3.available() > 0) {
                    Byte2 = Module3.readByte();
                    if (Byte2 <= nModuleEvents[2]) {
                      CurrentEvent[nCurrentEvents] = Byte2 + Ev-1; nCurrentEvents++; 
                    }
                  }
                #endif
                Byte1++;
              break;
              case 3:
                #if MACHINE_TYPE == 3
                  if (Module4.available() > 0) {
                    Byte2 = Module4.readByte();
                    if (Byte2 <= nModuleEvents[3]) {
                      CurrentEvent[nCurrentEvents] = Byte2 + Ev-1; nCurrentEvents++; 
                    }
                  }
                #endif
                Byte1++;
              break;
              case 4:
                #if MACHINE_TYPE == 3 && ETHERNET_COM == 0
                  if (Module5.available() > 0) {
                    Byte2 = Module5.readByte();
                    if (Byte2 <= nModuleEvents[4]) {
                      CurrentEvent[nCurrentEvents] = Byte2 + Ev-1; nCurrentEvents++; 
                    }
                  }
                #endif
                Byte1++;
              break;
            }
          }
          break;
          case 'X':
              if (SoftEvent1 < nModuleEvents[USBInputPos]) {
                CurrentEvent[nCurrentEvents] = SoftEvent1 + Ev; nCurrentEvents++;
                SoftEvent1 = 255;
              }
          break;
          case 'Z':
              if (SoftEvent2 < nModuleEvents[USBInputPos+1]) {
                CurrentEvent[nCurrentEvents] = SoftEvent2 + Ev; nCurrentEvents++;
                SoftEvent2 = 255;
              }
          break;
        }
        Ev += nModuleEvents[i];
      }
      Ev = GlobalTimerStartPos;
      // Determine if a global timer expired
      for (int i = 0; i < nGlobalTimersUsed; i++) {
        if (GlobalTimersActive[i] == true) {
          if (CurrentTime >= GlobalTimerEnd[i]) {
            setGlobalTimerChannel(i, 0);
            GlobalTimersTriggered[i] = false;
            GlobalTimersActive[i] = false;
            if (GlobalTimerLoop[i] > 0) {
              if (GlobalTimerLoopCount[i] < GlobalTimerLoop[i])  {
                GlobalTimersTriggered[i] = true;
                GlobalTimerStart[i] = CurrentTime + GlobalTimerLoopIntervals[i];
                GlobalTimerEnd[i] = GlobalTimerStart[i] + GlobalTimers[i];
                if (SendGlobalTimerEvents[i]) {
                  CurrentEvent[nCurrentEvents] = Ev+MAX_GLOBAL_TIMERS; nCurrentEvents++;
                }
                if (GTUsingLoopCounter[i]) {
                  GlobalTimerLoopCount[i] += 1;
                }
              } else {
                if (SendGlobalTimerEvents[i]) {
                  CurrentEvent[nCurrentEvents] = Ev+MAX_GLOBAL_TIMERS; nCurrentEvents++;
                }
              }
            } else {
              CurrentEvent[nCurrentEvents] = Ev+MAX_GLOBAL_TIMERS; nCurrentEvents++;
            }
          }
        } else if (GlobalTimersTriggered[i] == true) {
          if (CurrentTime >= GlobalTimerStart[i]) {
            GlobalTimersActive[i] = true;
            if (GlobalTimerLoop[i]) {
              if (SendGlobalTimerEvents[i]) {
                CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
              }
            } else {
              CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
            }
            if (GlobalTimerOnsetTriggers[i] > 0) {
              for (int j = 0; j < nGlobalTimersUsed; j++) {
                if (bitRead(GlobalTimerOnsetTriggers[i], j)) {
                  if (j != i) {
                    triggerGlobalTimer(j);
                  }
                }
              }
            }
            setGlobalTimerChannel(i, 1);
          }
        }
        Ev++;
      }
      
      Ev = GlobalCounterPos;
      // Determine if a global event counter threshold was exceeded
      for (int x = 0; x < nGlobalCountersUsed; x++) {
        if (GlobalCounterAttachedEvents[x] < 254) {
          // Check for and handle threshold crossing
          if ((GlobalCounterCounts[x] == GlobalCounterThresholds[x]) && (GlobalCounterHandled[x] == false)) {
            CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
            GlobalCounterHandled[x] = true;
          }
          // Add current event to count (Crossing triggered on next cycle)
          for (int i = 0; i < nCurrentEvents; i++) {
            if (CurrentEvent[i] == GlobalCounterAttachedEvents[x]) {
              GlobalCounterCounts[x] = GlobalCounterCounts[x] + 1;
            }
          }
        }
        Ev++;
      }
      // Determine if a state timer expired
      Ev = TupPos;
      TimeFromStart = CurrentTime - StateStartTime;
      if ((TimeFromStart >= StateTimers[CurrentState]) && (MeaningfulStateTimer == true)) {
        CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
      }
      if (nCurrentEvents > maxCurrentEvents) { // Drop events beyond maxCurrentEvents
        nCurrentEvents = maxCurrentEvents;
      }
      // Now determine if a state transition should occur. The first event linked to a state transition takes priority.
      byte StateTransitionFound = 0; int i = 0; int CurrentColumn = 0;
      NewState = CurrentState;
      while ((!StateTransitionFound) && (i < nCurrentEvents)) {
        if (CurrentEvent[i] < GlobalTimerStartPos) {
          NewState = InputStateMatrix[CurrentState][CurrentEvent[i]];
        } else if (CurrentEvent[i] < GlobalTimerEndPos) {
          CurrentColumn = CurrentEvent[i] - GlobalTimerStartPos;
          NewState = GlobalTimerStartMatrix[CurrentState][CurrentColumn];
        } else if (CurrentEvent[i] < GlobalCounterPos) {
          CurrentColumn = CurrentEvent[i] - GlobalTimerEndPos;
          NewState = GlobalTimerEndMatrix[CurrentState][CurrentColumn];
        } else if (CurrentEvent[i] < ConditionPos) {
          CurrentColumn = CurrentEvent[i] - GlobalCounterPos;
          NewState = GlobalCounterMatrix[CurrentState][CurrentColumn];
        } else if (CurrentEvent[i] < TupPos) {
          CurrentColumn = CurrentEvent[i] - ConditionPos;
          NewState = ConditionMatrix[CurrentState][CurrentColumn];
        } else if (CurrentEvent[i] == TupPos) {
          NewState = StateTimerMatrix[CurrentState];
        }
        if (NewState != CurrentState) {
          StateTransitionFound = 1;
        }
        i++;
      }

      // Store timestamps of events captured in this cycle
      #if LIVE_TIMESTAMPS == 0
        #if MACHINE_TYPE == 2
          for (int i = 0; i < nCurrentEvents; i++) {
            CurrentTimeStamps.Uint32[i] = CurrentTime;
            nEvents++;
          }
          digitalWriteDirect(fRAMcs, LOW);
          digitalWriteDirect(fRAMhold, HIGH); // Resume logging
          SPI.transfer(CurrentTimeStamps.Bytes, nCurrentEvents * 4);
          digitalWriteDirect(fRAMhold, LOW); // Pause logging
          digitalWriteDirect(fRAMcs, HIGH);
        #else
          if ((nEvents + nCurrentEvents) < MaxTimestamps) {
            for (int x = 0; x < nCurrentEvents; x++) {
              Timestamps[nEvents] = CurrentTime;
              nEvents++;
            } 
          }
        #endif
      #endif
      // Write events captured to USB (if events were captured)
      if (nCurrentEvents > 0) {
        CurrentEventBuffer[1] = nCurrentEvents;
        for (int i = 2; i < nCurrentEvents+2; i++) {
          CurrentEventBuffer[i] = CurrentEvent[i-2];
        }
        #if LIVE_TIMESTAMPS == 0
          PC.writeByteArray(CurrentEventBuffer, nCurrentEvents+2);
        #else  
          typeBuffer.uint32 = CurrentTime;
          CurrentEventBuffer[nCurrentEvents+2] = typeBuffer.byteArray[0];
          CurrentEventBuffer[nCurrentEvents+3] = typeBuffer.byteArray[1];
          CurrentEventBuffer[nCurrentEvents+4] = typeBuffer.byteArray[2];
          CurrentEventBuffer[nCurrentEvents+5] = typeBuffer.byteArray[3];
          PC.writeByteArray(CurrentEventBuffer, nCurrentEvents+6);
        #endif   
      }
      nCurrentEvents = 0;
      CurrentEvent[0] = 254; // 254 = no event
      // Make state transition if necessary
      if (NewState != CurrentState) {
        if (NewState == nStates) {
          RunningStateMatrix = false;
          MatrixFinished = true;
        } else {
          if (SyncMode == 1) {
            SyncWrite();
          }
          StateStartTime = CurrentTime;
          if (using255BackSignal && (NewState == 255)) {
            CurrentStateTEMP = CurrentState;
            CurrentState = previousState;
            previousState = CurrentStateTEMP;
          } else {
            previousState = CurrentState;
            CurrentState = NewState;
          }
          setStateOutputs(CurrentState);
        }
      }
      if (SyncMode == 2) {
        if (SyncTimer == 1000) {
          SyncWrite();
          SyncTimer = 0;
        }
      }
      updateFlexOutputs(); // If setStateOutputs or global timer linked channels set FlexOutput update flags, the latest values are written
    } // End code to run after first loop
  }  else { // If not running state matrix
    
   
  } // End if not running state matrix
  #if MACHINE_TYPE == 4 // Send back analog data
    if (flexIO_updateAIflag) { // Send analog values captured from FlexI/O channels
      flexIO_updateAIflag = false;  
      adcBuffer.uint16[0] = trialCounter;  
      adcBufferPos = 1;
      for (int i = 0; i < nFlexIO; i++) {
        if (flexIOChannelType[i] == 2) {
          adcBuffer.uint16[adcBufferPos] = FlexIO.adcReadout[i];
          adcBufferPos++;
        }
      }
      PC2.writeByteArray(adcBuffer.byteArray, adcBufferPos*2);  
    }     
  #endif
  if (MatrixFinished) {
    if (SyncMode == 0) {
      ResetSyncLine();
    } else {
      SyncWrite();
    }
    MatrixEndTimeMicros = sessionTimeMicros();
    resetOutputs();
// Send trial timing data back to computer
    serialByteBuffer[0] = 1; // Op Code for sending events
    serialByteBuffer[1] = 1; // Read one event
    serialByteBuffer[2] = 255; // Send Matrix-end code
    PC.writeByteArray(serialByteBuffer, 3);
    #if LIVE_TIMESTAMPS == 1
      timeBuffer.uint32[0] = CurrentTime;
      timeBuffer.uint32[1] = nCyclesCompleted;
      timeBuffer.uint64[1] = MatrixEndTimeMicros;
      PC.writeByteArray(timeBuffer.byteArray, 16);
      //PC.writeUint32(CurrentTime);
    #else
      PC.writeUint32(nCyclesCompleted);
      timeBuffer.uint64[0] = MatrixEndTimeMicros;
      PC.writeByteArray(timeBuffer.byteArray, 8);
    #endif
    
    #if LIVE_TIMESTAMPS == 0
      PC.writeUint16(nEvents);
      #if MACHINE_TYPE == 2
        // Return event times from fRAM IC
        digitalWriteDirect(fRAMhold, HIGH);
        digitalWriteDirect(fRAMcs, LOW);
        SPI.transfer(3); // Send read op code
        SPI.transfer(0); // Send address bytes
        SPI.transfer(0);
        SPI.transfer(0);
        uint16_t nFullBufferReads = 0; // A buffer array (SerialRelayBuffer) will store data read from RAM and dump it to USB
        uint16_t nRemainderBytes = 0;
        if (nEvents*4 > SerialRelayBufferSize) {
          nFullBufferReads = (unsigned long)(floor(((double)nEvents)*4 / (double)SerialRelayBufferSize));
        } else {
          nFullBufferReads = 0;
        }  
        for (int i = 0; i < nFullBufferReads; i++) { // Full buffer transfers; skipped if nFullBufferReads = 0
          SPI.transfer(SerialRelayBuffer, SerialRelayBufferSize);
          PC.writeByteArray(SerialRelayBuffer, SerialRelayBufferSize);
        }
        nRemainderBytes = (nEvents*4)-(nFullBufferReads*SerialRelayBufferSize);
        if (nRemainderBytes > 0) {
          SPI.transfer(SerialRelayBuffer, nRemainderBytes);
          PC.writeByteArray(SerialRelayBuffer, nRemainderBytes);   
        }
        digitalWriteDirect(fRAMcs, HIGH);
      #else
        PC.writeUint32Array(Timestamps, nEvents);
      #endif  
    #endif   
    MatrixFinished = false;
    if (smaReady2Load) { // If the next trial's state matrix was loaded to the serial buffer during the trial
      loadStateMatrix();
      smaReady2Load = false;
    }
    updateStatusLED(0);
    updateStatusLED(2);
    if (RunStateMatrixASAP) {
      RunStateMatrixASAP = false;
      if (newSMATransmissionStarted){
          if (smaTransmissionConfirmed) {
            startSM();
          } else {
            runFlag = true; // Set run flag to true; new SM will start as soon as its transmission completes
          }
      } else {
        startSM();
      }
    }
  } // End Matrix finished
  nCyclesCompleted++;
} // End timer handler

void ResetSyncLine() {
  if (!usesSPISync) {
    SyncState = 0;
    if (SyncChannelOriginalType == 'P') {
      pwmWrite(SyncChannelHW, SyncState);
    } else {
      digitalWriteDirect(SyncChannelHW, SyncState);     
    }
  }
}
void SyncWrite() {
  if (!usesSPISync) {
    if (syncOn) { ;
      if (SyncState == 0) {
        SyncState = 1;
      } else {
        SyncState = 0;
      }
      if (SyncChannelOriginalType == 'P') {
        pwmWrite(SyncChannelHW, SyncState*255);
      } else {
        digitalWriteDirect(SyncChannelHW, SyncState);     
      }
    }
  }
}
void SyncRegWrite(int value) {
  if (usesSPISync) {
    SPI.transfer(value);
    digitalWriteDirect(SyncRegisterLatch,HIGH);
    digitalWriteDirect(SyncRegisterLatch,LOW);
  }
}
void updateStatusLED(int Mode) {
  if (statusLEDEnabled) {
    LEDTime = millis();
    switch (Mode) {
      case 0: { // Clear
          analogWrite(RedLEDPin, 0);
          digitalWriteDirect(GreenLEDPin, 0);
          analogWrite(BlueLEDPin, 0);
        } break;
      case 1: { // Disconnected from PC software (Blue, fading)
          if (connectionState == 0) {
            if (LEDTime > NextLEDBrightnessAdjustTime) {
              NextLEDBrightnessAdjustTime = LEDTime + LEDBrightnessAdjustInterval;
              if (LEDBrightnessAdjustDirection == 1) {
                if (LEDBrightness < 255) {
                  LEDBrightness = LEDBrightness + 1;
                } else {
                  LEDBrightnessAdjustDirection = 0;
                }
              }
              if (LEDBrightnessAdjustDirection == 0) {
                if (LEDBrightness > 0) {
                  LEDBrightness = LEDBrightness - 1;
                } else {
                  LEDBrightnessAdjustDirection = 2;
                }
              }
              if (LEDBrightnessAdjustDirection == 2) {
                NextLEDBrightnessAdjustTime = LEDTime + 500;
                LEDBrightnessAdjustDirection = 1;
              }
              analogWrite(BlueLEDPin, LEDBrightness);
              #if MACHINE_TYPE == 4
                analogWrite(RedLEDPin, LEDBrightness/4.5);
              #endif
            }
          }
        } break;
      case 2: { // Connected, waiting for state machine description (Green)
          analogWrite(BlueLEDPin, 0);
          analogWrite(RedLEDPin, 0);
          digitalWriteDirect(GreenLEDPin, 1);
        } break;
      case 3: { // Running state machine (Orange)
          digitalWrite(GreenLEDPin, HIGH);
          analogWrite(BlueLEDPin, 0);
          analogWrite(RedLEDPin, 128);
        } break;
    }
  } else {
    analogWrite(RedLEDPin, 0);
    digitalWriteDirect(GreenLEDPin, 0);
    analogWrite(BlueLEDPin, 0);
  }
}

void setStateOutputs(byte State) { 
  uint16_t CurrentTimer = 0; // Used when referring to the timer currently being triggered
  byte CurrentCounter = 0; // Used when referring to the counter currently being reset
  byte thisChannel = 0;
  byte thisMessage = 0;
  byte nMessageBytes = 0;
  boolean reqValveUpdate = false;
  SyncRegWrite((State+1)); // If firmware 0.5 or 0.6, writes current state code to shift register
  
  // Cancel global timers
  CurrentTimer = smGlobalTimerCancel[State];
  if (CurrentTimer > 0) {
    for (int i = 0; i < nGlobalTimersUsed; i++) {
      if (bitRead(CurrentTimer, i)) {
        if (GlobalTimersActive[i]) {
          GlobalTimersTriggered[i] = false;
          GlobalTimersActive[i] = false;
          setGlobalTimerChannel(i, 0);
          CurrentEvent[nCurrentEvents] = i+GlobalTimerEndPos; nCurrentEvents++;
        } else if (GlobalTimersTriggered[i]) {
          GlobalTimersTriggered[i] = false;
        }
      }
    }
  }
  
  // Trigger global timers
  CurrentTimer = smGlobalTimerTrig[State];  
  if (CurrentTimer > 0) {
    for (int i = 0; i < nGlobalTimersUsed; i++) {
      if (bitRead(CurrentTimer, i)) {
        triggerGlobalTimer(i);
      }
    }
  }
  
  // Update output channels
  for (int i = 0; i < nOutputs; i++) {
      switch(OutputHW[i]) {
        case 'U':
          thisMessage = OutputStateMatrix[State][i];
          if (thisMessage > 0) {
            nMessageBytes = SerialMessage_nBytes[thisMessage][thisChannel];
            for (int i = 0; i < nMessageBytes; i++) {
               serialByteBuffer[i] = SerialMessageMatrix[thisMessage][thisChannel][i];
            }
            switch(thisChannel) {
              case 0:
                Module1.writeByteArray(serialByteBuffer, nMessageBytes);
              break;
              case 1:
                Module2.writeByteArray(serialByteBuffer, nMessageBytes);
              break;
              #if MACHINE_TYPE > 1
                case 2:
                  Module3.writeByteArray(serialByteBuffer, nMessageBytes);
                break;
              #endif
              #if MACHINE_TYPE == 3
                case 3:
                  Module4.writeByteArray(serialByteBuffer, nMessageBytes);
                break;
                #if ETHERNET_COM == 0
                  case 4:
                    Module5.writeByteArray(serialByteBuffer, nMessageBytes);
                  break;
                #endif
              #endif
            }
          }
          thisChannel++;
        break;
        case 'X':
          if (OutputStateMatrix[State][i] > 0) {
              serialByteBuffer[0] = 2; // Code for MATLAB to receive soft-code byte
              serialByteBuffer[1] = OutputStateMatrix[State][i]; // Soft code byte
              PC.writeByteArray(serialByteBuffer, 2);
          }
          thisChannel++;
        break;
        case 'Z':
          #if MACHINE_TYPE > 2
            if (OutputStateMatrix[State][i] > 0) {
              PC1.writeByte(OutputStateMatrix[State][i]);
            }
          #endif
          thisChannel++;
        break;
        case 'D':
        case 'B':
        case 'W':
          if (outputOverrideState[i] == 0) {
            digitalWriteDirect(OutputCh[i], OutputStateMatrix[State][i]); 
          }
        case 'V':
          if (outputOverrideState[i] == 0) {
            if (usesSPIValves) {
              outputState[i] = OutputStateMatrix[State][i];
              reqValveUpdate = true;
            } else {
              digitalWriteDirect(OutputCh[i], OutputStateMatrix[State][i]); 
            }
          }
        break;
        case 'P':
          if (outputOverrideState[i] == 0) {   
             pwmWrite(OutputCh[i], OutputStateMatrix[State][i]);
          }
        break;
        #if MACHINE_TYPE == 4
          case 'F':
            switch (flexIOChannelType[OutputCh[i]]) {
              case 1: // Digital output
                FlexIO.setDO(OutputCh[i], OutputStateMatrix[State][i]);
                flexIO_updateDOflag = true;
              break;
              case 3: // Analog output
                // Set values
                flexIOValues[OutputCh[i]] = OutputStateMatrix[State][i];
                flexIO_updateAOflag = true;
              break;
            }
          break;
        #endif
     }
  }

  // Update valves
  if (reqValveUpdate) {
    valveWrite();
    reqValveUpdate = false;
  }
  // Reset event counters
  CurrentCounter = smGlobalCounterReset[State];
  if (CurrentCounter > 0) {
    CurrentCounter = CurrentCounter - 1; // Convert to 0 index
    GlobalCounterCounts[CurrentCounter] = 0;
    GlobalCounterHandled[CurrentCounter] = false;
  }
  #if MACHINE_TYPE == 4
    thisAnalogEnable = analogThreshEnable[State];
    if (thisAnalogEnable > 0) {
      for (int i = 0; i < nFlexIO; i++) {
        if (bitRead(thisAnalogEnable, i)) {
            analogThreshold1Enabled[i] = 1;
            analogThreshold2Enabled[i] = 1;
        }
      }
    }
    thisAnalogEnable = analogThreshDisable[State];
    if (thisAnalogEnable > 0) {
      for (int i = 0; i < nFlexIO; i++) {
        if (bitRead(thisAnalogEnable, i)) {
            analogThreshold1Enabled[i] = 0;
            analogThreshold2Enabled[i] = 0;
        }
      }
    }
  #endif
  // Enable state timer only if handled
  if (StateTimerMatrix[State] != State) {
    MeaningfulStateTimer = true;
  } else {
    MeaningfulStateTimer = false;
  }
}

void triggerGlobalTimer(byte timerID) {
  GlobalTimersTriggered[timerID] = true;
  GlobalTimerStart[timerID] = CurrentTime;
  if (GlobalTimerOnsetDelays[timerID] > 0){
    GlobalTimerStart[timerID] += GlobalTimerOnsetDelays[timerID];
  } else {
    if (!GlobalTimersActive[timerID]) {
      GlobalTimersActive[timerID] = true;
      setGlobalTimerChannel(timerID, 1);
      if (GlobalTimerOnsetTriggers[timerID] > 0) {
        for (int j = 0; j < nGlobalTimersUsed; j++) {
          if (bitRead(GlobalTimerOnsetTriggers[timerID], j)) {
            if (j != timerID) {
              triggerGlobalTimer(j);
            }
          }
        }
      }
      CurrentEvent[nCurrentEvents] = timerID+GlobalTimerStartPos; nCurrentEvents++;
    }
  }
  GlobalTimerEnd[timerID] = GlobalTimerStart[timerID] + GlobalTimers[timerID];
  if (GTUsingLoopCounter[timerID]) {
    GlobalTimerLoopCount[timerID] = 1;
  } 
}

void resetOutputs() {
  for (int i = 0; i < nOutputs; i++) {
    switch (OutputHW[i]) {
      case 'B':
      case 'W':
      case 'V':
        digitalWriteDirect(OutputCh[i], 0);
        outputOverrideState[i] = 0; 
        outputState[i] = 0;
      break;
      case 'P':
        pwmWrite(OutputCh[i], 0); 
        outputOverrideState[i] = 0; 
      break;
    }
  }
  #if MACHINE_TYPE == 4
    for (int i = 0; i < nFlexIO; i++) {
      switch(flexIOChannelType[i]) {
        case 1:
          FlexIO.setDO(i,0);
        break;
        case 3:
          FlexIO.writeDAC(i, 0);
        break;
      }
    }
    FlexIO.writeDO();
  #endif
  valveWrite();
  for (int i = 0; i < MAX_GLOBAL_TIMERS; i++) {
    GlobalTimersTriggered[i] = false;
    GlobalTimersActive[i] = false;
    GlobalTimerLoopCount[i] = 0;
  }
  for (int i = 0; i < MAX_GLOBAL_COUNTERS; i++) {  
    GlobalCounterCounts[i] = 0;
    GlobalCounterHandled[i] = false;
  }
  MeaningfulStateTimer = false;
}

uint64_t sessionTimeMicros() {
   uint64_t sessionTime = 0;
   #if MACHINE_TYPE > 2
    currentTimeMicros = teensyMicros;
   #else
    currentTimeMicros = micros();
   #endif
   sessionTime = ((uint64_t)currentTimeMicros + ((uint64_t)nMicrosRollovers*4294967295)) - sessionStartTimeMicros;
   return sessionTime;
}

void resetSessionClock() {
    #if MACHINE_TYPE > 2
      teensyMicros = 0;
      lastTimeMicros = teensyMicros;
      sessionStartTimeMicros = teensyMicros;
    #else
      sessionStartTimeMicros = micros();
    #endif
    nMicrosRollovers = 0;
    #if MACHINE_TYPE == 4
      firstTrialFlag = true;
    #endif
}

void resetSerialMessages() {
  for (int i = 0; i < MaxStates; i++) {
    for (int j = 0; j < nSerialChannels-1; j++) {
      SerialMessageMatrix[i][j][0] = i;
      SerialMessage_nBytes[i][j] = 1;
    }
  }
}

void valveWrite() {
  byte value = 0;
  for (int i = ValvePos; i < nOutputs; i++) {
    if (outputState[i] == 1) {
      bitSet(value, i-ValvePos);
    }
  }
  SPI.transfer(value);
  digitalWriteDirect(valveCSChannel, HIGH);
  digitalWriteDirect(valveCSChannel, LOW);
}

void updateFlexOutputs() {
  #if MACHINE_TYPE == 4
    if (flexIO_updateDOflag) {
      flexIO_updateDOflag = false;
      FlexIO.writeDO();
    }
    if (flexIO_updateAOflag) {
      for (int i = 0; i < nFlexIO; i++) {
        if (flexIOChannelType[i] == 3) {
          FlexIO.writeDAC(i, flexIOValues[i]);
        }
      }
      flexIO_updateAOflag = false;
    }
  #endif
}

void digitalWriteDirect(int pin, boolean val) { // >10x Faster than digitalWrite(), specific to Arduino Due
  #if MACHINE_TYPE < 3
    if (val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
    else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
  #else
    digitalWriteFast(pin, val);
  #endif
}

byte digitalReadDirect(int pin) { // >10x Faster than digitalRead(), specific to Arduino Due
  #if MACHINE_TYPE < 3
    return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
  #else
    return digitalReadFast(pin);
  #endif
}

#if MACHINE_TYPE == 4
  // This PSRAM memory test was adopted from PJRC's teensy41_psram_memtest repository : https://github.com/PaulStoffregen/teensy41_psram_memtest
  // Thanks Paul!!
  bool check_fixed_pattern(uint32_t pattern)
  {
    volatile uint32_t *p;
    for (p = memory_begin; p < memory_end; p++) {
      *p = pattern;
    }
    arm_dcache_flush_delete((void *)memory_begin,
      (uint32_t)memory_end - (uint32_t)memory_begin);
    for (p = memory_begin; p < memory_end; p++) {
      uint32_t actual = *p;
      if (actual != pattern) return false;
    }
    return true;
  }
#endif

void pwmWrite(byte channel, byte value) {
  #if MACHINE_TYPE < 3
    analogWrite(channel, value); 
  #else
    if (value == 255) {
      analogWrite(channel, 256); // On Teensy boards, 256 = always high
    } else {
      analogWrite(channel, value); 
    }
  #endif
}
            
void setGlobalTimerChannel(byte timerChan, byte op) {
  byte thisChannel = 0; 
  byte thisMessage = 0;
  byte nMessageBytes = 0;
  thisChannel = GlobalTimerChannel[timerChan];
  switch (OutputHW[thisChannel]) {
    case 'U': // UART
      if (op == 1) {
        thisMessage = GlobalTimerOnMessage[timerChan];
      } else {
        thisMessage = GlobalTimerOffMessage[timerChan];
      }
      if (thisMessage < 254) {
        nMessageBytes = SerialMessage_nBytes[thisMessage][thisChannel];
          for (int i = 0; i < nMessageBytes; i++) {
             serialByteBuffer[i] = SerialMessageMatrix[thisMessage][thisChannel][i];
          }
        switch (thisChannel) {
          case 0:
            Module1.writeByteArray(serialByteBuffer, nMessageBytes);
          break;
          case 1:
            Module2.writeByteArray(serialByteBuffer, nMessageBytes);
          break;
          #if MACHINE_TYPE > 1
            case 2:
              Module3.writeByteArray(serialByteBuffer, nMessageBytes);
            break;
          #endif
          #if MACHINE_TYPE == 3
            case 3:
              Module4.writeByteArray(serialByteBuffer, nMessageBytes);
            break;
            #if ETHERNET_COM == 0
              case 4:
                Module5.writeByteArray(serialByteBuffer, nMessageBytes);
              break;
            #endif
          #endif
        }
      }
    break;
    case 'X':
      if (op == 1) {
        thisMessage = GlobalTimerOnMessage[timerChan];
      } else {
        thisMessage = GlobalTimerOffMessage[timerChan];
      }
      serialByteBuffer[0] = 2; // Code for MATLAB to receive soft-code byte
      serialByteBuffer[1] = thisMessage;
      PC.writeByteArray(serialByteBuffer, 2);
    break;
    case 'Z':
      #if MACHINE_TYPE > 2
        if (op == 1) {
          thisMessage = GlobalTimerOnMessage[timerChan];
        } else {
          thisMessage = GlobalTimerOffMessage[timerChan];
        }
        PC1.writeByte(thisMessage);
      #endif
    break;
    case 'B': // Digital IO (BNC, Wire, Digital, Valve-line)
    case 'W':
    case 'D':
      if (op == 1) {
        digitalWriteDirect(OutputCh[thisChannel], HIGH);
        outputOverrideState[thisChannel] = 1;
      } else {
        digitalWriteDirect(OutputCh[thisChannel], LOW);
        outputOverrideState[thisChannel] = 0;
      }
    break;
    case 'V':
      if (usesSPIValves) {
        outputState[thisChannel] = op;
        valveWrite();
      } else {
        if (op == 1) {
          digitalWriteDirect(OutputCh[thisChannel], HIGH);
        } else {
          digitalWriteDirect(OutputCh[thisChannel], LOW);
        }
      }
      outputOverrideState[thisChannel] = op;
    break;
    case 'P': // Port (PWM / LED)
      if (op == 1) {
        pwmWrite(OutputCh[thisChannel], GlobalTimerOnMessage[timerChan]);
        outputOverrideState[thisChannel] = 1;
      } else {
        analogWrite(OutputCh[thisChannel], 0);
        outputOverrideState[thisChannel] = 0;
      }
    break;
    #if MACHINE_TYPE == 4
      case 'F': // Flex I/O
        switch (flexIOChannelType[OutputCh[thisChannel]]) {
            case 1: // Digital output
              if (op == 0) {
                FlexIO.setDO(OutputCh[thisChannel], 0);
              } else if (op == 1) {
                FlexIO.setDO(OutputCh[thisChannel], 1);
              }
              flexIO_updateDOflag = true;
            break;
            case 3: // Analog output
              // Set values
              if (op == 0) {
                flexIOValues[OutputCh[thisChannel]] = GlobalTimerOffMessage[timerChan];
              } else if (op == 1) {
                flexIOValues[OutputCh[thisChannel]] = GlobalTimerOnMessage[timerChan];
              }
              flexIO_updateAOflag = true;
            break;
          }
          outputOverrideState[thisChannel] = op;
      break;
    #endif
  }
  inputState[nInputs+timerChan] = op;
}
            
void relayModuleInfo(ArCOM serialCOM, byte moduleID) {
  boolean moduleFound = false;
  boolean setBaudRate = false;
  uint32_t newBaudRate = 0;
  if (serialCOM.available() > 0) {
    Byte1 = serialCOM.readByte();
    if (Byte1 == 'A') { // A = Acknowledge; this is most likely a module
      if (serialCOM.available() > 3) {
        moduleFound = true;
        PC.writeByte(1); // Module detected
        for (int i = 0; i < 4; i++) { // Send firmware version
          PC.writeByte(serialCOM.readByte());
        }
        nBytes = serialCOM.readByte(); // Length of module name
        PC.writeByte(nBytes);
        for (int i = 0; i < nBytes; i++) { // Transfer module name
          PC.writeByte(serialCOM.readByte());
        }
        Byte1 = serialCOM.readByte(); // 1 if more module info follows, 0 if not
        if (Byte1 == 1) { // Optional: module can return additional info with an op code scheme
          while(Byte1 == 1) {
            PC.writeByte(1); // indicate that more info is coming for this module
            CommandByte = serialCOM.readByte();
            switch (CommandByte) {
              case '#': // Number of behavior events to reserve for the module
                PC.writeByte('#');
                PC.writeByte(serialCOM.readByte());
              break;
              case 'E': // Event name strings (replace default event names: ModuleName1_1, etc)
                PC.writeByte('E');
                Byte2 = serialCOM.readByte(); // nEvent names to transmit
                PC.writeByte(Byte2);
                for (int i = 0; i < Byte2; i++) {
                  Byte3 = serialCOM.readByte(); // Length of event name (in characters)
                  PC.writeByte(Byte3);
                  serialCOM.readByteArray(SerialRelayBuffer, Byte3);
                  PC.writeByteArray(SerialRelayBuffer, Byte3);
                }
              break;
              case 'V': // Major hardware version number
                PC.writeByte('V');
                PC.writeByte(serialCOM.readByte());
              break;
              case 'v': // Minor hardware version number (
                PC.writeByte('v');
                PC.writeByte(serialCOM.readByte());
              break;
            }
            Byte1 = serialCOM.readByte(); // 1 if more module info follows, 0 if not
          }
          PC.writeByte(0); // indicate that no more info is coming for this module 
        } else {
          PC.writeByte(0); // indicate that no more info is coming for this module 
        }
      }
    }
  }
  if (!moduleFound) {
    PC.writeByte(0); // Module not detected
  }
  while (serialCOM.available() > 0) { // Clear any redundant self descriptions. These may be caused by older modules responding to the second '255' request for self-description
    serialCOM.readByte();
  }
}

void disableModuleRelays() {
  for (int i = 0; i < nSerialChannels; i++) { // Shut off all other channels
    UARTrelayMode[Byte1] = false;
  }
}

void relayModuleBytes() {
  for (int i = 0; i < nSerialChannels; i++) { // If relay mode is on, return any incoming module bytes to MATLAB/Python
      if (UARTrelayMode[i]) {
        switch(i) {
          case 0:
            Byte3 = Module1.available();
            if (Byte3>0) { 
              for (int j = 0; j < Byte3; j++) {
                 PC.writeByte(Module1.readByte());       
              }
            }
          break;
          case 1:
            Byte3 = Module2.available();
            if (Byte3>0) { 
              for (int j = 0; j < Byte3; j++) {
                 PC.writeByte(Module2.readByte());       
              }
            }
          break;
          #if MACHINE_TYPE > 1
            case 2:
              Byte3 = Module3.available();
              if (Byte3>0) { 
                for (int j = 0; j < Byte3; j++) {
                   PC.writeByte(Module3.readByte());       
                }
              }
            break;
          #endif
          #if MACHINE_TYPE == 3
            case 3:
                Byte3 = Module4.available();
                if (Byte3>0) { 
                  for (int j = 0; j < Byte3; j++) {
                     PC.writeByte(Module4.readByte());       
                  }
                }
            break;
            #if ETHERNET_COM == 0
              case 4:
                Byte3 = Module5.available();
                if (Byte3>0) { 
                  for (int j = 0; j < Byte3; j++) {
                     PC.writeByte(Module5.readByte());       
                  }
                }
              break;
            #endif
          #endif
        }
      }
    }
}

void clearSerialBuffers() {
  Byte1 = 0;
  for (int i = 0; i < BNCInputPos; i++) {
      switch (InputHW[i]) {
        case 'U': 
            switch(Byte1) {
              case 0:
                while (Module1.available() > 0) {
                  Module1.readByte(); Byte1++;
                }
              break;
              case 1:
                while (Module2.available() > 0) {
                  Module2.readByte(); Byte1++;
                }
              break;
              #if MACHINE_TYPE > 1
                case 2:
                  while (Module3.available() > 0) {
                    Module3.readByte(); Byte1++;
                  }
                break;
              #endif
              #if MACHINE_TYPE == 3
                case 3:
                  while (Module4.available() > 0) {
                    Module4.readByte(); Byte1++;
                  }
                break;
                #if ETHERNET_COM == 0
                  case 4:
                    while (Module5.available() > 0) {
                      Module5.readByte(); Byte1++;
                    }
                  break;
                #endif
              #endif
            }
       break;
     }
  }
}

void loadStateMatrix() { // Loads a state matrix from the serial buffer into the relevant local variables
  #if MACHINE_TYPE > 1
    nStates = StateMatrixBuffer[0];
    nGlobalTimersUsed = StateMatrixBuffer[1];
    nGlobalCountersUsed = StateMatrixBuffer[2];
    nConditionsUsed = StateMatrixBuffer[3];
  #else
    nStates = PC.readByte();
    nGlobalTimersUsed = PC.readByte();
    nGlobalCountersUsed = PC.readByte();
    nConditionsUsed = PC.readByte();
  #endif
  bufferPos = 4; // Current position in serial relay buffer
  memset(smGlobalTimerTrig, 0, sizeof(smGlobalTimerTrig[0]) * nStates);
  memset(smGlobalTimerCancel, 0, sizeof(smGlobalTimerCancel[0]) * nStates);
  memset(smGlobalCounterReset, 0, sizeof(smGlobalCounterReset[0]) * nStates);
  memset(StateTimerMatrix, 0, sizeof(StateTimerMatrix[0]) * nStates);
  memset(OutputStateMatrix, 0, sizeof(OutputStateMatrix[0][0]) * OutputMatrixSize * nStates);
  for (byte x = 0; x < nStates; x++) { // Reset matrix to default
    memset(&InputStateMatrix[x], x, InputMatrixSize*sizeof(InputStateMatrix[0][0]));
//    For some reason loops are faster than memset if nBytes to set is smaller than some threshold
    for (int y = 0; y < MAX_GLOBAL_TIMERS; y++) {
      GlobalTimerStartMatrix[x][y] = x;
    }
    for (int y = 0; y < MAX_GLOBAL_TIMERS; y++) {
      GlobalTimerEndMatrix[x][y] = x;
    }
    for (int y = 0; y < MAX_GLOBAL_COUNTERS; y++) {
      GlobalCounterMatrix[x][y] = x;
    }
    for (int y = 0; y < MAX_CONDITIONS; y++) {
      ConditionMatrix[x][y] = x;
    }
  }
  #if MACHINE_TYPE > 1 // Bpod 0.7+; Read state matrix from RAM buffer
    memcpy(StateTimerMatrix, &StateMatrixBuffer[bufferPos], nStates*sizeof(StateTimerMatrix[0]));
    bufferPos += sizeof(StateTimerMatrix[0])*nStates;
    for (int x = 0; x < nStates; x++) { // Get Input Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      for (int y = 0; y<nOverrides; y++) {
        col = StateMatrixBuffer[bufferPos]; bufferPos++;
        val = StateMatrixBuffer[bufferPos]; bufferPos++;
        InputStateMatrix[x][col] = val;
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Output Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      #if MACHINE_TYPE == 4 // Output matrix is 16-bit for state machine 2+
        typeBuffer.byteArray[0] = (uint8_t)nOverrides;
        typeBuffer.byteArray[1] = StateMatrixBuffer[bufferPos]; bufferPos++;
        nOverrides = (uint8_t)typeBuffer.uint16;
      #endif
      for (int y = 0; y<nOverrides; y++) {
        col = StateMatrixBuffer[bufferPos]; bufferPos++;
        #if MACHINE_TYPE == 4 // Output matrix is 16-bit for state machine 2+
          typeBuffer.byteArray[0] = (uint8_t)col;
          typeBuffer.byteArray[1] = StateMatrixBuffer[bufferPos]; bufferPos++;
          col = typeBuffer.uint16;
        #endif  
        val = StateMatrixBuffer[bufferPos]; bufferPos++;
        #if MACHINE_TYPE == 4 // Output matrix is 16-bit for state machine 2+
          typeBuffer.byteArray[0] = (uint8_t)val;
          typeBuffer.byteArray[1] = StateMatrixBuffer[bufferPos]; bufferPos++;
          val = typeBuffer.uint16;
        #endif
        OutputStateMatrix[x][col] = val;
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Timer Start Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = StateMatrixBuffer[bufferPos]; bufferPos++;
          val = StateMatrixBuffer[bufferPos]; bufferPos++;
          GlobalTimerStartMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Timer End Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = StateMatrixBuffer[bufferPos]; bufferPos++;
          val = StateMatrixBuffer[bufferPos]; bufferPos++;
          GlobalTimerEndMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Counter Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = StateMatrixBuffer[bufferPos]; bufferPos++;
          val = StateMatrixBuffer[bufferPos]; bufferPos++;
          GlobalCounterMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Condition Matrix differences
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = StateMatrixBuffer[bufferPos]; bufferPos++;
          val = StateMatrixBuffer[bufferPos]; bufferPos++;
          ConditionMatrix[x][col] = val;
        }
      }
    }

    if (nGlobalTimersUsed > 0) {
      memcpy(GlobalTimerChannel, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerChannel[0]));
      bufferPos += sizeof(GlobalTimerChannel[0])*nGlobalTimersUsed;
      memcpy(GlobalTimerOnMessage, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerOnMessage[0]));
      bufferPos += sizeof(GlobalTimerOnMessage[0])*nGlobalTimersUsed;
      memcpy(GlobalTimerOffMessage, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerOffMessage[0]));
      bufferPos += sizeof(GlobalTimerOffMessage[0])*nGlobalTimersUsed;
      for (int i = 0; i < nGlobalTimersUsed; i++) {
        GlobalTimerLoop[i] = StateMatrixBuffer[bufferPos]; bufferPos++;
        if (GlobalTimerLoop[i] > 1) {
          GTUsingLoopCounter[i] = true;
        } else {
          GTUsingLoopCounter[i] = false;
        }
      }
      memcpy(SendGlobalTimerEvents, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(SendGlobalTimerEvents[0]));
      bufferPos += sizeof(SendGlobalTimerEvents[0])*nGlobalTimersUsed;
    }
    if (nGlobalCountersUsed > 0) {
      memcpy(GlobalCounterAttachedEvents, &StateMatrixBuffer[bufferPos], nGlobalCountersUsed*sizeof(GlobalCounterAttachedEvents[0]));
      bufferPos += sizeof(GlobalCounterAttachedEvents[0])*nGlobalCountersUsed;
    }
    if (nConditionsUsed > 0) {
      memcpy(ConditionChannels, &StateMatrixBuffer[bufferPos], nConditionsUsed*sizeof(ConditionChannels[0]));
      bufferPos += sizeof(ConditionChannels[0])*nConditionsUsed;
      memcpy(ConditionValues, &StateMatrixBuffer[bufferPos], nConditionsUsed*sizeof(ConditionValues[0]));
      bufferPos += sizeof(ConditionValues[0])*nConditionsUsed;
    }
    nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
    for (int i = 0; i < nOverrides; i++) {
      col = StateMatrixBuffer[bufferPos]; bufferPos++;
      val = StateMatrixBuffer[bufferPos]; bufferPos++;
      smGlobalCounterReset[col] = val;
    }
    #if MACHINE_TYPE == 4
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      for (int i = 0; i < nOverrides; i++) {
        col = StateMatrixBuffer[bufferPos]; bufferPos++;
        val = StateMatrixBuffer[bufferPos]; bufferPos++;
        analogThreshEnable[col] = val;
      }
      nOverrides = StateMatrixBuffer[bufferPos]; bufferPos++;
      for (int i = 0; i < nOverrides; i++) {
        col = StateMatrixBuffer[bufferPos]; bufferPos++;
        val = StateMatrixBuffer[bufferPos]; bufferPos++;
        analogThreshDisable[col] = val;
      }
    #endif
    memcpy(smGlobalTimerTrig, &StateMatrixBuffer[bufferPos], nStates*sizeof(smGlobalTimerTrig[0]));
    bufferPos += sizeof(smGlobalTimerTrig[0])*nStates;
    memcpy(smGlobalTimerCancel, &StateMatrixBuffer[bufferPos], nStates*sizeof(smGlobalTimerCancel[0]));
    bufferPos += sizeof(smGlobalTimerCancel[0])*nStates;
    memcpy(GlobalTimerOnsetTriggers, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerOnsetTriggers[0]));
    bufferPos += sizeof(GlobalTimerOnsetTriggers[0])*nGlobalTimersUsed;
    memcpy(StateTimers, &StateMatrixBuffer[bufferPos], nStates*sizeof(StateTimers[0]));
    bufferPos += 4*nStates;
    if (nGlobalTimersUsed > 0) {
      memcpy(GlobalTimers, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimers[0]));
      bufferPos += 4*nGlobalTimersUsed;
      memcpy(GlobalTimerOnsetDelays, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerOnsetDelays[0]));
      bufferPos += 4*nGlobalTimersUsed;
      memcpy(GlobalTimerLoopIntervals, &StateMatrixBuffer[bufferPos], nGlobalTimersUsed*sizeof(GlobalTimerLoopIntervals[0]));
      bufferPos += 4*nGlobalTimersUsed;
    }
    if (nGlobalCountersUsed > 0) {
      memcpy(GlobalCounterThresholds, &StateMatrixBuffer[bufferPos], nGlobalCountersUsed*sizeof(GlobalCounterThresholds[0]));
      bufferPos += 4*nGlobalCountersUsed;
    }
    byte containsAdditionalOps = StateMatrixBuffer[bufferPos]; bufferPos++; // New for firmware r23 - certain additional ops can be packaged with the state machine description
    boolean clearedSerialMessageLibrary = false;
    while (containsAdditionalOps) {
      byte thisOp = StateMatrixBuffer[bufferPos]; bufferPos++;
      switch (thisOp) {
        case 'L': // Load serial message library for one module
          if (!clearedSerialMessageLibrary) {
            resetSerialMessages();
            clearedSerialMessageLibrary = true;
          }
          Byte1 = StateMatrixBuffer[bufferPos]; bufferPos++;
          Byte2 = StateMatrixBuffer[bufferPos]; bufferPos++;
          for (int i = 0; i < Byte2; i++) {
            Byte3 = StateMatrixBuffer[bufferPos]; bufferPos++; // Message Index
            Byte4 = StateMatrixBuffer[bufferPos]; bufferPos++; // Message Length
            SerialMessage_nBytes[Byte3][Byte1] = Byte4;
            for (int j = 0; j < Byte4; j++) {
              SerialMessageMatrix[Byte3][Byte1][j] = StateMatrixBuffer[bufferPos]; bufferPos++;
            }
          }
        break;
      }
      containsAdditionalOps = StateMatrixBuffer[bufferPos]; bufferPos++;
    }
  #else // Bpod 0.5; Read state matrix from serial port
    for (int x = 0; x < nStates; x++) { // Get State timer matrix
      StateTimerMatrix[x] = PC.readByte();
    }
    for (int x = 0; x < nStates; x++) { // Get Input Matrix differences
      nOverrides = PC.readByte();
      for (int y = 0; y<nOverrides; y++) {
        col = PC.readByte();
        val = PC.readByte();
        InputStateMatrix[x][col] = val;
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Output Matrix differences
      nOverrides = PC.readByte();
      for (int y = 0; y<nOverrides; y++) {
        col = PC.readByte();
        val = PC.readByte();
        OutputStateMatrix[x][col] = val;
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Timer Start Matrix differences
      nOverrides = PC.readByte();
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = PC.readByte();
          val = PC.readByte();
          GlobalTimerStartMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Timer End Matrix differences
      nOverrides = PC.readByte();
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = PC.readByte();
          val = PC.readByte();
          GlobalTimerEndMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Global Counter Matrix differences
      nOverrides = PC.readByte();
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = PC.readByte();
          val = PC.readByte();
          GlobalCounterMatrix[x][col] = val;
        }
      }
    }
    for (int x = 0; x < nStates; x++) { // Get Condition Matrix differences
      nOverrides = PC.readByte();
      if (nOverrides > 0) {
        for (int y = 0; y<nOverrides; y++) {
          col = PC.readByte();
          val = PC.readByte();
          ConditionMatrix[x][col] = val;
        }
      }
    }
    if (nGlobalTimersUsed > 0) {
      PC.readByteArray(GlobalTimerChannel, nGlobalTimersUsed); // Get output channels of global timers
      PC.readByteArray(GlobalTimerOnMessage, nGlobalTimersUsed); // Get serial messages to trigger on timer start
      PC.readByteArray(GlobalTimerOffMessage, nGlobalTimersUsed); // Get serial messages to trigger on timer end
      PC.readByteArray(GlobalTimerLoop, nGlobalTimersUsed); // Get global timer loop state (true/false)
      for (int i = 0; i < nGlobalTimersUsed; i++) {
        if (GlobalTimerLoop[i] > 1) {
          GTUsingLoopCounter[i] = true;
        } else {
          GTUsingLoopCounter[i] = false;
        }
      }
      PC.readByteArray(SendGlobalTimerEvents, nGlobalTimersUsed); // Send global timer events (enabled/disabled)
    }
    if (nGlobalCountersUsed > 0) {
      PC.readByteArray(GlobalCounterAttachedEvents, nGlobalCountersUsed); // Get global counter attached events
    }
    if (nConditionsUsed > 0) {
      PC.readByteArray(ConditionChannels, nConditionsUsed); // Get condition channels
      PC.readByteArray(ConditionValues, nConditionsUsed); // Get condition values
    }
    nOverrides = PC.readByte();
    for (int y = 0; y<nOverrides; y++) {
      col = PC.readByte();
      val = PC.readByte();
      smGlobalCounterReset[col] = val;
    }     
    #if GLOBALTIMER_TRIG_BYTEWIDTH == 1
        PC.readByteArray(smGlobalTimerTrig, nStates);
        PC.readByteArray(smGlobalTimerCancel, nStates);
        PC.readByteArray(GlobalTimerOnsetTriggers, nGlobalTimersUsed);
    #elif GLOBALTIMER_TRIG_BYTEWIDTH == 2
        PC.readUint16Array(smGlobalTimerTrig, nStates);
        PC.readUint16Array(smGlobalTimerCancel, nStates);
        PC.readUint16Array(GlobalTimerOnsetTriggers, nGlobalTimersUsed);
    #elif GLOBALTIMER_TRIG_BYTEWIDTH == 4
        PC.readUint32Array(smGlobalTimerTrig, nStates);
        PC.readUint32Array(smGlobalTimerCancel, nStates);
        PC.readUint32Array(GlobalTimerOnsetTriggers, nGlobalTimersUsed);
    #endif
    
    PC.readUint32Array(StateTimers, nStates); // Get state timers
    if (nGlobalTimersUsed > 0) {
      PC.readUint32Array(GlobalTimers, nGlobalTimersUsed); // Get global timers
      PC.readUint32Array(GlobalTimerOnsetDelays, nGlobalTimersUsed); // Get global timer onset delays
      PC.readUint32Array(GlobalTimerLoopIntervals, nGlobalTimersUsed); // Get loop intervals
    }
    if (nGlobalCountersUsed > 0) {
      PC.readUint32Array(GlobalCounterThresholds, nGlobalCountersUsed); // Get global counter event count thresholds
    }
    byte containsAdditionalOps = PC.readByte();
    boolean clearedSerialMessageLibrary = false;
    while (containsAdditionalOps) {
      byte thisOp = PC.readByte();
      switch (thisOp) {
        case 'L': // Load serial message library for one module
          if (!clearedSerialMessageLibrary) {
            resetSerialMessages();
            clearedSerialMessageLibrary = true;
          }
          Byte1 = PC.readByte();
          Byte2 = PC.readByte();
          for (int i = 0; i < Byte2; i++) {
            Byte3 = PC.readByte(); // Message Index
            Byte4 = PC.readByte(); // Message Length
            SerialMessage_nBytes[Byte3][Byte1] = Byte4;
            for (int j = 0; j < Byte4; j++) {
              SerialMessageMatrix[Byte3][Byte1][j] = PC.readByte();
            }
          }
        break;
      }
      containsAdditionalOps = PC.readByte();
    }
    smaTransmissionConfirmed = true;
  #endif
}

void setFlexIOChannelTypes() {
  #if MACHINE_TYPE == 4
    for (int i = 0; i<nFlexIO; i++) {
      FlexIO.setChannelType(i, flexIOChannelType[i]);
    }
    FlexIO.updateChannelTypes();
  #endif
}

void startSM() {  
  if (newSMATransmissionStarted){
      if (smaTransmissionConfirmed) {
        PC.writeByte(1);
      } else {
        PC.writeByte(0);
      }
      #if MACHINE_TYPE > 2
        Serial.send_now();
      #endif
      newSMATransmissionStarted = false;
  }
  updateStatusLED(3);
  NewState = 0;
  previousState = 0;
  CurrentState = 0;
  nEvents = 0;
  SoftEvent1 = 255; // No event
  SoftEvent2 = 255;
  MatrixFinished = false;
  #if LIVE_TIMESTAMPS == 0
    if (usesFRAM) {
      // Initialize fRAM
      SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
      digitalWriteDirect(fRAMcs, LOW);
      SPI.transfer(6); // Send Write enable code
      digitalWriteDirect(fRAMcs, HIGH); // Must go high afterwards to enable writes
      delayMicroseconds(10);
      digitalWriteDirect(fRAMcs, LOW);
      SPI.transfer(2); // Send write op code
      SPI.transfer(0); // Send address bytes
      SPI.transfer(0);
      SPI.transfer(0);
      digitalWriteDirect(fRAMhold, LOW); // Pause logging
      digitalWriteDirect(fRAMcs, HIGH);
    }
  #endif
  // Reset event counters
  for (int i = 0; i < MAX_GLOBAL_COUNTERS; i++) {
    GlobalCounterCounts[i] = 0;
    GlobalCounterHandled[i] = false;
  }
  // Read initial state of sensors
  for (int i = BNCInputPos; i < nInputs; i++) {
    if (inputEnabled[i] == 1) {
      inputState[i] =  digitalReadDirect(InputCh[i]);
      lastInputState[i] = inputState[i];
    } else {
      inputState[i] = logicLow[i];
      lastInputState[i] = logicLow[i];
    }
    inputOverrideState[i] = false;
  }
  #if MACHINE_TYPE == 4
    for (int i = FlexInputPos; i < FlexInputPos+nFlexIO; i++) { // Todo: Mod to read any digital inputs
      inputState[i] = logicLow[i];
      lastInputState[i] = logicLow[i];
    }
  #endif
  
  for (int i = nInputs; i < nInputs+MAX_GLOBAL_TIMERS; i++) { // Clear global timer virtual lines
    inputState[i] = 0;
  }
  clearSerialBuffers();
  // Reset timers
  StateStartTime = 0;
  CurrentTime = 0;
  nCurrentEvents = 0;
  nCyclesCompleted = 0;
  CurrentEvent[0] = 254; // 254 = no event
  RunningStateMatrix = 1;
  firstLoop = 1;
}
