#include "thermo.h"

uint16_t eepromCurrentAddr = 0;                                                             // EEPROM address buffer

template<typename T> void mem(T* variable, bool wrt = false);
template<typename T> void mem(T* variable, bool wrt = false)    //Function template for working with EEPROM memory
{
	if (wrt) { EEPROM.put(eepromCurrentAddr, *variable); }
	else { EEPROM.get(eepromCurrentAddr, *variable); };
	eepromCurrentAddr += sizeof(*variable);
}

void mem_save_persistent()
{
	EEPROM.put(SETPOINT_ADDR, Setpoint);   // Load some parameters (which can be changed by means of display-encoder interface) into EEPROM
	EEPROM.put(CHANNEL_ADDR, channelIndex);
	eepromCurrentAddr = MODE_ADDR;
	mem(&regulationMode, true);
	mem(&calibration, true);
}

void mem_rw(bool wrt)
{
	mem(&Setpoint, wrt);
	mem(&K, wrt);
	mem(&amplifierCoeff, wrt);
	mem(&integralTermLimit, wrt);
	mem(&channelIndex, wrt);
	mem(&averaging, wrt);
	mem(&dsIndexes, wrt);
	mem(&defaultAmbientTemp, wrt);
	mem(&regulationMode, wrt);
	mem(&calibration, wrt);
	mem(&distillExtraPower, wrt);
	mem(&distillTempWindow, wrt);
	mem(&rampStepLimit, wrt);
	uint16_t io;
	mem(&io, wrt);
	gpio_write_all(io);
	mem(&cjc, wrt);
	mem(&enableCooler, wrt);
	eepromCurrentAddr = eepromStart;
}