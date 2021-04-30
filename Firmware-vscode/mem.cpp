#include "thermo.h"

#define FIRST_RUN_BYTE 0x03

EEMEM uint8_t ee_first_run;

EEMEM float ee_setpoint;
EEMEM uint8_t ee_pid_mode;
EEMEM float ee_calibration[arraySize(calibration)];
EEMEM float ee_K[arraySize(K)][arraySize(K[0])];
EEMEM float ee_amplifier_coef;
EEMEM float ee_integral_limit;
EEMEM uint8_t ee_channel;
EEMEM uint8_t ee_averaging[arraySize(averaging)];
EEMEM uint8_t ee_ds_indexes[arraySize(dsIndexes)];
EEMEM float ee_default_ambient;
EEMEM float ee_distillation_bias;
EEMEM float ee_distillation_window;
EEMEM float ee_ramp_step;
EEMEM uint16_t ee_gpio;
EEMEM uint8_t ee_cjc;
EEMEM uint8_t ee_cooler_enable[arraySize(enableCooler)];

void mem_load_array_f(float* src, float* dst, uint8_t l)
{
	for (uint8_t i = 0; i < l; i++)
	{
		*dst++ = eeprom_read_float(src++);
	}
}

void mem_save_array_f(float* dst, float* src, uint8_t l)
{
	for (uint8_t i = 0; i < l; i++)
	{
		eeprom_update_float(dst++, *src++);
	}
}

void mem_save_array_b(uint8_t* dst, bool* src, uint8_t l)
{
	for (uint8_t i = 0; i < l; i++)
	{
		eeprom_update_byte(dst++, *src++);
	}
}

void mem_load_array_b(uint8_t* src, bool* dst, uint8_t l)
{
	for (uint8_t i = 0; i < l; i++)
	{
		*dst++ = eeprom_read_byte(src++);
	}
}

void mem_save()
{
	mem_save_persistent();
	for (uint8_t i = 0; i < arraySize(ee_K); i++)
	{
		mem_save_array_f(ee_K[i], K[i], arraySize(ee_K[i]));
	}
	eeprom_update_float(&ee_amplifier_coef, amplifierCoeff);
	eeprom_update_float(&ee_integral_limit, integralTermLimit);
	mem_save_array_b(ee_averaging, averaging, arraySize(ee_averaging));
	for (uint8_t i = 0; i < arraySize(ee_ds_indexes); i++)
	{
		eeprom_update_byte(&(ee_ds_indexes[i]), dsIndexes[i]);
	}
	eeprom_update_float(&ee_default_ambient, defaultAmbientTemp);
	eeprom_update_float(&ee_distillation_bias, distillExtraPower);
	eeprom_update_float(&ee_distillation_window, distillTempWindow);
	eeprom_update_float(&ee_ramp_step, rampStepLimit);
	eeprom_update_word(&ee_gpio, gpio_get_output_register());
	eeprom_update_byte(&ee_cjc, cjc);
	mem_save_array_b(ee_cooler_enable, enableCooler, arraySize(ee_cooler_enable));
}

void mem_load()
{
	Setpoint = eeprom_read_float(&ee_setpoint);
	regulationMode = eeprom_read_byte(&ee_pid_mode);
	mem_load_array_f(ee_calibration, calibration, arraySize(calibration));
	for (uint8_t i = 0; i < arraySize(ee_K); i++)
	{
		mem_load_array_f(ee_K[i], K[i], arraySize(ee_K[i]));
	}
	amplifierCoeff = eeprom_read_float(&ee_amplifier_coef);
	integralTermLimit = eeprom_read_float(&ee_integral_limit);
	channelIndex = eeprom_read_byte(&ee_channel);
	mem_load_array_b(ee_averaging, averaging, arraySize(ee_averaging));
	for (uint8_t i = 0; i < arraySize(ee_ds_indexes); i++)
	{
		dsIndexes[i] = eeprom_read_byte(&(ee_ds_indexes[i]));
	}
	defaultAmbientTemp = eeprom_read_float(&ee_default_ambient);
	distillExtraPower = eeprom_read_float(&ee_distillation_bias);
	distillTempWindow = eeprom_read_float(&ee_distillation_window);
	rampStepLimit = eeprom_read_float(&ee_ramp_step);
	gpio_write_all(eeprom_read_word(&ee_gpio));
	cjc = eeprom_read_byte(&ee_cjc);
	mem_load_array_b(ee_cooler_enable, enableCooler, arraySize(ee_cooler_enable));
}

void mem_save_persistent()
{
	eeprom_update_float(&ee_setpoint, Setpoint);
	eeprom_update_byte(&ee_pid_mode, regulationMode);
	eeprom_update_byte(&ee_channel, channelIndex);
	mem_save_array_f(ee_calibration, calibration, arraySize(calibration));
}

bool mem_get_first_run()
{
	return eeprom_read_byte(&ee_first_run) != FIRST_RUN_BYTE;
}

void mem_set_first_run()
{
	eeprom_write_byte(&ee_first_run, FIRST_RUN_BYTE);
}