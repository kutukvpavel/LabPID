#include "thermo.h"

#include "src\PCA9555-master\clsPCA9555.h"

#define GPIO_MODE_MAP 12, 4  //Number of consequtive inputs (element index % 2 != 0) / outputs (element index % 2 == 0)
#define GPIO_IC_ADDRESS 0x20_ui8

PCA9555 gpio_port(GPIO_IC_ADDRESS);

uint8_t gpio_map[] = { GPIO_MODE_MAP };
uint8_t output_count;
uint8_t input_count;
bool gpioOK;

void gpio_init()
{
    if (gpioOK) return;
    gpioOK = gpio_port.begin();
    if (!gpioOK) return;
    gpio_port.setClock(400000);
    gpio_write_all(mem_get_gpio()); //Set output values first, since default value for output register is "all ON"
    uint8_t current_pin = 0;
    for (uint8_t i = 0; i < arraySize(gpio_map); i++) //Only then set pin modes so that they can actually become outputs
    {
        uint8_t state;
        if (i % 2 == 0)
        {
            state = OUTPUT;
            output_count++;
        }
        else
        {
            state = INPUT;
            input_count++;
        }
        for (uint8_t j = 0; j < gpio_map[i]; j++)
        {
            gpio_port.pinMode(current_pin++, state);
        }
    }
}

void gpio_write_all(uint16_t val)
{
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
    gpio_port.digitalWrite(address, value);
}

uint16_t gpio_get_all_outputs()
{
    return gpio_port.getOutputRegister();
}

bool gpio_get_output(uint8_t address)
{
    return gpio_get_all_outputs() & _BV(address);
}

bool gpio_check_output_address(uint8_t addr)
{
    return gpioOK && (addr < output_count);
}

bool gpio_check_input_address(uint8_t addr)
{
    return gpioOK && (addr < input_count);
}