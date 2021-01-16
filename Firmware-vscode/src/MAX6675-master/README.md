# MAX6675
Arduino library for interfacing with MAX6675 thermocouple amplifier. Originally from Adafruit but as they abandoned the [repository](https://github.com/adafruit/MAX6675-library), @SirUli forked and updated the repository from all current pull requests on the repository.

# Usage

## Initialization


### Wiring
There are different namings for each of the connections out there. Therefore copied from [Wikipedia](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus):

|Name in the library|Long Name|Other names|
|---|---|---|
|**SCLK**|Serial Clock|SCK, CLK|
|**CS**|Slave Select|nCS, SS, CSB, CSN, EN, nSS, STE, SYNC, SSQ|
|**MISO**|Master Input / Slave Output|SOMI, SDO (for slave devices ), DO, DOUT, SO, MRSR|

Ensure to wire this correctly.

### Regular setup
**Important** due to a specific behaviour in platformio, the SPI library needs to be included in the main file:
```cpp
#include <SPI.h>
```
See [here](https://github.com/platformio/platformio/issues/702) for the explanation.

Setup the Sensor like this:
```cpp
MAX6675 thermocouple;
```
During the setup method, you need to choose whether you initialize the Hardware SPI outside the MAX sensor or if the library should take care of it all.

#### Library takes care
Set the pins for SCLK, CS, MISO as arguments. You can additional set the optional offset for the temperature in degrees Celsius. If you want to use Fahrenheit offsets, you need to set that using setOffsetFahrenheit();
```cpp
thermocouple.begin(SCLK, CS, MISO[, OFFSET]);
```

#### Hardware SPI
[@eadf](https://github.com/eadf) added a hardware SPI mode. In this mode the SPI pins can be reused for your MAX6675 device as well as any other SPI gadget you got connected. Assumes you already have setup SPI with ```SPI.begin()```. You can additional set the optional offset for the temperature in degrees Celsius. If you want to use Fahrenheit offsets, you need to set that using setOffsetFahrenheit();
```cpp
thermocouple.begin(CS[, OFFSET]);
```

### Legacy initialization in constructor
#### SPI Setup
If the SPI is not used for anything else, then setup the Sensor like this. You can additional set the optional offset for the temperature in degrees Celsius. If you want to use Fahrenheit offsets, you need to set that using setOffsetFahrenheit();
```cpp
MAX6675 thermocouple(SCLK, CS, MISO[, OFFSET]);
```

#### Hardware SPI
[@eadf](https://github.com/eadf) added a hardware SPI mode. In this mode the SPI pins can be reused for your MAX6675 device as well as any other SPI gadget you got connected. Assumes you already have setup SPI with ```SPI.begin()```. You can additional set the optional offset for the temperature in degrees Celsius. If you want to use Fahrenheit offsets, you need to set that using setOffsetFahrenheit();
Use the single value constructor to access this mode:
```cpp
MAX6675 thermocouple(CS[, OFFSET]);
```

## Methods
Retrieves the temperature in degrees Celsius

```cpp
double readCelsius();
```

Retrieves the temperature in degrees Fahrenheit

```cpp
double readFahrenheit();
```

Configures an offset in degrees Celsius for the sensor

```cpp
void setOffsetCelsius();
```

Configures an offset in degrees Fahrenheit for the sensor

```cpp
void setOffsetFahrenheit();
```

# Continuous Integration
[![Build Status](https://travis-ci.org/SirUli/MAX6675.svg?branch=master)](https://travis-ci.org/SirUli/MAX6675)

# Release History
* 1.0 Original Release by Adafruit
* 2.0 Move to SirUli's repository with fixes and change of constructor
