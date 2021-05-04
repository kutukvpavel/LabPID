#include "thermo.h"

#include "src\PCA9555-master\clsPCA9555.h"

PCA9555 gpio_port(GPIO_IC_ADDRESS);

uint8_t gpio_map[] = { GPIO_MODE_MAP };
bool gpioOK = false;

void gpio_init()
{
    if (gpioOK) return;
    gpioOK = gpio_port.begin();
    if (!gpioOK) return;
    gpio_port.setClock(400000);
    uint8_t current_pin = 0;
    for (uint8_t i = 0; i < arraySize(gpio_map); i++)
    {
        uint8_t state = i % 2 == 0 ? OUTPUT : INPUT;
        for (uint8_t j = 0; j < gpio_map[i]; j++)
        {
            gpio_port.pinMode(current_pin++, state);
        }
    }
}

void gpio_write_all(uint16_t val)
{
    if (!gpioOK) return;
    gpio_port.writeAll(val);
}

uint16_t gpio_read_all()
{
    return gpio_port.readAll();
}

bool gpio_read(uint8_t address)
{
    return gpio_port.digitalRead(address);
}

void gpio_write(uint8_t address, bool value)
{
    if (!gpioOK) return;
    gpio_port.digitalWrite(address, value);
}

void gpio_write(uint8_t code)
{
    gpio_write(code & 0x7F_ui8, (code & 0x80_ui8) != 0_ui8);
}

uint16_t gpio_get_output_register()
{
    return gpio_port.getOutputRegister();
}