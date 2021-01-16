# LabPID
PID temperature controller for laboratory use

Hardware:
 - Arduino Nano (or any atmega-328 device with a compatible pinout, or even a bare atmega328 flashed with optiboot)
 - MAX6675 SPI integrated K-type thermocouple amplifier + ADC + cold junction compensation
 - Up to 8 DS18B20 sensors (though only 2 can be recorded simultaneously)
 - Analog input (linear 0-5V with programmable Volt/Kelvin coefficient) uses internal atmega ADC (10-bit) and an external TL431 precision shunt regulator
 - 16x2 ALCD
 - Single encoder with a button for offline operation
 - Some GPIOs for controlling auxillary equipment during execution of temperature profiles
 - Several different regulation modes (Normal, Aggressive timings, Distillation, Manual)
 - Independent power switching module required. I've used a low-power MOSFET to control an opto-isolated solid state relay (usually a triac in case of line voltages or a power FET in case of DC). Switching frequency is 1Hz with variable duty cycle (PWM). Together with a DC heater it's ideal for noise-critical applications. Usually even small heaters have thermal mass that tolerates low switching frequency very well.

Software:
 - .NET 4.0 (XP SP3 compatible)
 - Uses standard arduino-like virtual COM port (commands are intended to be human-readable to enable simple operation through UART in case the software is unavailable)
 - Adjust all settings (PID coefficients, calibration values etc)
 - Real-time temperature plots (multiple channels, currently 3: selected input \[thermocouple, linear input, or "second ds18b20"\], "ambient" temperature ("first ds18b20") \[fixed\], "second ds18b20" \[fixed\])
 - Execute predefined temperature profiles and control GPIOs
 
Building:
 - Folder "Firmware" contains deprecated VisualMicro-based Arduino project! Use Firmware-VSCode workspace instead. It can be opened with VSCode with Arduino extension installed. DO NOT remove existing folder structure of the workspace (/src/ is the only subdirectory name included in compilation by arduino-builder, all other subfolders will be ignored!). The project compiles under Arduino 1.8.5 (and, probably, later). Almost all non-standard libraries required have been modified, so don't try to update them without merging the modifications, all non-standard libraries are included under /src/.
 - You'll almost certainly have to reconfigure C/C++ intellisense include paths based on your arduino installation. It's a known limitation of Arduino for VSCode.
 
P.S. The PCB layout was created way back in 2016 using SprintLayout, and was really just a prototyping board.
