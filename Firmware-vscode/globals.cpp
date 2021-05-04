#include "thermo.h"

const float mult[5] = { 100, 10 , 1, 0.1, 0.01 };                 // Multipliers for building up numbers
uint8_t channelIndex = CHANNEL_ADC;           // Current input used: 0 - ADC, 1 - SPI, 2 - DS18B20
uint8_t prevChannelIndex = 0xff;              // Backup used to check for changes, thus determining if display update is needed
uint8_t regulationMode = MODE_NORMAL;         // Regulation mode flag: 0 - normal, 1 - aggressive, 2 - distillation, 3 - manual
uint8_t dsAddresses[2][8];                                           // Address temporary storage, introduced to try to speed up OneWire-related actions. 0 - ambient, 1 - channel
uint8_t gpioMode = 0;                        //0bXXY where X denotes direction (input = 0, out = 1, pullup = 2) and Y stands for the state 
float prevSetpoint = -1;                                                     // Backup needed for display update necessity check...
float prevInputValue = -1;                                                   //  Same, more to come...
float Setpoint = 18.0f, Input, Output;                                       // PID main values
float integralTermLimit = 0.35;                                                // Integral term bound (in percents)
float K[CHANNEL_COUNT][2] = { { 18, 35 },{ 0.006, 0.19 },{ 15, 16 } };                       // Current regulation constants
float amplifierCoeff = INIT_AMP_COEFF;                                                    // Coefficient used for working with external amplifier
float calibration[CHANNEL_COUNT] = { 0, 0, 0 };                                        // Input calibration values
float distillExtraPower = 20;                                                          // Bias for distillation
float ambientTemp = 0;                                                       // Current DS18B20 (ambient) temperature
float defaultAmbientTemp = 25.0f;                                                  // Default ambient temperature
float distillTempWindow = 5;                                                          // Tolerated setpoint overshoot in D-mode
float dsChannelTemp = 0;                                                      // Current DS18B20 (channel) temperature
float rampStepLimit = 2;
uint8_t dsIndexes[2] = { 0, 1 };                                                 // DS18B20 indexes. 0 - ambient, 1 - channel
int8_t power = 0;                                                            // Current power value
int8_t prevPower = -1;                                                        // Similar backup for power value
uint8_t cursorType = CURSOR_NONE;                                                            // Current cursor type
uint8_t errorStatusDelay = 0;                                                           // Delay before error status (for the device not to lock when lowering setpoint manually)
bool averaging[CHANNEL_COUNT] = { 1, 1, 0 };                                             // Averaging state array: {mode 0, mode 1, mode 2}
bool logging = false;                                                 // Logging enable flags
bool condition[4] = { 0, 0, 0, 0 };                                          // Flags for error states behavior control {Safety error, DS chain: <= 1 sensor, DS chain: 0 sensors, DS chain changed}
bool cjc = false;
bool enableCooler[CHANNEL_COUNT] = { 1, 1, 1 };

ClickEncoder* encoder;                                                // Encoder object 
PID myPID(&Input, &Output, &Setpoint, K[0][0], K[1][0], K[2][0], DIRECT);    // PID Object