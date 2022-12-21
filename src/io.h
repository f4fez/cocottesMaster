#ifndef IO_H
#define IO_H

void ioInit();
void ioStart();
float ioGetTemperature();
float ioGetPressure();
float ioGetHumidity();

void commandDoorUp(bool manual);
void commandDoorDown(bool manual);
void commandLightOn();
void commandLightOff();
void commandPlugOn();
void commandPlugOff();

void ioLoop();

#endif // IO_H