#pragma once

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <EEPROM.h> 
//Modified, but not imported into the cppproj (one-time changes)
#include "libraries\encoder-arduino\ClickEncoder.h"
#include "libraries\PID\PID_v1.h" //Heavily modified
//Libraries imported into the project (continuous changes)
#include "MyFunctions.h"

//Definitions of constants
//#define DEBUG  //Debug info enable. Warning: consumes flash memory space and may impair communication with software!!

#pragma region Definitions
//#define NO_DS  -- Deprecated! [No Cold Junction Compensation (i.e. no ds18b20)]

#define FIRMWARE_VERSION "2b"
#define INFO "Lab PID temperature controller with GPIO."

#define PIN_GPIO 11                     //General purpose IO pin
#define PIN_INPUT A7                    //ADC input
#define TIMING 1000                     //Timing for PID in milliseconds
#define PIN_PWM 9                       //Pin for PWM PID output
#define PIN_ENCODER1 2                  //Encoder pins
#define PIN_ENCODER2 4
#define PIN_BUTTON 3
#define PIN_RW A2                       //LCD pins
#define PIN_RS A1
#define PIN_E A0
#define PIN_D4 8
#define PIN_D5 7
#define PIN_D6 6
#define PIN_D7 5
#define LBL_SPACING 1_ui8 //" "
#define LBL_LEN 2_ui8 //"S:"
#define LBLVALUE_LEN 6_ui8 //"123.45"            //Display configuration
#define LBLVALUE_ACCESSIBLE 6_ui8
#define LBLMODE_LEN 4_ui8 //" N0 ", "ERR!"
#define LBLMODE_ACCESSIBLE 2_ui8
#define LBLPOWER_LEN 5_ui8 //" 100 " "+100%"
#define LBLPOWER_ACCESSIBLE 3_ui8
#define DECIMAL_PLACES 3_ui8 //"123"         //Index of the last character in decimal part or length of decimal part of setpoint and input (interchangeable due to comma presence)
#define LCD_X 16_ui8
#define LCD_Y 2_ui8
#define INIT_AMP_COEFF 0.3                     //Factory-default temperature coeff. for external amplifier
#define PIN_ONEWIRE A3                 //OneWire bus pin
#define PIN_SS 10
#define EEPROM_START 1                  //EEPROM virtual start address
#define ERROR_TIMEOUT 3_ui8                 //Number of seconds to wait (re-checking the conditions) before showing error status
#define PWM_MAX 1023
#define PWM_MIN 0
#define ENCODER_STEPS 4_ui8

#define GPIO_DIRECTION(var) (((BV8(1) | BV8(2)) & (var)) >> 1_ui8)
#define GPIO_VALUE(var) (BV8(0) & (var))
#define SET_GPIO_DIRECTION(var, val) (var) &= BV8(0); (var) |= ((val) << 1_ui8)
#define MODE_NORMAL 0_ui8
#define MODE_AGGRESSIVE 1_ui8
#define MODE_DISTILL 2_ui8
#define MODE_MANUAL 3_ui8
#define CHANNEL_ADC 0_ui8
#define CHANNEL_MAX6675 1_ui8
#define CHANNEL_DS18B20 2_ui8
#define CHANNEL_COUNT 3_ui8
#define MENU_STATE_SETPOINT 0_ui8
#define MENU_STATE_MODE 1_ui8
#define MENU_STATE_CALIBRATION 2_ui8
#define MENU_STATE_POWER 3_ui8
#define CURSOR_NONE 0_ui8
#define CURSOR_UNDERLINE 1_ui8
#define CURSOR_BLINK 2_ui8
#define RIGHT_COLUMN_OFFSET (LBL_LEN + LBLVALUE_LEN + LBL_SPACING)
#define RIGHT_COLUMN_MARGIN (RIGHT_COLUMN_OFFSET + LBL_LEN)
#define LEFT_COLUMN_OFFSET 0_ui8
#define LEFT_COLUMN_MARGIN (LEFT_COLUMN_OFFSET + LBL_LEN)

#define SETPOINT_ADDR eepromStart //0
#define K_ADDR (SETPOINT_ADDR + sizeof(Setpoint)) //4
#define COEFF_ADDR (K_ADDR + sizeof(K)) //28
#define INTEGRAL_LIMIT_ADDR (COEFF_ADDR + sizeof(amplifierCoeff)) //32
#define CHANNEL_ADDR (INTEGRAL_LIMIT_ADDR + sizeof(integralTermLimit)) //36
#define AVERAGING_ADDR (CHANNEL_ADDR + sizeof(channelIndex)) //37
#define DSINDEX_ADDR (AVERAGING_ADDR + sizeof(averaging)) //40
#define DSDEFAULT_ADDR (DSINDEX_ADDR + sizeof(dsIndexes)) //42
#define MODE_ADDR (DSDEFAULT_ADDR + sizeof(defaultAmbientTemp)) //46
#define CALIBRATION_ADDR (MODE_ADDR + sizeof(regulationMode)) //47
#define DISTILL_POWER_ADDR (CALIBRATION_ADDR + sizeof(calibration)) //59
#define DISTILL_WINDOW_ADDR (DISTILL_POWER_ADDR + sizeof(distillExtraPower)) //63
#define RAMP_LIMIT_ADDR (DISTILL_WINDOW_ADDR + sizeof(distillTempWindow)) //67
#define GPIO_ADDR (RAMP_LIMIT_ADDR + sizeof(rampStepLimit)) //71
#define CJC_ADDR (GPIO_ADDR + sizeof(gpioMode)) //72
#pragma endregion

#pragma region Globals
//Global variables
//The quantity of global vars probably is more than it could be (some could have been transfered to local scope), 
//  but it's easier to estimate required space using arduino IDE if most of the vars are global. 
//Also, it seems the program has plenty of RAM left even with vars being organized in the way they are now, so nobody cares.

extern const uint8_t eepromStart;
extern const float mult[5];
extern uint8_t channelIndex;
extern uint8_t prevChannelIndex;
extern uint8_t regulationMode;
extern uint8_t dsAddresses[2][8];
extern uint8_t gpioMode;
extern float prevSetpoint;
extern float prevInputValue;
extern float Setpoint, Input, Output;
extern float integralTermLimit;
extern float K[CHANNEL_COUNT][2];
extern float amplifierCoeff;            
extern float calibration[CHANNEL_COUNT];
extern float distillExtraPower;         
extern float ambientTemp;               
extern float defaultAmbientTemp;        
extern float distillTempWindow;         
extern float dsChannelTemp;             
extern float rampStepLimit;            
extern uint8_t dsIndexes[2];            
extern int8_t power;                    
extern int8_t prevPower;               
extern uint8_t cursorType;   
extern uint8_t errorStatusDelay;    
extern bool averaging[CHANNEL_COUNT]; 
extern bool logging[2];        
extern bool condition[4];      
extern bool cjc;

extern ClickEncoder* encoder;
extern PID myPID;
#pragma endregion

void check_power();

#pragma region Common functions

char convert_regulation_mode(char src);

#pragma endregion

#pragma region OneWire

void ds_init();
void ds_read();

#pragma endregion

#pragma region Display logic

void lcd_init();
void lcd_draw_boot_screen();
void lcd_draw_reset_screen();
void lcd_draw_interface();
void lcd_process_cursor_type();
void lcd_process_cursor_position();
void lcd_process_fast();
void lcd_process_slow();

#pragma endregion

#pragma region Commands

void serial_process();
void serial_send_log();

#pragma endregion

#pragma region EEPROM memory

void mem_rw(bool wrt);
void mem_save_persistent();

#pragma endregion