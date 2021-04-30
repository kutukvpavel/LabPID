#include "thermo.h"

bool lineEndingPresent = false;
uint8_t serialBufferIndex = 0;                                                            // Index used for serial data transferring to virtual buffer (serialBuffer)
char serialBuffer[16];                                                  // Virtual input buffer for serial interface

void sendSpace();
float convert(float def);
void cmd();
void cfOut();
void serial_print_float(float val, uint8_t precision, bool ln = false, uint8_t width = 1);

void sendSpace()
{
	Serial.print(' ');
}

void serial_process()
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
		Serial.println(convert_regulation_mode(regulationMode));
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
		Serial.print(">S:");
		for (uint8_t k = 0; k < arraySize(dsAddresses); k++)
		{
			sendSpace();
			for (short j = 0; j < 8; j++)
			{
				Serial.print(dsAddresses[k][j]);
			}
		}
		Serial.println();
		break;
	case 'W':                                     // W - set power (only for manual mode)
		if (regulationMode != MODE_MANUAL) break;
		power = static_cast<int8_t>(convert(power));
		check_power();
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
	{
		if (!gpioOK)
		{
			Serial.println(">E:GPIO");
			break;
		}
		uint8_t data = static_cast<uint8_t>(convert(0xFF));
		if (data != 0xFF_ui8) {
		gpio_write(data);
		EEPROM.put(GPIO_ADDR, gpio_get_output_register());
#ifdef DEBUG
		Serial.println(gpioMode, BIN);
#endif
		}
		Serial.print(">G:");
		Serial.println(gpio_read_all());
		break;
	}
	case 'J':
		cjc = (convert(cjc) > 0);
		EEPROM.put(CJC_ADDR, cjc);
#ifdef DEBUG
		Serial.println(cjc);
#endif
		break;
	case 'N':
		enableCooler[channelIndex] = (convert(enableCooler[channelIndex]) != 0) ? true : false;
#ifdef DEBUG
		Serial.print(static_cast<uint8_t>(enableCooler[0])); Serial.print(" ");
		Serial.print(static_cast<uint8_t>(enableCooler[1])); Serial.print(" ");
		Serial.println(static_cast<uint8_t>(enableCooler[2]));
#endif
		break;
	default: break;
	}
}

void cfOut()                              //Outputs all the current technical data
{
	Serial.println(">D:");
	serial_print_float(Input, 2);
	serial_print_float(ambientTemp, 2);
	Serial.print(convert_regulation_mode(regulationMode)); Serial.println(static_cast<int>(channelIndex));
	for (uint8_t i = 0; i < arraySize(K[0]); ++i)
	{
		for (uint8_t j = 0; j < (arraySize(K) - 1_ui8); ++j)
		{
			serial_print_float(K[j][i], 4);
		}
		serial_print_float(K[arraySize(K) - 1_ui8][i], 4, true);
	}
	serial_print_float(integralTermLimit, 4);
	serial_print_float(distillExtraPower, 4);
	serial_print_float(amplifierCoeff, 4, true);
	for (uint8_t i = 0; i < (arraySize(calibration) - 1_ui8); i++)
	{
		serial_print_float(calibration[i], 4);
	}
	serial_print_float(calibration[arraySize(calibration) - 1_ui8], 4, true);
	for (uint8_t i = 0; i < (arraySize(averaging) - 1_ui8); i++)
	{
		serial_print_float(averaging[i], 4);
	}
	serial_print_float(averaging[arraySize(averaging) - 1_ui8], 4, true);
	serial_print_float(Setpoint, 2);
	serial_print_float(defaultAmbientTemp, 2);
	serial_print_float(distillTempWindow, 2, true);
	serial_print_float(rampStepLimit, 2);
	Serial.print(gpio_read_all()); sendSpace();
	Serial.println(cjc);
	for (uint8_t i = 0; i < (CHANNEL_COUNT - 1_ui8); i++)
	{
		Serial.print(static_cast<uint8_t>(enableCooler[i])); sendSpace();
	}
	Serial.println(static_cast<uint8_t>(enableCooler[CHANNEL_COUNT - 1_ui8]));
}

void serial_print_float(float val, uint8_t precision, bool ln, uint8_t width)
{
	char s[16];
	dtostrf(val, 1, precision, s);
	if (ln)
	{
		Serial.println(s);
	}
	else
	{
		Serial.print(s); sendSpace();
	}
}

void serial_send_log()
{
	Serial.print(">L:");
	serial_print_float(prevSetpoint, 2, false, 6);    //Build up the strings. TODO: consider improving execution speed.
	serial_print_float(prevInputValue, 2, false, 6);
	serial_print_float(ambientTemp, 2, false, 6);
	Serial.print(static_cast<int>(prevPower)); sendSpace();
	Serial.print(convert_regulation_mode(regulationMode)); sendSpace();
	Serial.print(static_cast<int>(prevChannelIndex)); sendSpace();
	serial_print_float(dsChannelTemp, 2, false, 6);
	Serial.println(static_cast<long>(gpio_read_all()));
}