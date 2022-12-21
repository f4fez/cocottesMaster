#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "io.h"
#include "SystemState.h"

Adafruit_BME280 bme;
bool envSensorAvailable;

extern struct SystemState state;

void ioInit() {
  Wire.begin();
  Serial2.begin(9600);
}

void ioStart() {
   
  Serial.println("Look up for BME280");
  envSensorAvailable = bme.begin(0x76);
    if (!envSensorAvailable) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    }
}

float ioGetTemperature() {
  if(!envSensorAvailable)
    return 0.0f;
  return bme.readTemperature();
}

float ioGetPressure() {
  if(!envSensorAvailable)
    return 0.0f;
  return bme.readPressure() / 100.0F;
}

float ioGetHumidity() {
  if(!envSensorAvailable)
    return 0.0f;
  return bme.readHumidity();
}

void ioLoop() {
  int val = Serial2.read();
  switch(val) {
    case -1:
      return;
    case 'L':
      state.light = true;
      return;
    case 'l':
      state.light = false;
      return;
    case 'P':
      state.heating = true;
      return;
    case 'p':
      state.heating = false;
      return;
    case 'U':
    case 'u':
      state.doorMotor = DOOR_GO_UP;
      return;
    case 'D':
    case 'd':
      state.doorMotor = DOOR_GO_DOWN;
      return;
    case 'S':
    case 's':
      state.doorMotor = DOOR_STOP;
      state.door = DOOR_UNKNOW;
      return;
    case 'o':
      state.door = DOOR_OPEN;
      return;
    case 'c':
      state.door = DOOR_CLOSE;
      return;
    case 'O':
      state.door = DOOR_OPEN_MANUAL;
      return;
    case 'C':
      state.door = DOOR_CLOSE_MANUAL;
      return;
    case 'X':
      Serial.write("Arduino reset received");
      return;
    default:
      Serial.write("Wrong char received: ");
      Serial.write(val);
  }
}

void commandDoorUp(bool manual) {
  if (state.doorMotor == DOOR_STOP)
    Serial2.write(manual ? 'U' : 'u');
}
void commandDoorDown(bool manual) {
  if (state.doorMotor == DOOR_STOP)
    Serial2.write(manual ? 'D' : 'd');
}
void commandLightOn() {
  Serial2.write('L');
}
void commandLightOff() {
  Serial2.write('l');
}
void commandPlugOn() {
  Serial2.write('P');
}
void commandPlugOff() {
  Serial2.write('p');
}
