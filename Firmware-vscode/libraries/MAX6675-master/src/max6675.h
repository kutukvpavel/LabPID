// this library is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/using-a-thermocouple
#ifndef MAX6675_LIBRARY_H
#define MAX6675_LIBRARY_H

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class MAX6675 {
 public:
  /**
   * Initiate software SPI mode.
   */
  MAX6675(int8_t SCLK, int8_t CS, int8_t MISO, double OFFSET = 0.0);
  /**
   * Initiate hardware SPI mode.
   * Assumes you already setup spi (with SPI.begin())
   */
  MAX6675(int8_t CS, double OFFSET = 0.0);

  /**
   * No initialization directly, only later
   */
  MAX6675();

  void begin(int8_t SCLK, int8_t CS, int8_t MISO, double OFFSET = 0.0);
  void begin(int8_t CS, double OFFSET = 0.0);

  double readCelsius(void);
  double readFahrenheit(void);

  void setOffsetCelsius(double OFFSET);

  void setOffsetFahrenheit(double OFFSET);

  // For compatibility with older versions:
  double readFarenheit(void) { return readFahrenheit(); }
 private:
  int8_t sclk, miso, cs;
  double offset;
  uint8_t spiread(void);
  bool hwSPI = false;
};

#endif
