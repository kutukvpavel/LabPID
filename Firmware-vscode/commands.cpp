#include "thermo.h"

#include "src\ArduinoModbusSlave-master\src\ModbusSlave.h"

#define FIRMWARE_VERSION 30 // x10
#define MODBUS_ADDRESS 1
#define BAUD_RATE 9600
#define MAX_ADDRESS(addr, len) (static_cast<uint8_t>(addr) + static_cast<uint8_t>(len) - 1_ui8)
#define READ_UINT8(offset) (static_cast<uint8_t>(slave.readRegisterFromBuffer(offset)))
#define WRITE_UINT8(data, offset) (slave.writeRegisterToBuffer(offset, data))

/* Register map */
//Inputs
enum
{
    INPUT_REG_VERSION = 0, //uint8
    INPUT_REG_INPUT, //float
    INPUT_REG_AMBIENT = INPUT_REG_INPUT + 2 //float
    //TODO: thrmocouple, DS stuff
};
//Holders
enum
{
    HOLDING_REG_SETPOINT = 0, //float
    HOLDING_REG_MODE = HOLDING_REG_SETPOINT + 2, //uint8
    HOLDING_REG_POWER, //int8, only accepts writes in MANUAL mode
    HOLDING_REG_P, //float
    HOLDING_REG_I = HOLDING_REG_P + 2, //float
    HOLDING_REG_D = HOLDING_REG_I + 2, //float
    HOLDING_REG_PA = HOLDING_REG_D + 2, //float
    HOLDING_REG_IA = HOLDING_REG_PA + 2, //float
    HOLDING_REG_DA = HOLDING_REG_IA + 2, //float
    HOLDING_REG_DISTILL_POWER = HOLDING_REG_DA + 2, //float
    HOLDING_REG_DISTILL_WINDOW = HOLDING_REG_DISTILL_POWER + 2, //float
    HOLDING_REG_RAMP = HOLDING_REG_DISTILL_WINDOW + 2, //float
    HOLDING_REG_I_LIMIT = HOLDING_REG_RAMP + 2, //float
    HOLDING_REG_AMBIENT_DEFAULT = HOLDING_REG_I_LIMIT + 2, //float
    HOLDING_REG_AMPLIFIER = HOLDING_REG_AMBIENT_DEFAULT + 2,//float
    HOLDING_REG_CALIBRATION = HOLDING_REG_AMPLIFIER + 2, //float x3
    HOLDING_REG_DS_CHANNEL_INDEX = HOLDING_REG_CALIBRATION + 2 * CHANNEL_COUNT, //uin8
    HOLDING_REG_DS_AMBIENT_INDEX, //uint8
    HOLDING_REG_ANALOG_CJC, //bool
    HOLDING_REG_AVERAGING, //bytes
    HOLDING_REG_COOLER = HOLDING_REG_AVERAGING + CHANNEL_COUNT, //bytes
    HOLDING_REG_SAVE = HOLDING_REG_COOLER + CHANNEL_COUNT, //Write-only
};


Modbus slave(MODBUS_ADDRESS, MODBUS_CONTROL_PIN_NONE);

void read_flags(bool* arr, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        arr[i] = READ_UINT8(i);
    }
}

void write_flags(bool* arr, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        WRITE_UINT8(arr[i], i);
    }
}

float read_float(uint8_t offset = 0) //Little-endian
{
    uint16_t b[] = { slave.readRegisterFromBuffer(offset), slave.readRegisterFromBuffer(offset + 1) };
    return *reinterpret_cast<float*>(&b);
}

void write_float(float val, uint8_t offset = 0)
{   
    uint16_t* b = reinterpret_cast<uint16_t*>(&val);
    slave.writeRegisterToBuffer(offset++, *b++);
    slave.writeRegisterToBuffer(offset, *b);
}

uint8_t cb_write_outputs(uint8_t fc, uint16_t addr, uint16_t len)
{
    if (!gpio_check_output_address(MAX_ADDRESS(addr, len))) return STATUS_ILLEGAL_DATA_ADDRESS;
    for (uint8_t i = 0; i < len; i++)
    {
        gpio_write(static_cast<uint8_t>(addr++), slave.readCoilFromBuffer(i));
    }
    return STATUS_OK;
}

uint8_t cb_read_outputs(uint8_t fc, uint16_t addr, uint16_t len)
{
    if (!gpio_check_output_address(MAX_ADDRESS(addr, len))) return STATUS_ILLEGAL_DATA_ADDRESS;
    for (uint8_t i = 0; i < len; i++)
    {
        slave.writeCoilToBuffer(i, gpio_get_output(addr++));
    }
    return STATUS_OK;
}

uint8_t cb_write_params(uint8_t fc, uint16_t addr, uint16_t len)
{
    switch (addr)
    {
    case HOLDING_REG_SETPOINT:
        Setpoint = read_float();
        break;
    case HOLDING_REG_MODE:
        regulationMode = READ_UINT8(0) % 4_ui8;
        break;
    case HOLDING_REG_POWER:
        if (regulationMode != MODE_MANUAL) return STATUS_ILLEGAL_DATA_VALUE;
        power = static_cast<int8_t>(slave.readRegisterFromBuffer(0));
        check_power();
        break;
    case HOLDING_REG_P:
        K[0][0] = read_float();
        break;
    case HOLDING_REG_I:
        K[1][0] = read_float();
        break;
    case HOLDING_REG_D:
        K[2][0] = read_float();
        break;
    case HOLDING_REG_PA:
        K[0][1] = read_float();
        break;
    case HOLDING_REG_IA:
        K[1][1] = read_float();
        break;
    case HOLDING_REG_DA:
        K[2][1] = read_float();
        break;
    case HOLDING_REG_DISTILL_POWER:
        distillExtraPower = read_float();
        break;
    case HOLDING_REG_DISTILL_WINDOW:
        distillTempWindow = read_float();
        break;
    case HOLDING_REG_RAMP:
        rampStepLimit = read_float();
        break;
    case HOLDING_REG_I_LIMIT:
        integralTermLimit = read_float();
        break;
    case HOLDING_REG_AMBIENT_DEFAULT:
        defaultAmbientTemp = read_float();
        break;
    case HOLDING_REG_AMPLIFIER:
        amplifierCoeff = read_float();
        break;
    case HOLDING_REG_CALIBRATION:
        {
            for (uint8_t i = 0; i < len; i++)
            {
                calibration[i] = read_float(i * 2_ui8);
            }
        }
        break;
    case HOLDING_REG_DS_CHANNEL_INDEX:
        dsIndexes[1] = READ_UINT8(0);
        break;
    case HOLDING_REG_DS_AMBIENT_INDEX:
        dsIndexes[0] = READ_UINT8(0);
        break;
    case HOLDING_REG_ANALOG_CJC:
        cjc = READ_UINT8(0);
        break;
    case HOLDING_REG_AVERAGING:
        read_flags(averaging, arraySize(averaging));
        break;
    case HOLDING_REG_COOLER:
        read_flags(enableCooler, arraySize(enableCooler));
        break;
    case HOLDING_REG_SAVE:
        mem_save();
        break;
    default:
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }
    return STATUS_OK;
}

uint8_t cb_read_params(uint8_t fc, uint16_t addr, uint16_t len)
{
    switch (addr)
    {
    case HOLDING_REG_SETPOINT:
        write_float(prevSetpoint);
        break;
    case HOLDING_REG_MODE:
        WRITE_UINT8(regulationMode, 0);
        break;
    case HOLDING_REG_POWER:
        WRITE_UINT8(power, 0);
        break;
    case HOLDING_REG_P:
        write_float(K[0][0]);
        break;
    case HOLDING_REG_I:
        write_float(K[1][0]);
        break;
    case HOLDING_REG_D:
        write_float(K[2][0]);
        break;
    case HOLDING_REG_PA:
        write_float(K[0][1]);
        break;
    case HOLDING_REG_IA:
        write_float(K[1][1]);
        break;
    case HOLDING_REG_DA:
        write_float(K[2][1]);
        break;
    case HOLDING_REG_DISTILL_POWER:
        write_float(distillExtraPower);
        break;
    case HOLDING_REG_DISTILL_WINDOW:
        write_float(distillTempWindow);
        break;
    case HOLDING_REG_RAMP:
        write_float(rampStepLimit);
        break;
    case HOLDING_REG_I_LIMIT:
        write_float(integralTermLimit);
        break;
    case HOLDING_REG_AMBIENT_DEFAULT:
        write_float(defaultAmbientTemp);
        break;
    case HOLDING_REG_AMPLIFIER:
        write_float(amplifierCoeff);
        break;
    case HOLDING_REG_CALIBRATION:
        {
            for (uint8_t i = 0; i < arraySize(calibration); i++)
            {
                write_float(calibration[i], i * 2_ui8);
            }
        }
        break;
    case HOLDING_REG_DS_CHANNEL_INDEX:
        WRITE_UINT8(dsIndexes[1], 0); 
        break;
    case HOLDING_REG_DS_AMBIENT_INDEX:
        WRITE_UINT8(dsIndexes[0], 0);
        break;
    case HOLDING_REG_ANALOG_CJC:
        WRITE_UINT8(cjc, 0);
        break;
    case HOLDING_REG_AVERAGING:
        write_flags(averaging, arraySize(averaging));
        break;
    case HOLDING_REG_COOLER:
        write_flags(enableCooler, arraySize(enableCooler));
        break;
    default:
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }
    return STATUS_OK;
}

uint8_t cb_read_inputs(uint8_t fc, uint16_t addr, uint16_t len)
{
    if (!gpio_check_input_address(MAX_ADDRESS(addr, len))) return STATUS_ILLEGAL_DATA_ADDRESS;
    for (uint8_t i = 0; i < len; i++)
    {
        slave.writeDiscreteInputToBuffer(i, gpio_read(addr++));
    }
    return STATUS_OK;
}

uint8_t cb_read_regs(uint8_t fc, uint16_t addr, uint16_t len)
{
    switch (addr)
    {
    case INPUT_REG_VERSION:
        slave.writeRegisterToBuffer(0, FIRMWARE_VERSION);
        break;
    case INPUT_REG_INPUT:
        write_float(Input);
        break;
    case INPUT_REG_AMBIENT:
        write_float(ambientTemp);
        break;
    default:
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }
    return STATUS_OK;
}

void serial_init()
{
    //Prepare event callbacks
    slave.cbVector[CB_WRITE_COILS] = cb_write_outputs;
    slave.cbVector[CB_READ_COILS] = cb_read_outputs;
    slave.cbVector[CB_WRITE_HOLDING_REGISTERS] = cb_write_params;
    slave.cbVector[CB_READ_HOLDING_REGISTERS] = cb_read_params;
    slave.cbVector[CB_READ_DISCRETE_INPUTS] = cb_read_inputs;
    slave.cbVector[CB_READ_INPUT_REGISTERS] = cb_read_regs;

    // Initialize serial communications with the PC
    Serial.begin(BAUD_RATE);
    slave.begin(BAUD_RATE);
}

void serial_process()
{
	slave.poll();
}