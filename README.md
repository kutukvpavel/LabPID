# LabPID
PID temperature controller with GPIO

# Basic Hardware:
 - Arduino Nano (or any atmega-328 device with a compatible pinout, or even a bare atmega328 flashed with optiboot)
 - MAX6675 SPI integrated K-type thermocouple amplifier + ADC + cold junction compensation
 - Up to 8 DS18B20 sensors (though only 2 can be recorded simultaneously)
 - Analog input (linear 0-5V with programmable Volt/Kelvin coefficient) uses internal atmega ADC (10-bit) and an external TL431 precision reference shunt regulator
 - 16x2 ALCD and a single encoder (with an integrated button) for offline operation
 - Two PWM outputs for a heater (1 second period) and an optional cooler (faster PWM, suitable for BLDC computer fans, for example)
 - PCA9555-based GPIOs: 4 inputs, 12 outputs by default (second PCA9555 can be added and GPIO map can be altered, see firmware code)
 - 12V and 5V_OneWire power rails have fuse monitoring

# PCB:

The \*.lay PCB file contains an old prototyping PCB with mains power supply. Only stuff under /new/ is maintained. This folder contains Proteus v8 design (schematic + PCB) as well as latest Gerber files (supported by JLCPCB). In addition to peripherals mentioned above, this design incorporates following stuff: 
 - Power supply from a PC or an external +12V & +5V power supply (through a Molex 4-pin connector, BEWARE: +5 AND +12 LINES ARE MIXED UP, ALTER THE PLUG's PINOUT ACCORDINGLY!)
 - 4 universal inputs with optoisolators that can be configured by jumpers to accept mains 220Vac or 5-24Vdc
 - 2 optorelays in SIP package for PWM outputs
 - 2 NRP-12 relays GPIO outputs
 - Up to 3 NRP-12 or SIP optorelays (universal layout) for 3 more outputs
 - ULN2003A open-collector outputs for the rest of them
 - Basic buffer/amplifier circuitry for analog input
 - 12V input, 5V input and 5V_OneWire output are protected by fuses, 12V and 5V_OneWire have fuse indicator LEDs and are monitored by the firmware
 - On-board DS18B20 for ambient temperature measurement
 - A big clunky DP-DT toggle switch mounting hole
 - Thermocouple low-pass filter
 - PCB is separated into to parts (they are connected by some tiny traces so that PCB manufactirers don't categorize them as different designs): main board and LCB board, they are then connected with some right-angle interconnects (2 rows are used for mechanical durability as well as ensuring good electrical contact)

# Software:
 - .NET 4.0 (XP SP3 compatible)
 - Uses standard arduino-like virtual COM port (commands are intended to be human-readable to enable simple operation through UART in case the software is unavailable)
 - Adjust all settings (PID coefficients, calibration values etc)
 - Real-time temperature plots (multiple channels, currently 3: selected input \[thermocouple, linear input, or "second ds18b20"\], "ambient" temperature ("first ds18b20") \[fixed\], "second ds18b20" \[fixed\])
 - Execute predefined temperature profiles and control GPIOs
 - Named pipe broadcast of events during profile execution for some interoperability
 
# Building:
 - Firmware-VSCode workspace can be opened with VSCode with Arduino extension installed. DO NOT change existing folder structure of the workspace (/src/ is the only subdirectory name included in compilation by arduino-builder, all other subfolders will be ignored!). The project compiles under Arduino 1.8.5 (and, probably, later). Almost all non-standard libraries required have been modified, so don't try to update them without merging the modifications, all non-standard libraries are included under /src/.
 - You'll almost certainly have to reconfigure C/C++ intellisense include paths based on your arduino installation. It's a known limitation of Arduino for VSCode.

