#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

enum doorMotorState {DOOR_GO_UP, DOOR_GO_DOWN, DOOR_STOP};
enum doorState {DOOR_OPEN_MANUAL, DOOR_CLOSE_MANUAL, DOOR_OPEN, DOOR_CLOSE, DOOR_UNKNOW};

struct SystemState {
  bool heating = false;
  bool light = false;
  enum doorMotorState doorMotor = DOOR_STOP;
  enum doorState door = DOOR_UNKNOW;
  float temperature = 0.0f;
  float pressure = 0.0f;
  float humidity = 0.0f;
  int rssi = 0;
  int sunrise;
  int sunset;
};

void stateInit();
void stateStart();
void stateDoorUpdate(enum doorMotorState way);
void stateLightUpdate(bool newState);
void stateHeatingUpdate(bool newState);
void updateMotorState(enum doorState expectedState);

#endif // SYSTEM_STATE_H