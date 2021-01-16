// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/using-a-thermocouple

#include "max6675.h"
#include <SPI.h>

int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;

MAX6675 thermocouple;
int vccPin = 3;
int gndPin = 2;

void setup() {
  Serial.begin(9600);

  thermocouple.begin(thermoCLK, thermoCS, thermoDO);
  // use Arduino pins
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp

   Serial.print("C = ");
   Serial.println(thermocouple.readCelsius());
   Serial.print("F = ");
   Serial.println(thermocouple.readFahrenheit());

   delay(1000);
}
