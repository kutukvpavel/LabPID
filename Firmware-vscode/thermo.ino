/*
* NB! Some of the libraries used (e.g. PID) have been modified! The code is valid only if modified libs are used.
*
*  Current major TODO:
*  Improve interface performance
*  _____________________________________
*  Developed by Kutukov Pavel, 2016-2021.
*  This software was developed to fulfill the author's personal needs only and is provided strictly as-is.
*/

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <SPI.h>

//Modified, but not imported into the cppproj (one-time changes)
#include "src\MAX6675-master\src\max6675.h" //SPI thermocouple amplifier
#include "src\Average-master\Average.h"

#include "thermo.h"

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

volatile bool max6675Ready = true;                                                     // Flag of readiness for reading (for MAX6675 proper timing: >400ms)
volatile bool updateDisplay = true;                                                        // Flag for display update enable
volatile uint16_t counter750ms = 0;                                                             // Virtual timer counter (750ms)
volatile uint16_t counter500ms = 0;                                                            // Virtual timer counter (500ms)
volatile uint16_t counterForDisplay = 0;                                                           // Virtual timer counter (display)
float averagingBuffer[AVERAGING_WINDOW];

MAX6675 thermocouple;                                  // MAX6675 thermocouple amplifier object
Average<float> averagingObject(AVERAGING_WINDOW, averagingBuffer);                                                // Averaging container

#pragma endregion

#pragma region ISRs
ISR(TIMER1_OVF_vect)                               // 1 sec timer routine
{
#ifdef DEBUG_TIMERS
	Serial.write('1');
#endif
	myPID.Compute();                            // For PID computations, safety check and output updating
	check_safety();
	uint16_t duty = (Output > 0) ? static_cast<uint16_t>(Output) : 0;
	timers_set_pwm_duty(duty);
	if (enableCooler[channelIndex]) digitalWrite(PIN_COOLER, Output < 0);
	logging[1] = true;                            //Also enables logging flag for the log to be periodically created (with 1 sec period, of course)
}

ISR(TIMER2_COMPA_vect)                               // 1 ms timer routine
{
#ifdef DEBUG_TIMERS
	Serial.write('2');
#endif
	encoder->service();                         // For encoder poll
	if (counterForDisplay > 199)
	{
		lcd_process_fast();                          // Display update logic
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
	if (power < 0 && !enableCooler[channelIndex])
	{
		power = 0;
	}
	else
	{
		if (power > 100) power = 100;
	}
}

void check_safety() // Check if something goes wrong (real temp rises more than 20C above the setpoint, ambient temperature rises above 60C, ds18b20 failed, no ds18b20 when using mode #2, broken thermocouple)
{
	OPTsetADMUX(EXTERNAL, PIN_FUSE_SENSE);
	bool fuse = OPTanalogRead() < 512u;
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
		OPTsetADMUX(EXTERNAL, PIN_INPUT);
		in = OPTanalogRead() * amplifierCoeff + (cjc ? ambientTemp : 0);
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
		Output = static_cast<float>((PWM_MAX * (power < 0 ? 0 : static_cast<uint32_t>(power))) / 100);
	}
	else
	{
		power = static_cast<int8_t>((Output * 100) / PWM_MAX);             //Else - count power value based on output
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

	asm("cli");   // No interrupts while we aren't ready yet
				  // put your setup code here, to run once:
	Serial.begin(9600);                                     // Initialize serial communications with the PC
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
	digitalWrite(PIN_PWM, LOW);

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_adc_message);
#endif
	analogReference(EXTERNAL);                              // Initialize ADC stuff

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
	timers_init();
	encoder = new ClickEncoder(PIN_ENCODER1, PIN_ENCODER2, PIN_BUTTON, ENCODER_STEPS, 0);   //Encoder init
																							//turn the PID on
	myPID.SetMode(AUTOMATIC);

	asm("sei");                                         // Remember to turn the interrupts back on

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

#ifdef DEBUG_STARTUP
	lcd_draw_message(lcd_gpio_message);
#endif
	gpio_init();
}

void loop() {
	// put your main code here, to run repeatedly:
	serial_process();                             // First, check for serial data
	if (counter750ms > 700)                                    //OneWire communications
	{
		if (cursorType == CURSOR_NONE)       // And for polling sensors (worst-case period of conversion: 750ms, may actually reach 600-650ms),
		{                                           // don't poll when cursor is on (for the interface not to freeze)
			ds_read();
			counter750ms = 0;                                   //Don't forget to reset the virtual timer
		}
	}
	if (max6675Ready)                                 //Read inputs' values
	{
		read_input();
		max6675Ready = false;
	}
	pid_process();
	if (updateDisplay && (counterForDisplay < 180))                        //Display logic (printing part, cursor part has been moved to the ISR). This condition was intended to synchronize interrupt and main loop parts
	{                                                     //  for the cursor not to jump around occasionally. It works somehow.
		#ifdef DEBUG_DISPLAY
			Serial.println("UPD");
		#endif
		lcd_process_slow();
		lcd_process_cursor_position();                           // Move cursor back to it's position immediately to prevent jumping to the last filled field
		updateDisplay = false;                           // Reset the flag
	}					   
	mem_save_persistent();
	if (logging[0] && logging[1])        // Check whether logging is enabled and it's time to send the info
	{
		serial_send_log();
		logging[1] = false;             //Reset the timing flag
	}
	lcd_process_cursor_type();                          // Encapsulated for convenience, changes cursor according to 'cursorType' volatile variable
	lcd_process_cursor_position();
}