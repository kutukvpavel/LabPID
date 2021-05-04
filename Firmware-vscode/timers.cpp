#include "thermo.h"

void timers_init()
{
    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: 62,500 kHz
    // Mode: Fast PWM top=ICR1
    // OC1A output: Non-Inv.
    // OC1B output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer1 Overflow Interrupt: On
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0x82;
    TCCR1B=0x1C;
    TCNT1H=0x00;
    TCNT1L=0x00;

    ICR1 = static_cast<uint16_t>((TIMING * 62499UL) / 1000UL);

    OCR1AH=0x00; // init to 0 duty cycle
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;

    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 15,625 kHz
    // Mode: Ph. correct PWM top=OCR2A
    // OC2A output: Disconnected
    // OC2B output: Non-Inverted PWM
    ASSR=0x00;
    TCCR2A=0x21;
    TCCR2B=0x0F;
    TCNT2=0x00;
    OCR2A=0xFF; //16.3mS
    OCR2B=0x00;

    // Timer/Counter 1 Interrupt(s) initialization
    TIMSK1=0x01;

    // Timer/Counter 2 Interrupt(s) initialization
    TIMSK2=0x01;

    // Timer0 (millis) COMPA interrupt reuse
    OCR0A = 0x7F; //Half period phase shift drom millis in order to minimize interference
    TIMSK0 |= BV8(OCIE0A);
}