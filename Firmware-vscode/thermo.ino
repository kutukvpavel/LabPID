/*
* NB! Some of the libraries used (e.g. PID) have been modified! The code is valid only if modified libs are used.
*
*  Current major TODO:
*  None
*  _____________________________________
*  Developed by Kutukov Pavel, 2016-2021.
*  This software was developed to fulfill the author's personal needs only and is provided strictly as-is.
*/

#include "thermo.h"

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <SPI.h>

//Modified, but not imported into the cppproj (one-time changes)
#include "src\MAX6675-master\src\max6675.h" //SPI thermocouple amplifier
#include "src\Average-master\Average.h"
#include "src\Shared Libraries\OptPin.h"

#pragma region Definitions

#define AVERAGING_WINDOW 3_ui8
#define PIN_COOLER 3
#define PIN_INPUT A7                    //ADC input
#define PIN_PWM 9                       //Pin for PWM PID output
#define PIN_ENCODER1 A0                  //Encoder pins
#define PIN_ENCODER2 A1
#define PIN_BUTTON A2
#define PIN_FUSE_SENSE A6
#define PIN_SS 10
#define PWM_MAX 62499u
#define PWM_MIN -255
#define ENCODER_STEPS 4_ui8

#pragma endregion

#pragma region Variables

const char lcd_booting_message[] = "Booting...";
const char lcd_resetting_message[] = "Resetting...";

#ifdef DEBUG_STARTUP
const char lcd_spi_message[] = "SPI";
const char lcd_serial_message[] = "Serial";
const char lcd_button_message[] = "Button";
const char lcd_gpio_message[] = "GPIO";
const char lcd_eeprom_message[] = "EEPROM";
const char lcd_pid_message[] = "PID";
const char lcd_max6675_message[] = "MAX6675";
const char lcd_ds_message[] = "DS18B20";
const char lcd_adc_message[] = "ADC";
#endif

volatile uint8_t counterForLog = 0;
volatile uint32_t counterForDisplay = 0;                                                           // Virtual timer counter (display)
volatile bool updateDisplay = true;                                                        // Flag for display update enable

uint32_t counter750ms = 0;                                                             // Virtual timer counter (750ms)
uint32_t counter500ms = 0;                                                           // Virtual timer counter (500ms)
float averagingBuffer[AVERAGING_WINDOW];

MAX6675 thermocouple;                                  // MAX6675 thermocouple amplifier object
Average<float> averagingObject(AVERAGING_WINDOW, averagingBuffer);                                                // Averaging container

#pragma endregion

#pragma region ISRs

ISR(TIMER2_OVF_vect) //16mS
{
#ifdef DEBUG_TIMERS
	Serial.write('2');
#endif
	uint32_t now = millis();
	if (static_cast<uint16_t>(now - counterForDisplay) > 199u)
	{
		lcd_process_fast();                          // Display update logic
		updateDisplay = 1;
		counterForDisplay = now;
	}
}

ISR(TIMER1_OVF_vect)                               // 1 sec timer routine
{
#ifdef DEBUG_TIMERS
	Serial.write('1');
#endif
	myPID.Compute();                            // For PID computations, safety check and output updating
	check_safety();
	timers_set_heater_duty((Output > 0) ? static_cast<uint16_t>(Output) : 0);
	timers_set_cooler_duty(static_cast<uint8_t>(
		(Output < 0 && enableCooler[channelIndex]) ? -Output : 0));
}

ISR(TIMER0_COMPA_vect)
{
#ifdef DEBUG_TIMERS
	Serial.write('0');
#endif    
	encoder->service();
}
#pragma endregion

#pragma region Common functions

char convert_regulation_mode(char src)  //Routine for converting user-friendly into system-recognized values of current mode (var. "regulationMode") and vice-versa
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

#pragma region Checks

void check_power()
{
	if (power < 0)
	{
		if (!enableCooler[channelIndex]) power = 0;
		else if (power < -100) power = -100;
	}
	else
	{
		if (power > 100) power = 100;
	}
}

void check_safety() // Check if something goes wrong (real temp rises more than 20C above the setpoint, ambient temperature rises above 60C, ds18b20 failed, no ds18b20 when using mode #2, broken thermocouple)
{
	bool fuse = OPTanalogRead(PIN_FUSE_SENSE) < 512u;
	condition[0] = (
		((Input - Setpoint > 20) && (Output > 0)) ||
		(ambientTemp > 60) ||
		(ambientTemp < -25) ||
		(dsChannelTemp < -25) ||
		(condition[2] && (channelIndex == CHANNEL_DS18B20)) ||
		(!(Input == Input)) ||
		fuse);
	if (condition[0])
	{
		if (errorStatusDelay > ERROR_TIMEOUT)
		{
			if (myPID.GetMode() == AUTOMATIC)
			{
				myPID.SetMode(MANUAL);
				Serial.println(fuse ? ">E:FUSE" : ">E!");
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

#pragma region Main logic

void read_input()                           //Update input according to the mode being used
{
	float in = 0;
	switch (channelIndex)
	{
	case CHANNEL_ADC:                              // 0 - external amplifier (ADC input)
		in = OPTanalogRead(PIN_INPUT) * amplifierCoeff + (cjc ? ambientTemp : 0);
		break;
	case CHANNEL_MAX6675:                              // 1 - max6675 (SPI)
		in = static_cast<float>(thermocouple.readCelsius());
		break;
	case CHANNEL_DS18B20:                              // 2 - ds18b20  (1-wire)
		in = dsChannelTemp;   // Updated in "ds_read"
		break;
	default:
		break;
	}
	in += calibration[channelIndex];          // Apply calibration value
	if (averaging[channelIndex])
	{                         
		Input = averagingObject.rolling(in); // Apply averaging if enabled for current mode
	}
	else
	{
		Input = in;
	}
}

void pid_process()
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
	if (lcd_is_editing() && (channelIndex == CHANNEL_DS18B20))            //Disable PID regulating if input is not updated
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
		if (power < 0)
		{
			Output = static_cast<float>(-PWM_MIN) * 0.01f * static_cast<float>(enableCooler[channelIndex] ? power : 0);
		}
		else
		{
			Output = static_cast<float>(PWM_MAX) * 0.01f * static_cast<float>(power);
		}
	}
	else
	{
		power = static_cast<int8_t>((Output * 100) / (Output < 0 ? -PWM_MIN : PWM_MAX));             //Else - count power value based on output
	}
}

#pragma endregion

void setup() {
	lcd_init();
	#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_serial_message);
#else
	lcd_draw_message(lcd_booting_message);
#endif

	serial_init();
#ifdef DEBUG_STARTUP
	Serial.println("LabPID");
	lcd_draw_message(lcd_spi_message);
#endif
	SPI.begin();                                            // Init hardware SPI for max6675-compatible devices
	uint8_t t = 0;

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_button_message);
#endif
	pinMode(PIN_BUTTON, INPUT_PULLUP);                          //Provide ability to reset to factory defaults by holding encoder's button while booting up
	
	//Check if this is the first run and we'll have to fill the EEPROM up too
	if (mem_get_first_run() || (!digitalRead(PIN_BUTTON)))
	{
		lcd_draw_message(lcd_resetting_message);
		delay(1000);
#ifdef DEBUG_STARTUP
		lcd_draw_message(lcd_gpio_message);
#endif
		gpio_init();
		gpio_write_all(0u);
		mem_save(); //Defaults have already been loaded in the BSS section
		mem_set_first_run();
#ifdef DEBUG
		Serial.println("EEPROM refilled.");
#endif
		lcd_draw_message(lcd_booting_message);
	}

	pinMode(PIN_FUSE_SENSE, INPUT);
	pinMode(PIN_INPUT, INPUT);
	pinMode(PIN_PWM, OUTPUT);
	pinMode(PIN_COOLER, OUTPUT);
	digitalWrite(PIN_PWM, LOW);
	digitalWrite(PIN_COOLER, LOW);

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_adc_message);
#endif
	OPTanalogReference(EXTERNAL);                              // Initialize ADC stuff

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_gpio_message);
#endif
	gpio_init();

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_eeprom_message);
#endif
	mem_load();
#ifdef DEBUG
	cfOut();
#endif

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_pid_message);
#endif
	myPID.SetSampleTime(TIMING);                          //Time-related stuff init
	myPID.SetOutputLimits(PWM_MIN, PWM_MAX);
	asm("cli");
	timers_init();
	encoder = new ClickEncoder(PIN_ENCODER1, PIN_ENCODER2, PIN_BUTTON, ENCODER_STEPS, 0);   //Encoder init
	//turn the PID on
	myPID.SetMode(AUTOMATIC);
	asm("sei");

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_max6675_message);
#endif
	thermocouple.begin(PIN_SS);

#ifndef NO_DS
#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_ds_message);
#endif
	ds_init();                                          //Setup OneWire sensors
#endif       

	lcd_draw_interface();
}

void loop() {
	// put your main code here, to run repeatedly:
	serial_process();                             // First, check for serial data
	uint32_t now = millis();
	if (static_cast<uint16_t>(now - counter750ms) > 700u)                                    //OneWire communications
	{
		ds_read();
		counter750ms = now;                                   //Don't forget to reset the virtual timer
	}
	if (static_cast<uint16_t>(now - counter500ms) > 499u)                                 //Read inputs' values
	{
		read_input();
		counter500ms = now;
	}
	pid_process();
	if (updateDisplay /*&& ((now - counterForDisplay) < 180)*/)                        //Display logic (printing part, cursor part has been moved to the ISR). This condition was intended to synchronize interrupt and main loop parts
	{                                                     //  for the cursor not to jump around occasionally. It works somehow.
#ifdef DEBUG_DISPLAY
		Serial.println("UPD");
#endif
		lcd_process_slow();                           
		updateDisplay = false;                           // Reset the flag
	}		
	lcd_process_cursor_position();		// Move cursor back to it's position immediately to prevent jumping to the last filled field	
	lcd_process_cursor_type();    
	mem_save_persistent();
}