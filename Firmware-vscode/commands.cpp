#include "thermo.h"

#include "src\ArduinoModbusSlave-master\src\ModbusSlave.h"

#define FIRMWARE_VERSION 30 // x10
#define MODBUS_ADDRESS 1
#define BAUD_RATE 9600

Modbus slave(MODBUS_ADDRESS, MODBUS_CONTROL_PIN_NONE);

void serial_init()
{
    Serial.begin(BAUD_RATE);                                     // Initialize serial communications with the PC
    slave.begin(BAUD_RATE);
}

void serial_process()
{
	slave.poll();
}

