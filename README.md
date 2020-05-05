# LabPID
PID temperature controller for laboratory use

Hardware:
 - Arduino Nano (or any atmega-328 device with a compatible pinout, or even a bare atmega328 flashed with optiboot)
 - MAX6675 SPI integrated K-type thermocouple amplifier + ADC + cold junction compensation
 - Up to 8 DS18B20 sensors
 - Analog input (linear 0-5V with programmable Volt/Kelvin coefficient) uses internal atmega ADC and an external TL431 precision shunt regulator
 - 16x2 ALCD
 - Single encoder with a button for offline operation
 - Some GPIO for controlling auxillary equipment during execution of temperature profiles
 - Several different regulation modes (Normal, Aggressive timings, Distillation, Manual)
 - Independent power switching module required. I've used a low-power MOSFET to control an opto-isolated solid state relay (usually a triac in case of line voltages or a power FET in case of DC). Switching frequency is 1Hz with variable duty cycle (PWM). Together with a DC heater it's ideal for noise-critical applications. Usually even small heaters have thermal mass that tolerates low switching frequency very well.

Software:
 - .NET 4.0 (XP SP3 compatible)
 - Uses standard arduino-like virtual COM port
 - Adjust all settings (PID coefficients, calibration values etc)
 - Real-time temperature plots (multiple channels, though this is a work in progress, and there are caveats)
 - Execute predefined temperature profiles
 
 
