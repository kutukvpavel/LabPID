/*  *
*  NB! Some of the libraries used (e.g. PID) have been modified! The code is valid only if modified libs are used.
*
*  Current major TODO:
*  Improve interface performance
*  _____________________________________
*  Developed by Kutukov Pavel, 2016-2020.
*  This software was developed to fulfill the author's personal needs only and is provided strictly as-is.
*/
#include "thermo.h"

#pragma region Globals
const uint8_t eepromStart = EEPROM_START;                                         // Starting EEPROM address  TODO: consider changing it to dynamic var                                                              
const float mult[5] = { 100, 10 , 1, 0.1, 0.01 };                 // Multipliers for building up numbers
uint8_t channelIndex = CHANNEL_ADC;           // Current input used: 0 - ADC, 1 - SPI, 2 - DS18B20
uint8_t prevChannelIndex = 0xff;              // Backup used to check for changes, thus determining if display update is needed
uint8_t regulationMode = MODE_NORMAL;         // Regulation mode flag: 0 - normal, 1 - aggressive, 2 - distillation, 3 - manual
uint8_t dsAddresses[2][8];                                           // Address temporary storage, introduced to try to speed up OneWire-related actions. 0 - ambient, 1 - channel
uint8_t gpioMode = 0;                        //0bXXY where X denotes direction (input = 0, out = 1, pullup = 2) and Y stands for the state 
float prevSetpoint = -1;                                                     // Backup needed for display update necessity check...
float prevInputValue = -1;                                                   //  Same, more to come...
float Setpoint, Input, Output;                                       // PID main values
float integralTermLimit = 0.35;                                                // Integral term bound (in percents)
float K[CHANNEL_COUNT][2] = { { 18, 35 },{ 0.006, 0.19 },{ 15, 16 } };                       // Current regulation constants
float amplifierCoeff = INIT_AMP_COEFF;                                                    // Coefficient used for working with external amplifier
float calibration[CHANNEL_COUNT] = { 0, 0, 0 };                                        // Input calibration values
float distillExtraPower = 0;                                                          // Bias for distillation
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
bool logging[] = { 0, 0 };                                                 // Logging enable flags
bool condition[4] = { 0, 0, 0, 0 };                                          // Flags for error states behavior control {Safety error, DS chain: <= 1 sensor, DS chain: 0 sensors, DS chain changed}
bool cjc = false;
ClickEncoder *encoder;                                                // Encoder object 
PID myPID(&Input, &Output, &Setpoint, K[0][0], K[1][0], K[2][0], DIRECT);    // PID Object
#pragma endregion

bool max6675Ready = true;                                                     // Flag of readiness for reading (for MAX6675 proper timing: >400ms)
bool updateDisplay = true;                                                        // Flag for display update enable
uint16_t counter750ms = 0;                                                             // Virtual timer counter (750ms)
uint16_t counter500ms = 0;                                                            // Virtual timer counter (500ms)
uint16_t counterForDisplay = 0;                                                           // Virtual timer counter (display)

MAX6675 thermocouple;                                  // MAX6675 thermocouple amplifier object
Average<float> averageContainer(3);                                                // Averaging container

#pragma region ISRs
ISR(TIMER1_OVF_vect)                               // 1 sec timer routine
{
	myPID.Compute();                            // For PID computations, safety check and output updating
	check_safety();
	Timer1.setPwmDuty(PIN_PWM, static_cast<uint16_t>(Output));
	logging[1] = true;                            //Also enables logging flag for the log to be periodically created (with 1 sec period, of course)
}

void isr_fast(void)                               // 1 ms timer routine
{
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
	if (power < 0)
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

#pragma region Main logic

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
		in = dsChannelTemp;   // Updated in "ds_read"
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
		Output = static_cast<float>(PWM_MAX * (power < 0 ? 0 : static_cast<long>(power))) / 100; //Negative power values are reserved yet
	}
	else
	{
		power = static_cast<int8_t>((Output * 100) / PWM_MAX);             //Else - count power value based on output
	}
}

#pragma endregion

void setup() {
	asm("cli");   // No interrupts while we aren't ready yet
				  // put your setup code here, to run once:
	
	lcd_init();
	lcd_draw_boot_screen();
	Serial.begin(9600);                                     // Initialize serial communications with the PC
	SPI.begin();                                            // Init hardware SPI for max6675-compatible devices
	uint8_t t = 0;

	pinMode(PIN_BUTTON, INPUT_PULLUP);                          //Provide ability to reset to factory defaults by holding encoder's button while booting up
	
	t = EEPROM.read(0);                                           //Check if this is the first run and we'll have to fill the EEPROM up too
	if ((t != 0x01) || (!digitalRead(PIN_BUTTON)))
	{
		lcd_draw_reset_screen();
		delay(500);
		for (uint16_t i = 0; i < EEPROM.length(); ++i) EEPROM.write(i, 0);
		Setpoint = 18.0f;
		mem_rw(true);
		EEPROM.write(0, 0x01);                                //Set the 'filled' flag on
		static_assert(EEPROM_START > 0, "Address 0 is reserved for the first run flag!");
		
#ifdef DEBUG
		Serial.println("EEPROM refilled.");
#endif
		lcd_draw_boot_screen();
	}

	analogReference(EXTERNAL);                              // Initialize ADC stuff
	pinMode(PIN_INPUT, INPUT);
	pinMode(PIN_PWM, OUTPUT);
	digitalWrite(PIN_PWM, LOW);

	mem_rw(false);

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
	MsTimer2::set(1, isr_fast);
	MsTimer2::start();
	encoder = new ClickEncoder(PIN_ENCODER1, PIN_ENCODER2, PIN_BUTTON, ENCODER_STEPS, 0);   //Encoder init
																							//turn the PID on
	myPID.SetMode(AUTOMATIC);
	thermocouple.begin(PIN_SS);
#ifndef NO_DS
	ds_init();                                          //Setup OneWire sensors
#endif       

	lcd_draw_interface();
	
	asm("sei");                                         // Remember to turn the interrupts back on
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