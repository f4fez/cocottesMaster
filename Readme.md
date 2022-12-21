# Cocotte master
Cocotte master is an ESP32 supervisor for the cocotte AVR project. The AVR subproject (Another repository) manage input and outputs to control chicken house doors and electrical elements.

This device communicate as master using the serial port to the AVR slave.
The main goal is to open and close the chicken gate at sunrise/sunset. Computations are done based the position set in the
SystemState.cpp file

The device require a wifi connection to get valide date/time. An http server is also available to command outputs (Door/light...)
A BME280 probe is connected on the SPI port to provide temperatures and humidity.

The http server is **NOT** secured ! Do not expose it on internet as is.

## Build
This project is intended to be build with platform.io