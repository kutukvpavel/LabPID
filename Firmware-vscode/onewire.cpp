#include "thermo.h"

//Modified, but not imported into the cppproj (one-time changes)
#include "src\OneWire\OneWire.h"
#include "src\DallasTemperature\DallasTemperature.h"

OneWire oneWire(PIN_ONEWIRE);                                        // OneWire bus object
DallasTemperature dsSensors(&oneWire);                                  // DS18B20 Object

void ds_init()
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

void ds_read()         // One-wire devices poll routine
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