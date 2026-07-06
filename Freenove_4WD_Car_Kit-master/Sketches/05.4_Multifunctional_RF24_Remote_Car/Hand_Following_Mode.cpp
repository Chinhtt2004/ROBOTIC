#include "Hand_Following_Mode.h"
#include "Freenove_4WD_Car_for_Arduino.h"

void updateHandFollowingMode() {
  int distanceCm = (int)getSonar();

  if (distanceCm <= 0 || distanceCm >= MAX_DISTANCE) {
    resetCarAction();
    setBuzzer(false);
    return;
  }

  if (distanceCm <= HAND_FOLLOW_EMERGENCY_DISTANCE) {
    resetCarAction();
    setBuzzer(true);
    return;
  }

  int error = HAND_FOLLOW_TARGET_DISTANCE - distanceCm;

  if (abs(error) <= HAND_FOLLOW_TOLERANCE) {
    resetCarAction();
    setBuzzer(false);
    return;
  }

  int speed = map(constrain(abs(error), HAND_FOLLOW_TOLERANCE, 60), HAND_FOLLOW_TOLERANCE, 60, HAND_FOLLOW_MIN_SPEED, HAND_FOLLOW_MAX_SPEED);
  speed = constrain(speed, HAND_FOLLOW_MIN_SPEED, HAND_FOLLOW_MAX_SPEED);

  if (error > 0) {
    motorRun(speed, speed);
  } else {
    motorRun(-speed, -speed);
  }
}
