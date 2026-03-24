#include "global.h"
#include "motor.h"

/*
ESP32 DEVKIT V1
SMART CAR
*/

extern bool connect_wifi();
extern void init_motor_pin_mode();

uint32_t global_status_word = 0;

void setup() {
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  init_motor_pin_mode();

  connect_wifi();
}

void loop() {
  delay(1000);
  MOTOR_MOVE_FULL_SPEED(&car);
  delay(1000);
  MOTOR_MOVE_STOP(&car);
}
