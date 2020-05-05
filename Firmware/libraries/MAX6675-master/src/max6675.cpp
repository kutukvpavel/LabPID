// This libary is originally developed in this repo:
// https://github.com/adafruit/MAX6675-library
//
// Original Header:
// this library is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/using-a-thermocouple

#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

#include <stdlib.h>
#include "max6675.h"
#include <SPI.h>

MAX6675::MAX6675(int8_t SCLK, int8_t CS, int8_t MISO, double OFFSET) {
  begin(SCLK, CS, MISO, OFFSET);
}

MAX6675::MAX6675(int8_t CS, double OFFSET) {
  begin(CS, OFFSET);
}

MAX6675::MAX6675(){
}

void MAX6675::begin(int8_t SCLK, int8_t CS, int8_t MISO, double OFFSET) {
  hwSPI = false;
  offset = OFFSET;
  sclk = SCLK;
  cs = CS;
  miso = MISO;

  //define pin modes
  pinMode(cs, OUTPUT);
  pinMode(sclk, OUTPUT);
  pinMode(miso, INPUT);

  digitalWrite(cs, HIGH);
}

void MAX6675::begin(int8_t CS, double OFFSET) {
  hwSPI = true;
  offset = OFFSET;
  cs = CS;
  //define pin modes
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
}

double MAX6675::readCelsius(void) {

  uint16_t v;

  digitalWrite(cs, LOW);

  // CSB Fall to Output Enable
  delayMicroseconds(1000);

  if (hwSPI) {
#ifdef MAX6675_LIBRARY_HW_SLOWDOWN
    uint8_t oldSPCR = SPCR;
    SPCR |= 3; // As slow as possible (clock/128 or clock/64 depending on SPI2X)
#endif // MAX6675_LIBRARY_HW_SLOWDOWN
    v = SPI.transfer16(0);
#ifdef MAX6675_LIBRARY_HW_SLOWDOWN
    SPCR = oldSPCR;
#endif // MAX6675_LIBRARY_HW_SLOWDOWN
  } else {
    v = spiread();
    v <<= 8;
    v |= spiread();
  }

  digitalWrite(cs, HIGH);
  // CSB Rise to Output Disable
  delayMicroseconds(1000);

  if (v & 0x4) {
    // uh oh, no thermocouple attached!
    return NAN;
    //return -100;
  }

  v >>= 3;

  return v*0.25;
}

double MAX6675::readFahrenheit(void) {
  return readCelsius() * 9.0/5.0 + 32;
}

void MAX6675::setOffsetCelsius(double OFFSET) {
  offset = OFFSET;
}

void MAX6675::setOffsetFahrenheit(double OFFSET) {
  offset = (OFFSET - 32) * (5.0/9.0);
}

byte MAX6675::spiread(void) {
  int i;
  byte d = 0;

  for (i=7; i>=0; i--)
  {
    digitalWrite(sclk, LOW);
    delayMicroseconds(1000);
    if (digitalRead(miso)) {
      //set the bit to 0 no matter what
      d |= (1 << i);
    }

    digitalWrite(sclk, HIGH);
    delayMicroseconds(1000);
  }

  return d;
}
