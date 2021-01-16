#include "thermo.h"

bool lineEndingPresent = false;
uint8_t serialBufferIndex = 0;                                                            // Index used for serial data transferring to virtual buffer (serialBuffer)
char serialBuffer[16];                                                  // Virtual input buffer for serial interface

void sendSpace();
float convert(float def);
void cmd();
void cfOut();

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

void cfOut()                              //Outputs all the current technical data
{
	Serial.println(">D:");
	Serial.print(Input); sendSpace();
	Serial.print(ambientTemp); sendSpace();
	Serial.print(convert_regulation_mode(regulationMode)); Serial.println(static_cast<int>(channelIndex));
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

void serial_send_log()
{
	char s[7][7];
	dtostrf(prevSetpoint, 6, 2, s[0]);    //Build up the strings. TODO: consider improving execution speed.
	dtostrf(prevInputValue, 6, 2, s[1]);
	dtostrf(ambientTemp, 6, 2, s[2]);
	sprintf(s[3], "%i", prevPower);
	sprintf(s[4], "%c%1u", convert_regulation_mode(regulationMode), prevChannelIndex);
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