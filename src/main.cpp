#include <Arduino.h>
#include "net.h"
#include "SPIFFS.h"
#include <sunset.h>
#include "io.h"
#include "SystemState.h"

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void setup()
{
  Serial.begin(9600);
  initSPIFFS();
  ioInit();
  stateInit();
  net_init();

  ioStart();
  stateStart();
  net_start();
  Serial.println("System setup end");
}

void loop()
{
  net_loop();
  ioLoop();
}
