/*  *
*  NB! Some of the libraries used (e.g. PID) have been modified! The code is valid only if modified libs are used.
*
*  Current major TODO:
*  None
*  _____________________________________
*  Developed by Kutukov Pavel, 2016-2020.
*  This software was developed to fulfill the author's personal needs only and is provided strictly as-is.
*/

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <SPI.h>
#include <LiquidCrystal.h>
#include <EEPROM.h> 
//Modified, but not imported into the cppproj (one-time changes)
#include "libraries\MAX6675-master\src\max6675.h" //SPI thermocouple amplifier
#include "libraries\OneWire\OneWire.h"
#include "libraries\DallasTemperature\DallasTemperature.h"
#include "libraries\MsTimer2\MsTimer2.h"
#include "libraries\TimerOne-master\TimerOne.h"
#include "libraries\encoder-arduino\ClickEncoder.h"
#include "libraries\PID\PID_v1.h" //Heavily modified
#include "libraries\Average-master\Average.h"
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

uint8_t menuState = MENU_STATE_SETPOINT;      // Menu state flag: 0 - setpoint, 1 - mode, 2 - calibration (input field), 3 - power (for manual mode)
uint8_t channelIndex = CHANNEL_ADC;           // Current input used: 0 - ADC, 1 - SPI, 2 - DS18B20
uint8_t prevChannelIndex = 0xff;              // Backup used to check for changes, thus determining if display update is needed
uint8_t regulationMode = MODE_NORMAL;         // Regulation mode flag: 0 - normal, 1 - aggressive, 2 - distillation, 3 - manual
uint8_t prevRegulationMode = 0xff;
//char b[35];                                                           // Buffer for final log string
uint8_t dsAddresses[2][8];                                           // Address temporary storage, introduced to try to speed up OneWire-related actions. 0 - ambient, 1 - channel
char serialBuffer[16];                                                  // Virtual input buffer for serial interface
uint8_t gpioMode = 0;                        //0bXXY where X denotes direction (input = 0, out = 1, pullup = 2) and Y stands for the state 
float prevSetpoint = -1;                                                     // Backup needed for display update necessity check...
float prevInputValue = -1;                                                   //  Same, more to come...
float Setpoint, Input, Output;                                       // PID main values
float integralTermLimit = 0.35;                                                // Integral term bound (in percents)
float K[][2] = { { 18, 35 },{ 0.006, 0.19 },{ 15, 16 } };                       // Current regulation constants
float amplifierCoeff = INIT_AMP_COEFF;                                                    // Coefficient used for working with external amplifier
float calibration[3] = { 0, 0, 0 };                                        // Input calibration values
float distillExtraPower = 0;                                                          // Bias for distillation
float ambientTemp = 0;                                                       // Current DS18B20 (ambient) temperature
float defaultAmbientTemp = 25.0f;                                                  // Default ambient temperature
float distillTempWindow = 5;                                                          // Tolerated setpoint overshoot in D-mode
float dsChannelTemp = 0;                                                      // Current DS18B20 (channel) temperature
float rampStepLimit = 2;
const float mult[] = { 100, 10 , 1, 0.1, 0.01 };                 // Multipliers for building up numbers
uint8_t dsIndexes[2] = { 0, 1 };                                                 // DS18B20 indexes. 0 - ambient, 1 - channel
int8_t power = 0;                                                            // Current power value
int8_t prevPower = -1;                                                        // Similar backup for power value
uint8_t relativeCursorX = 0;                                                              //
uint8_t absoluteCursorY = 0;                                                              //
uint8_t cursorType = CURSOR_NONE;                                                            // Current cursor type
uint8_t serialBufferIndex = 0;                                                            // Index used for serial data transferring to virtual buffer (serialBuffer)
uint8_t errorStatusDelay = 0;                                                           // Delay before error status (for the device not to lock when lowering setpoint manually)
uint8_t lineEndingIndex = 0;
const uint8_t eepromStart = EEPROM_START;                                         // Starting EEPROM address  TODO: consider changing it to dynamic var                                                              
uint16_t counter750ms = 0;                                                             // Virtual timer counter (750ms)
uint16_t counter500ms = 0;                                                            // Virtual timer counter (500ms)
uint16_t counterForDisplay = 0;                                                           // Virtual timer counter (display)
uint16_t eepromCurrentAddr = 0;                                                             // EEPROM address buffer
int16_t encoderValue = 0;
bool averaging[] = { 1, 1, 0 };                                             // Averaging state array: {mode 0, mode 1, mode 2}
bool menuBlankState = 0;                                                      // Backup used to determine if some fields are already blanked out and no update is needed (menu in mode #2)
bool logging[] = { 0, 0 };                                                 // Logging enable flags
bool updateDisplay = true;                                                        // Flag for display update enable
bool condition[4] = { 0, 0, 0, 0 };                                          // Flags for error states behavior control {Safety error, DS chain: <= 1 sensor, DS chain: 0 sensors, DS chain changed}
bool max6675Ready = true;                                                     // Flag of readiness for reading (for MAX6675 proper timing: >400ms)
bool cjc = false;
bool lineEndingPresent = false;
ClickEncoder *encoder;                                                // Encoder object 
LiquidCrystal lcd(PIN_RS, PIN_E, PIN_D4, PIN_D5, PIN_D6, PIN_D7);     // LCD Object 
PID myPID(&Input, &Output, &Setpoint, K[0][0], K[1][0], K[2][0], DIRECT);    // PID Object
MAX6675 thermocouple;                                  // MAX6675 thermocouple amplifier object
Average<float> averageContainer(3);                                                // Averaging container
OneWire oneWire(PIN_ONEWIRE);                                        // OneWire bus object
DallasTemperature dsSensors(&oneWire);                                  // DS18B20 Object
#pragma endregion

#pragma region ISRs
ISR(TIMER1_OVF_vect)                               // 1 sec timer routine
{
	myPID.Compute();                            // For PID computations, safety check and output updating
	safety_check();
	Timer1.setPwmDuty(PIN_PWM, static_cast<uint16_t>(Output));
	logging[1] = true;                            //Also enables logging flag for the log to be periodically created (with 1 sec period, of course)
}

void isr2(void)                               // 1 ms timer routine
{
	encoder->service();                         // For encoder poll
	if (counterForDisplay > 199)
	{
		display();                          // Display update logic
		updateDisplay = 1;
		counterForDisplay = 0;
	}
	if (counter500ms > 499)                          // Timer for max6675 thermocouple interface polling (500ms)
	{
		max6675Ready = 1;
		counter500ms = 0;
	}
	counter750ms++;                                       //OneWire poll timer
	counter500ms++;                                      //max6675 timer
	counterForDisplay++;                                     //Display timer

}
#pragma endregion

#pragma region Simple functions
void sendSpace()
{
	Serial.print(' ');
}

template<typename T> void mem(T *variable, bool wrt = false)    //Function template for working with EEPROM memory
{
	if (wrt) { EEPROM.put(eepromCurrentAddr, *variable); }
	else { EEPROM.get(eepromCurrentAddr, *variable); };
	eepromCurrentAddr += sizeof(*variable);
}

char ConvertMode(char src)  //Routine for converting user-friendly into system-recognized values of current mode (var. "regulationMode") and vice-versa
{
	switch (src)
	{
	case 'N':
		return MODE_NORMAL;
	case 'A':
		return MODE_AGGRESSIVE;
	case 'D':
		return MODE_DISTILL;
	case 'M':
		return MODE_MANUAL;
	case MODE_NORMAL:
		return 'N';
	case MODE_AGGRESSIVE:
		return 'A';
	case MODE_DISTILL:
		return 'D';
	case MODE_MANUAL:
		return 'M';
	default:
		return 0xff;
	}
}
#pragma endregion

#pragma region OneWire
void setup_ds()
{
	dsSensors.begin();                            //Init OneWire bus and sensors
	condition[1] = false;                             //Defaulting, required for further calls
	int dsc = dsSensors.getDeviceCount();         //Get the number of sensors
	if (dsc)   //Warn in case of one or more sensors aren't present
	{
		if (dsc > 1)
		{
			for (uint8_t i = 0; i < 2; i++)
			{
				if (dsc <= dsIndexes[i])
				{
					condition[3] = 1; //Raise appropriate flag if we've to rearrange the EEPROM'ed chain structure
					dsIndexes[i] = dsc;  //Set the out-of-bounds-sensor index to last+1 and decrement it until we reach a free spot
					bool moveFurther = true;
					while (moveFurther)
					{
						dsIndexes[i] -= 1;
						moveFurther = false;
						for (uint8_t j = 0; j < 2; j++)
						{
							if (j == i) continue;
							moveFurther = moveFurther || (dsIndexes[j] == dsIndexes[i]);
						}
					}
				}
				dsSensors.getAddress(dsAddresses[i], dsIndexes[i]);      //If everything's OK then load addresses (to make further communication processes faster) and send request to start the conversion
			}
		}
		else
		{
			condition[1] = 1;
			dsSensors.getAddress(dsAddresses[1], dsIndexes[1]); //Treat single sensor as a channel (not as ambient)
		}
		dsSensors.requestTemperatures();
	}
	else
	{
		condition[1] = 1;
		condition[2] = 1;
	}
}

void readds()         // One-wire devices poll routine
{
	if (condition[2] || condition[1])                          //If there are no sensors then set default ambient temperature
	{
		ambientTemp = defaultAmbientTemp;
	}
	else
	{
		ambientTemp = dsSensors.getTempC(dsAddresses[0]);
	}
	if (!condition[2])
	{
		if (channelIndex == CHANNEL_DS18B20)                           // Poll second sensor only in mode 2
		{
			dsChannelTemp = dsSensors.getTempC(dsAddresses[1]);
		}
		dsSensors.requestTemperatures();
	}
}
#pragma endregion

#pragma region Display logic
void setcursor()                              // Cursor modes switching
{
	switch (cursorType)                              // 0 - disabled, 1 - underlining, 2 - underlining + blinking
	{
	case CURSOR_NONE:
		lcd.noBlink();
		lcd.noCursor();
		break;
	case CURSOR_UNDERLINE:
		lcd.noBlink();                        // Don't forget to turn previous mode off, because it isn't reset automatically
		lcd.cursor();
		break;
	case CURSOR_BLINK:
		lcd.blink();
	}
}

void print_double(double dbl, int x, int y)   //Encapsulated for convenience, prints a double on the screen at specified coordinates
{
	char b[7];
	dtostrf(dbl, 6, 2, b);
	//sprintf(b, "%s", str_temp);
	lcd.setCursor(x, y);
	lcd.print(b);
}

void pwrprint()
{
	char b[6];
	sprintf(b, "%4i%%", power);
	prevPower = power;
	lcd.setCursor(RIGHT_COLUMN_MARGIN, 1);
	lcd.print(b);
}

void curpos()                             // Setting the cursor position, including shift chosen according to the field being edited
{
	absoluteCursorY = ((menuState == MENU_STATE_CALIBRATION) || (menuState == MENU_STATE_POWER)) ? 1 : 0;
	//if ((menuState == MENU_STATE_MODE) && (relativeCursorX > 1_ui8)) relativeCursorX = 1;         // A workaround for mode field
	uint8_t margin = 0;
	switch (menuState)
	{
	case MENU_STATE_MODE:
		++margin;
	case MENU_STATE_POWER:
		margin += RIGHT_COLUMN_MARGIN;
		break;
	default:
		margin = LEFT_COLUMN_MARGIN;
		break;
	}
	lcd.setCursor(relativeCursorX + margin, absoluteCursorY);
}

void display(void)                              // Main routine for display logic
{
	int16_t encoderRawValue = encoder->getValue();                  // Get encoder and button value/state
	ClickEncoder::Button button = encoder->getButton();
	bool force = false;
	if (button != ClickEncoder::Open) {          // Button handling
												 /*#ifdef DEBUG
												 Serial.print("Button: ");
												 Serial.println(button);
												 #endif*/
		switch (button) {
		case ClickEncoder::Clicked:             // Clicked
			switch (cursorType)
			{
			case CURSOR_UNDERLINE:                             // If underlining cursor is on then switch to the blinking one
				cursorType = CURSOR_BLINK;
				break;
			case CURSOR_BLINK:
				cursorType = CURSOR_UNDERLINE;                            // If blinking is on, then do vice-versa
				break;
			default: break;
			}
			break;
		case ClickEncoder::DoubleClicked:       // Double-clicked
			if (cursorType == CURSOR_NONE)                      // Cycle: Off -> setpoint field -> mode field -> input field -> Off
			{
				cursorType = CURSOR_UNDERLINE;
			}
			else
			{
				if (((menuState > MENU_STATE_MODE) && (regulationMode != MODE_MANUAL)) || (menuState == MENU_STATE_POWER))
				{
					cursorType = CURSOR_NONE;
					menuState = MENU_STATE_SETPOINT;
				}
				else
				{
					++menuState;
				}
			}
			encoderValue = 0;
			force = true;
			break;
		default:
			break;
		}
	}
	if ((encoderRawValue != 0) || force)                         // Check if anything changed since last cycle
	{
		encoderValue += encoderRawValue;
		encoderValue %= LBLMODE_ACCESSIBLE * LBLPOWER_ACCESSIBLE * LBLVALUE_ACCESSIBLE; //Truncate the value to avoid overflow while preserving modular arithmetic properties
		/*#ifdef DEBUG
		Serial.println("Encoder!");
		#endif*/
		uint8_t bound = 1;     // Choose bounds: 6 chars for setpoint field, 2 characters for mode field, 6 characters for input field, 3 characters for power field
		switch (menuState)
		{
		case MENU_STATE_SETPOINT:
		case MENU_STATE_CALIBRATION:
			bound = LBLVALUE_ACCESSIBLE;
			break;
		case MENU_STATE_MODE:
			bound = LBLMODE_ACCESSIBLE;
			break;
		case MENU_STATE_POWER:
			bound = LBLPOWER_ACCESSIBLE;
		default: 
			break;
		}
		switch (cursorType)                             // Determine what to do with the values
		{
		case CURSOR_UNDERLINE:                               // 1 - Move cursor
			relativeCursorX = (encoderValue >= 0 ? encoderValue % bound : (bound - 1 + (encoderValue + 1) % bound));
			if (relativeCursorX == 3)                            // Skip the comma in setpoint field
			{
				(encoderRawValue > 0) ? relativeCursorX++ : relativeCursorX--;
			}
			break;
		case CURSOR_BLINK:                            // 2 - Change value
		{
			int k = relativeCursorX;
			if (relativeCursorX > DECIMAL_PLACES) k--;    // Fix skipped comma index incrementation (in order to fit into multipliers array's bounds)
			switch (menuState)                      // Determine what value to change
			{
			case MENU_STATE_SETPOINT:                    // 0 - setpoint
				Setpoint += mult[k] * encoderRawValue;        // Change setpoint using array of multipliers chosen according to the cursor index
				break;
			case MENU_STATE_MODE:                    // 1 - mode
				switch (relativeCursorX)
				{
				case 0:                     // Scroll between N, A, D and M
					regulationMode = static_cast<uint8_t>(abs((regulationMode + encoderRawValue) % 4));
					break;
				case 1:                     // Scroll between 0 and 2
					channelIndex = static_cast<uint8_t>(abs((channelIndex + encoderRawValue) % 3));
					break;
				default:
					break;
				}
				break;
			case MENU_STATE_CALIBRATION:                      // 2 - calibration
				calibration[channelIndex] += mult[k] * encoderRawValue; //Similar to setpoint change
				break;
			case MENU_STATE_POWER:                      // 3 - power
				power += mult[k] * encoderRawValue;
				checkpwr();
				break;
			default: break;
			}
			break;
		}
		}
	}
}

void display2()
{
	if (Setpoint != prevSetpoint)                  // Searching for changed data and partially updating screen
	{
		prevSetpoint = Setpoint;
		print_double(prevSetpoint, 2, 0);
	}
	if ((cursorType != CURSOR_NONE) && (channelIndex == CHANNEL_DS18B20))              // Blank out input and power fields if menu is enabled in mode #2 (due to non-updating values)
	{
		if (menuBlankState)
		{
			if (menuState == MENU_STATE_CALIBRATION) print_double(prevInputValue, LEFT_COLUMN_MARGIN, 1); // Don't blank out input field when user's editing calibration values
			if (regulationMode == MODE_MANUAL) pwrprint();     // always update power value in manual mode (otherwise it will appear to be stuck while in fact it will be changing)
		}
		else
		{
			for (uint8_t i = 0; i < LBLVALUE_LEN; i++)
			{
				lcd.setCursor(LEFT_COLUMN_MARGIN + i, 1);
				lcd.print(' ');
			}
			for (uint8_t i = 0; i < LBLPOWER_LEN; i++)
			{
				lcd.setCursor(RIGHT_COLUMN_MARGIN + i, 1);
				lcd.print(' ');
			}
			menuBlankState = true;
		}
	}
	else
	{
		if ((Input != prevInputValue) || menuBlankState)   // Remember to fill blanked fields
		{
			prevInputValue = Input;
			print_double(prevInputValue, LEFT_COLUMN_MARGIN, 1);
		}
		if ((power != prevPower) || menuBlankState)
		{
			pwrprint();
		}
		menuBlankState = 0;
	}
	if (condition[0] && (errorStatusDelay > ERROR_TIMEOUT))                    // Major error sign - label "ERR!" in mode field
	{
		lcd.setCursor(RIGHT_COLUMN_MARGIN, 0);
		lcd.print("ERR!");
	}
	else
	{
		if ((channelIndex != prevChannelIndex) || (regulationMode != prevRegulationMode))             // Mode field structure (normal): "M: {N,A,D,M}[0..2]{!}"
		{
			char b[5];
			sprintf(b, " %c%1u ", ConvertMode(regulationMode), channelIndex);
			lcd.setCursor(RIGHT_COLUMN_MARGIN, 0);
			lcd.print(b);
			prevChannelIndex = channelIndex;
			prevRegulationMode = regulationMode;
			if (!myPID.GetMode() == MANUAL) myPID.SetMode(AUTOMATIC); //Reset PID to fix a strange bug causing power to stay at ~20% after channel selection no matter how input changes
		}
		if ((condition[1] && cjc) || condition[2] || condition[3])                  // Warning sign - exclamation mark in the right upper corner of the screen
		{
			lcd.setCursor(RIGHT_COLUMN_MARGIN + LBLMODE_LEN, 0);
			lcd.print('!');
		}
	}
}
#pragma endregion

#pragma region Commands
void serial()
{
	if (lineEndingPresent)
	{
		if (--serialBufferIndex >= 1)
		{
			if (serialBuffer[--serialBufferIndex] != '\r') serialBufferIndex++;
			if (serialBufferIndex > 0) cmd();                         //Run cmd depending on received data
		}
		serialBufferIndex = 0;
		lineEndingPresent = false;
	}
	else
	{
		if (Serial.available())                 //Update serial data
		{
			serialBuffer[serialBufferIndex] = static_cast<uint8_t>(Serial.read());
			if (serialBuffer[serialBufferIndex] == '\n') lineEndingPresent = true;
			++serialBufferIndex;
		}
	}
}

float convert(float def)                      // Putting all converted characters together into a double, again using our multipliers array
{
	if (serialBufferIndex < 2_ui8) return def;
	float res = decodeFloat(serialBuffer, 2, serialBufferIndex - 2_ui8);
	if (!(res == res)) return def; //NAN test
	return res;
}

void cmd()                       // Handling PC commands, general packet structure: {[0..9],{A,T,P,I,D,C,S}}{+,-}(000.0000){CR+LF,{}}
{
	switch (serialBuffer[0])
	{
	case '0':
		Setpoint = convert(Setpoint);               // 0 - basic setpoint changing
#ifdef DEBUG
		Serial.println(Setpoint);
#endif
		break;
	case '1':
		K[0][0] = convert(K[0][0]);                // 1 - First coeff. (P)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		EEPROM.get(K_ADDR, K);
		Serial.println(K[0][0], 5);
#endif
		break;
	case '2':
		K[1][0] = convert(K[1][0]);                // 2 - Second coeff. (I)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		Serial.println(K[1][0], 5);
#endif
		break;
	case '3':
		K[2][0] = convert(K[2][0]);                // 3 - Third coeff. (D)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		Serial.println(K[2][0], 5);
#endif
		break;
	case '9':                           // 9 - Log
		logging[0] = (convert(logging[0]) > 0);
#ifdef DEBUG
		Serial.println(logging[0]);
#endif
		break;
	case '8':                           // 8 - Info
		cfOut();
		break;
	case '4':
		amplifierCoeff = convert(amplifierCoeff);                     // 4 - external amplifier coefficient
		EEPROM.put(COEFF_ADDR, amplifierCoeff);
#ifdef DEBUG
		Serial.println(amplifierCoeff, 4);
#endif   
		break;
	case '5':
		integralTermLimit = convert(integralTermLimit);                // 5 - integral cut-off threshold
		EEPROM.put(INTEGRAL_LIMIT_ADDR, integralTermLimit);
#ifdef DEBUG
		Serial.println(integralTermLimit, 4);
#endif   
		break;
	case '7':                                 // 7 - Input channel
		channelIndex = static_cast<uint8_t>(convert(channelIndex));
#ifdef DEBUG
		Serial.println((int)channelIndex);
#endif
		break;
	case '6':                                 // 6 - Averaging enable/disable for current mode
		averaging[channelIndex] = (convert(averaging[channelIndex]) > 0);
		EEPROM.put(AVERAGING_ADDR, averaging);
#ifdef DEBUG
		Serial.print(averaging[0]); Serial.print(" ");
		Serial.print(averaging[1]); Serial.print(" ");
		Serial.println(averaging[2]);
#endif 
		break;
	case 'T':                                  // T - Default ambient temperature
		defaultAmbientTemp = convert(defaultAmbientTemp);
		EEPROM.put(DSDEFAULT_ADDR, defaultAmbientTemp);
#ifdef DEBUG
		Serial.println(defaultAmbientTemp);
#endif      
		break;
	case 'A':                                 // A - regulation mode (N,A,D,M -> 0..3)
		regulationMode = static_cast<uint8_t>(convert(regulationMode));
#ifdef DEBUG
		Serial.println(ConvertMode(regulationMode));
#endif  
		break;
	case 'P':
		K[0][1] = convert(K[0][1]);                      // P - First aggressive coeff. (Pa)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		Serial.println(K[0][1], 5);
#endif
		break;
	case 'I':
		K[1][1] = convert(K[1][1]);                      // I - Second aggressive coeff. (Ia)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		Serial.println(K[1][1], 5);
#endif
		break;
	case 'D':
		K[2][1] = convert(K[2][1]);                      // D - Third aggressive coeff. (Da)
		EEPROM.put(K_ADDR, K);
#ifdef DEBUG
		Serial.println(K[2][1], 5);
#endif
		break;
	case 'S':                                 // S - sensor address (by current index)
	{
		dsIndexes[0] = static_cast<uint8_t>(serialBuffer[2] - '0');
		dsIndexes[1] = static_cast<uint8_t>(serialBuffer[3] - '0');
		EEPROM.put(DSINDEX_ADDR, dsIndexes);
#ifdef DEBUG
		Serial.print(dsIndexes[0]); Serial.print(" "); Serial.println(dsIndexes[1]);
#endif     
		break;
	}
	case 'C':                                 // C - current calibration coeff
		calibration[channelIndex] = convert(calibration[channelIndex]);
#ifdef DEBUG
		Serial.print(calibration[0], 5); Serial.print(" ");
		Serial.print(calibration[1], 5); Serial.print(" ");
		Serial.println(calibration[2], 5);
#endif
		break;
	case 'V':
		Serial.println(">I:");
		Serial.print("Firmware version: "); Serial.println(FIRMWARE_VERSION);
		Serial.print("Additional info: "); Serial.println(INFO);
		break;
	case 'B':                                         // B - distillation bias
		distillExtraPower = convert(distillExtraPower);
		EEPROM.put(DISTILL_POWER_ADDR, distillExtraPower);
#ifdef DEBUG
		Serial.println(distillExtraPower, 4);
#endif
		break;
	case 'L':                                           // S - current sensors' addresses
		Serial.println(">S:");
		for (uint8_t k = 0; k < arraySize(dsAddresses); k++)
		{
			for (short j = 0; j < 8; j++)
			{
				Serial.print(dsAddresses[k][j]);
			}
			Serial.print(k > 0 ? "\r\n" : " ");
		}
		break;
	case 'W':                                     // W - set power (only for manual mode)
		if (regulationMode != MODE_MANUAL) break;
		power = static_cast<int8_t>(convert(power));
		checkpwr();
		break;
	case 'O':                                         // O - tolerated distillation overshoot
		distillTempWindow = abs(convert(distillTempWindow));
		EEPROM.put(DISTILL_WINDOW_ADDR, distillTempWindow);
#ifdef DEBUG
		Serial.println(distillTempWindow, 2);
#endif
		break;
	case 'R':
		rampStepLimit = convert(rampStepLimit);
		EEPROM.put(RAMP_LIMIT_ADDR, rampStepLimit);
#ifdef DEBUG
		Serial.println(rampStepLimit, 2);
#endif
		break;
	case 'G':
		gpioMode = static_cast<uint8_t>(convert(gpioMode));
		EEPROM.put(GPIO_ADDR, gpioMode);
#ifdef DEBUG
		Serial.println(gpioMode, BIN);
#endif
		Serial.print(">G:");
		Serial.println(digitalRead(PIN_GPIO));
		break;
	case 'J':
		cjc = (convert(cjc) > 0);
		EEPROM.put(CJC_ADDR, cjc);
#ifdef DEBUG
		Serial.println(cjc);
#endif
		break;
	default: break;
	}
}

void cfOut(void)                              //Outputs all the current technical data
{
	Serial.println(">D:");
	Serial.print(Input); sendSpace();
	Serial.print(ambientTemp); sendSpace();
	Serial.print(ConvertMode(regulationMode)); Serial.println(static_cast<int>(channelIndex));
	for (uint8_t i = 0; i < arraySize(K[0]); ++i)
	{
		for (uint8_t j = 0; j < (arraySize(K) - 1_ui8); ++j)
		{
			Serial.print(K[j][i], 4); sendSpace();
		}
		Serial.println(K[arraySize(K) - 1_ui8][i], 4);
	}
	Serial.print(integralTermLimit, 4); sendSpace();
	Serial.print(distillExtraPower, 4); sendSpace();
	Serial.println(amplifierCoeff, 4);
	for (uint8_t i = 0; i < (arraySize(calibration) - 1_ui8); i++)
	{
		Serial.print(calibration[i], 4); sendSpace();
	}
	Serial.println(calibration[arraySize(calibration) - 1_ui8], 4);
	for (uint8_t i = 0; i < (arraySize(averaging) - 1_ui8); i++)
	{
		Serial.print(averaging[i], 4); sendSpace();
	}
	Serial.println(averaging[arraySize(averaging) - 1_ui8], 4);
	Serial.print(Setpoint); sendSpace();
	Serial.print(defaultAmbientTemp); sendSpace();
	Serial.println(distillTempWindow);
	Serial.print(rampStepLimit); sendSpace();
	Serial.print(static_cast<int>(gpioMode)); sendSpace();
	Serial.println(cjc);
}
#pragma endregion

#pragma region Checks
void checkpwr()
{
	if (power < 0)
	{
		power = 0;
	}
	else
	{
		if (power > 100) power = 100;
	}
}

void safety_check() // Check if something goes wrong (real temp rises more than 20C above the setpoint, ambient temperature rises above 60C, ds18b20 failed, no ds18b20 when using mode #2, broken thermocouple)
{
	condition[0] = (
		((Input - Setpoint > 20) && (Output > 0)) ||
		(ambientTemp > 60) ||
		(ambientTemp < -25) ||
		(dsChannelTemp < -25) ||
		(condition[2] && (channelIndex == CHANNEL_DS18B20)) ||
		(!(Input == Input)));
	if (condition[0])
	{
		if (errorStatusDelay > ERROR_TIMEOUT)
		{
			if (myPID.GetMode() == AUTOMATIC)
			{
				myPID.SetMode(MANUAL);
				Serial.println(">E!");
			}
			Output = 0;
		}
		else
		{
			errorStatusDelay++;
		}
	}
	else
	{
		if ((myPID.GetMode() == MANUAL) && (regulationMode != MODE_MANUAL))             // Return to normal operation if the problem's solved (and if automatic mode is enabled)
		{
			myPID.SetMode(AUTOMATIC);
			errorStatusDelay = 0;
		}
	}
}
#pragma endregion

void read_input()                           //Update input according to the mode being used
{
	float in = 0;
	switch (channelIndex)
	{
	case CHANNEL_ADC:                              // 0 - external amplifier (ADC input)
		in = analogRead(PIN_INPUT) * amplifierCoeff + (cjc ? ambientTemp : 0);
		break;
	case CHANNEL_MAX6675:                              // 1 - max6675 (SPI)
		in = static_cast<float>(thermocouple.readCelsius());
		break;
	case CHANNEL_DS18B20:                              // 2 - ds18b20  (1-wire)
		in = dsChannelTemp;   // Updated in "readds"
		break;
	default:
		break;
	}
	in += calibration[channelIndex];          // Apply calibration value
	if (averaging[channelIndex])
	{
		averageContainer.push(in);                           // Apply averaging if enabled for current mode
		Input = averageContainer.mean();
	}
	else
	{
		Input = in;
	}
}

void setPID()
{
	switch (regulationMode)                              //Apply updated coefficients according to current mode of regulation (actual update process takes place in 'cmd' and 'convert')
	{
	case MODE_NORMAL:
	case MODE_AGGRESSIVE:
		myPID.SetTunings(K[0][regulationMode], K[1][regulationMode], K[2][regulationMode]);
		myPID.SetDistillBias(0);
		break;
	case MODE_DISTILL:
		myPID.SetTunings(K[0][1], K[1][1], K[2][1]);
		myPID.SetDistillBias((Input - Setpoint > distillTempWindow) ? 0 : distillExtraPower);  //Check for overshoot (for D-mode only)
		break;
	default: break;
	}
	myPID.SetMaxITerm(integralTermLimit);
	myPID.SetRampLimit(rampStepLimit);
	if ((cursorType != CURSOR_NONE) && (channelIndex == CHANNEL_DS18B20))            //Disable PID regulating if input is not updated
	{
		myPID.SetMode(MANUAL);
	}
	else
	{
		if ((myPID.GetMode() == MANUAL) && (!condition[0]) && (regulationMode != MODE_MANUAL)) myPID.SetMode(AUTOMATIC);   //Enable PID regulation if there no more problems
	}
	if (regulationMode == MODE_MANUAL)            //If manual mode is enabled then turn off the PID and count output value based on power value
	{
		if (myPID.GetMode() != MANUAL) myPID.SetMode(MANUAL);
		Output = static_cast<float>(PWM_MAX * (power < 0 ? 0 : static_cast<long>(power))) / 100; //Negative power values are reserved yet
	}
	else
	{
		power = static_cast<int8_t>((Output * 100) / PWM_MAX);             //Else - count power value based on output
	}
}

void setup() {
	asm("cli");   // No interrupts while we aren't ready yet
				  // put your setup code here, to run once:
	pinMode(PIN_RW, OUTPUT);                                // LCD & PWM stuff
	digitalWrite(PIN_RW, LOW);
	lcd.begin(LCD_X, LCD_Y);
	lcd.clear();
	lcd.home();
	lcd.print("Booting...");
	Serial.begin(9600);                                     // Initialize serial communications with the PC
	SPI.begin();                                            // Init hardware SPI for max6675-compatible devices
															//int j;
	uint8_t t = 0;
	eepromCurrentAddr = eepromStart;                                             // Put 
	pinMode(PIN_BUTTON, INPUT_PULLUP);                          //Provide ability to reset to factory defaults by holding encoder's button while booting up
	t = EEPROM.read(0);                                           //Check if this is the first run and we'll have to fill the EEPROM up too
	if ((t != 0x01) || (!digitalRead(PIN_BUTTON)))
	{
		lcd.home();
		lcd.print("Resetting...");
		delay(500);

		for (uint16_t i = 0; i < EEPROM.length(); ++i) EEPROM.write(i, 0);

		Setpoint = 18.0f;

		mem(&Setpoint, 1);
		mem(&K, 1);
		mem(&amplifierCoeff, 1);
		mem(&integralTermLimit, 1);
		mem(&channelIndex, 1);
		mem(&averaging, 1);
		mem(&dsIndexes, 1);
		mem(&defaultAmbientTemp, 1);
		mem(&regulationMode, 1);
		mem(&calibration, 1);
		mem(&distillExtraPower, 1);
		mem(&distillTempWindow, 1);
		mem(&rampStepLimit, 1);
		mem(&gpioMode, 1);
		mem(&cjc, 1);

		EEPROM.write(0, 0x01);                                //Set the 'filled' flag on
		eepromCurrentAddr = eepromStart;

#ifdef DEBUG
		Serial.println("EEPROM refilled.");
#endif
	}
	lcd.home();
	lcd.print("Booting...");
	analogReference(EXTERNAL);                              // Initialize ADC stuff
	pinMode(PIN_INPUT, INPUT);
	pinMode(PIN_PWM, OUTPUT);
	digitalWrite(PIN_PWM, LOW);

	mem(&Setpoint, 0);                                      //Get all of the saved parameters loaded into memory
	mem(&K, 0);
	mem(&amplifierCoeff, 0);
	mem(&integralTermLimit, 0);
	mem(&channelIndex, 0);
	mem(&averaging, 0);
	mem(&dsIndexes, 0);
	mem(&defaultAmbientTemp, 0);
	mem(&regulationMode, 0);
	mem(&calibration, 0);
	mem(&distillExtraPower, 0);
	mem(&distillTempWindow, 0);
	mem(&rampStepLimit, 0);
	mem(&gpioMode, 0);
	mem(&cjc, 0);

	pinMode(PIN_GPIO, GPIO_DIRECTION(gpioMode));
	digitalWrite(PIN_GPIO, GPIO_VALUE(gpioMode));

#ifdef DEBUG
	cfOut();
#endif
	myPID.SetSampleTime(TIMING);                          //Time-related stuff init
	myPID.SetOutputLimits(PWM_MIN, PWM_MAX);
	Timer1.initialize(((unsigned long)TIMING) * 1000UL);
	//Timer1.attachInterrupt(isr1);
	Timer1.pwm(PIN_PWM, 0);
	MsTimer2::set(1, isr2);
	MsTimer2::start();
	encoder = new ClickEncoder(PIN_ENCODER1, PIN_ENCODER2, PIN_BUTTON, ENCODER_STEPS, 0);   //Encoder init
																							//turn the PID on
	myPID.SetMode(AUTOMATIC);
	thermocouple.begin(PIN_SS);
#ifndef NO_DS
	setup_ds();                                          //Setup OneWire sensors
#endif                                          
	lcd.clear();
	lcd.setCursor(LEFT_COLUMN_OFFSET, 0);
	lcd.print("S:");                                       //Print basic symbols on the screen
	lcd.setCursor(LEFT_COLUMN_OFFSET, 1);
	lcd.print("R:");
	lcd.setCursor(RIGHT_COLUMN_OFFSET, 1);
	lcd.print("P:");
	lcd.setCursor(RIGHT_COLUMN_OFFSET, 0);
	lcd.print("M:");
	asm("sei");                                         // Remember to turn the interrupts back on
}

void sendLog()
{
	char s[7][7];
	dtostrf(prevSetpoint, 6, 2, s[0]);    //Build up the strings. TODO: consider improving execution speed.
	dtostrf(prevInputValue, 6, 2, s[1]);
	dtostrf(ambientTemp, 6, 2, s[2]);
	sprintf(s[3], "%i", prevPower);
	sprintf(s[4], "%c%1u", ConvertMode(regulationMode), prevChannelIndex);
	dtostrf(dsChannelTemp, 6, 2, s[5]);
	sprintf(s[6], "%i", digitalRead(PIN_GPIO));
	Serial.print(">L:");
	for (uint8_t i = 0; i < arraySize(s); i++)
	{
		sendSpace();
		Serial.print(s[i]);
	}
	Serial.println();
}

void savePersistent()
{
	EEPROM.put(SETPOINT_ADDR, Setpoint);   // Load some parameters (which can be changed by means of display-encoder interface) into EEPROM
	EEPROM.put(CHANNEL_ADDR, channelIndex);
	eepromCurrentAddr = MODE_ADDR;
	mem(&regulationMode, true);
	mem(&calibration, true);
}

void loop() {
	// put your main code here, to run repeatedly:
	serial();                             // First, check for serial data
	if (counter750ms > 700)                                    //OneWire communications
	{
		if (cursorType == CURSOR_NONE)       // And for polling sensors (worst-case period of conversion: 750ms, may actually reach 600-650ms),
		{                                           // don't poll when cursor is on (for the interface not to freeze)
			readds();
			counter750ms = 0;                                   //Don't forget to reset the virtual timer
		}
	}
	if (max6675Ready)                                 //Read inputs' values
	{
		read_input();
		max6675Ready = false;
	}
	setPID();
	if (updateDisplay && (counterForDisplay < 180))                        //Display logic (printing part, cursor part has been moved to the ISR). This condition was intended to synchronize interrupt and main loop parts
	{                                                     //  for the cursor not to jump around occasionally. It works somehow.
		display2();
		curpos();                           // Move cursor back to it's position immediately to prevent jumping to the last filled field
		updateDisplay = false;                           // Reset the flag
	}					   
	savePersistent();
	if (logging[0] && logging[1])        // Check whether logging is enabled and it's time to send the info
	{
		sendLog();
		logging[1] = false;             //Reset the timing flag
	}
	setcursor();                          // Encapsulated for convenience, changes cursor according to 'cursorType' volatile variable
	curpos();
}