#include "thermo.h"

//Modified, but not imported into the cppproj (one-time changes)
#include "src\OneWire\OneWire.h"
#include "src\DallasTemperature\DallasTemperature.h"

#define PIN_ONEWIRE 8                 //OneWire bus pin

OneWire oneWire(PIN_ONEWIRE);                                        // OneWire bus object
DallasTemperature dsSensors(&oneWire);                                  // DS18B20 Object

void ds_init()
{
	dsSensors.begin();                            //Init OneWire bus and sensors
	int dsc = dsSensors.getDeviceCount();         //Get the number of sensors
	condition[1] = dsc == 1;
	condition[2] = dsc == 0;
	if (condition[2]) return;
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
		dsSensors.setWaitForConversion(false);
		dsSensors.requestTemperatures();
}

void ds_read()         // One-wire devices poll routine
{
	if (condition[2])                          //If there are no sensors then set default ambient temperature
	{
		ambientTemp = defaultAmbientTemp;
	}
	else
	{
		ambientTemp = dsSensors.getTempC(dsAddresses[0]);
		if (condition[1]) //If only 1 sensor give the user an ooportunity to monitor it as a channel
		{
			dsChannelTemp = ambientTemp;
		}
		else
		{
			if (channelIndex == CHANNEL_DS18B20)                           // Poll second sensor only in mode 2
			{
				dsChannelTemp = dsSensors.getTempC(dsAddresses[1]);
			}
		}
		dsSensors.requestTemperatures();
	}
}