#pragma once

#include <avr/eeprom.h>
//Modified, but not imported into the cppproj (one-time changes)
#include "src\encoder-arduino\ClickEncoder.h"
#include "src\PID\PID_v1.h" //Heavily modified
#include "src\Shared Libraries\MyFunctions.h"

//Definitions of constants
//#define DEBUG  //Debug info enable. Warning: consumes flash memory space and may impair communication with software!!
//#define DEBUG_STARTUP //LCD detailed startup info
//#define DEBUG_DISPLAY
//#define DEBUG_TIMERS //Warning: may overflow serial buffer (outputs characters '1' and '2' when corresponding ISRs fire).

#define CHANNEL_COUNT 3_ui8
#define CHANNEL_ADC 0_ui8
#define CHANNEL_MAX6675 1_ui8
#define CHANNEL_DS18B20 2_ui8
#define MODE_NORMAL 0_ui8
#define MODE_AGGRESSIVE 1_ui8
#define MODE_DISTILL 2_ui8
#define MODE_MANUAL 3_ui8
#define ERROR_TIMEOUT 7_ui8                 //Number of seconds to wait (re-checking the conditions) before showing error status
#define CURSOR_NONE 0_ui8
#define CURSOR_UNDERLINE 1_ui8
#define CURSOR_BLINK 2_ui8
#define TIMING 1000                     //Timing for PID in milliseconds

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
inline void timers_set_heater_duty(uint16_t) __attribute__((always_inline));
void timers_set_heater_duty(uint16_t v)
{
    OCR1A = v;
}
inline void timers_set_cooler_duty(uint8_t) __attribute__((always_inline));
void timers_set_cooler_duty(uint8_t v)
{
    OCR2B = v;
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
bool lcd_is_editing();

#pragma endregion

#pragma region Commands

void serial_init();
void serial_process();

#pragma endregion

#pragma region EEPROM memory

bool mem_get_first_run();
void mem_set_first_run();
void mem_save();
void mem_load();
void mem_save_persistent();
uint16_t mem_get_gpio();

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