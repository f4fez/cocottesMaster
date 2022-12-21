#include <Arduino.h>
#include <ESPNtpClient.h>
#include <sunset.h>

#include "SystemState.h"
#include "io.h"
#include "net.h"

#define LATITUDE        45.8063889
#define LONGITUDE       3.339444
#define DST_OFFSET      2

struct SystemState state;
TimerHandle_t sensorTimer;
SunSet sun;

void sensorTimerCallback( TimerHandle_t xTimer );

void stateInit() {
  sun.setPosition(LATITUDE, LONGITUDE, DST_OFFSET);
  sun.setTZOffset(DST_OFFSET);
}

void stateStart() {
  sensorTimerCallback(NULL);
  sensorTimer = xTimerCreate("Sensor", 10000/portTICK_PERIOD_MS, pdTRUE, 0, sensorTimerCallback);
  xTimerStart( sensorTimer, 0 );
}

void stateDoorUpdate(enum doorMotorState way) {
  state.doorMotor = way;
}

void stateLightUpdate(bool newState) {
  state.light = newState;
}

void stateHeatingUpdate(bool newState) {
  state.heating = newState;
}


String twoDigits(int digits)
{
    if(digits < 10) {
        String i = '0'+String(digits);
        return i;
    }
    else {
        return String(digits);
    }
}

void print_systemState() {
  Serial.print("State: T=");
  Serial.print(state.temperature);
  Serial.print("°C. P=");
  Serial.print(state.pressure);
  Serial.print("hPA. H=");
  Serial.print(state.humidity);
  Serial.print("%. W=");
  Serial.print(state.rssi);
  Serial.println("dBm");

  Serial.print("Sunrise at ");
  Serial.print(((int)state.sunrise)/60);
  Serial.print(":");
  Serial.print(twoDigits(((int)state.sunrise)%60));
  Serial.print(". Sunset at ");
  Serial.print(((int)state.sunset)/60);
  Serial.print(":");
  Serial.print(twoDigits(((int)state.sunset)%60));
}

void sensorTimerCallback( TimerHandle_t xTimer ) {
  state.pressure = ioGetPressure();
  state.temperature = ioGetTemperature();
  state.humidity = ioGetHumidity();
  state.rssi = get_rssi();
  
  if(NTP.getLastNTPSync() != 0 && state.doorMotor == DOOR_STOP) {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    Serial.println (NTP.getTimeDateStringUs());
    localtime_r(&now, &timeinfo);
    sun.setCurrentDate(timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday);
    state.sunrise = (int) sun.calcCivilSunrise();
    state.sunset = (int) sun.calcCivilSunset();

    int currentTimeMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    bool isDay = state.sunrise < currentTimeMinutes && currentTimeMinutes < state.sunset;
    updateMotorState(isDay ? DOOR_OPEN : DOOR_CLOSE);

  }
  
  print_systemState();
}

  void updateMotorState(enum doorState expectedState) {
    enum doorState currentState = state.door;
    if (currentState == expectedState) {
      return;
    } else if ((expectedState == DOOR_OPEN) && (currentState == DOOR_OPEN_MANUAL)) {
      state.door = DOOR_OPEN;
    } else if ((expectedState == DOOR_CLOSE) && (currentState == DOOR_CLOSE_MANUAL)) {
      state.door = DOOR_CLOSE;
    } else if ((currentState == DOOR_UNKNOW || currentState == DOOR_CLOSE) && expectedState == DOOR_OPEN) {
        commandDoorUp(false);
    } else if ((currentState == DOOR_UNKNOW || currentState == DOOR_OPEN) && expectedState == DOOR_CLOSE) {
        commandDoorDown(false);
    }
    
  }

