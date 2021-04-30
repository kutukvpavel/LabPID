#pragma once

#include <avr/eeprom.h>
//Libraries imported into the project (continuous changes)
#include "MyFunctions.h"
//Modified, but not imported into the cppproj (one-time changes)
#include "src\encoder-arduino\ClickEncoder.h"
#include "src\PID\PID_v1.h" //Heavily modified

//Definitions of constants
//#define DEBUG  //Debug info enable. Warning: consumes flash memory space and may impair communication with software!!
//#define DEBUG_STARTUP //LCD detailed startup info
//#define DEBUG_DISPLAY
//#define DEBUG_TIMERS

#pragma region Definitions
//#define NO_DS  -- Deprecated! [No Cold Junction Compensation (i.e. no ds18b20)]

#define FIRMWARE_VERSION "2b"
#define INFO "Lab PID temperature controller with GPIO."

#define AVERAGING_WINDOW 3_ui8
#define PIN_COOLER 3
#define PIN_INPUT A7                    //ADC input
#define TIMING 1000                     //Timing for PID in milliseconds
#define PIN_PWM 9                       //Pin for PWM PID output
#define PIN_ENCODER1 A0                  //Encoder pins
#define PIN_ENCODER2 A1
#define PIN_BUTTON A2
#define PIN_RS 7                //LCD pins
#define PIN_E 6
#define PIN_D4 2
#define PIN_D5 A3
#define PIN_D6 4
#define PIN_D7 5
#define PIN_FUSE_SENSE A6
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
#define PIN_ONEWIRE 8                 //OneWire bus pin
#define PIN_SS 10
#define EEPROM_START 1                  //EEPROM virtual start address
#define ERROR_TIMEOUT 7_ui8                 //Number of seconds to wait (re-checking the conditions) before showing error status
#define PWM_MAX 62499u
#define PWM_MIN -1
#define ENCODER_STEPS 4_ui8

#define GPIO_MODE_MAP 12, 4  //Number of consequtive inputs (element index % 2 != 0) / outputs (element index % 2 == 0)
#define GPIO_IC_ADDRESS 0x20_ui8
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
#pragma endregion

#pragma region Globals
//Global variables
//The quantity of global vars probably is more than it could be (some could have been transfered to local scope), 
//  but it's easier to estimate required space using arduino IDE if most of the vars are global. 
//Also, it seems the program has plenty of RAM left even with vars being organized in the way they are now, so nobody cares.

extern const float mult[5];
extern uint8_t channelIndex;
extern uint8_t prevChannelIndex;
extern uint8_t regulationMode;
extern uint8_t dsAddresses[2][8];
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
extern bool logging;        
extern bool condition[4];      
extern bool cjc;
extern bool enableCooler[CHANNEL_COUNT];
extern bool gpioOK;

extern ClickEncoder* encoder;
extern PID myPID;
#pragma endregion

#pragma region Timers

void timers_init();
inline void timers_set_pwm_duty(uint16_t) __attribute__((always_inline));
void timers_set_pwm_duty(uint16_t v)
{
    OCR1A = v;
}

#pragma endregion

#pragma region Common functions

char convert_regulation_mode(char src);
void check_power();

#pragma endregion

#pragma region OneWire

void ds_init();
void ds_read();

#pragma endregion

#pragma region Display logic

void lcd_init();
void lcd_draw_message(const char* msg);
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

bool mem_get_first_run();
void mem_set_first_run();
void mem_save();
void mem_load();
void mem_save_persistent();

#pragma endregion

#pragma region GPIO

void gpio_init();
uint16_t gpio_read_all();
void gpio_write_all(uint16_t val);
bool gpio_read(uint8_t address);
void gpio_write(uint8_t address, bool value);
void gpio_write(uint8_t code);
uint16_t gpio_get_output_register();

#pragma endregion