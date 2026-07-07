#include "Hand_Following_Mode.h"
#include "Freenove_4WD_Car_for_Arduino.h"
#include "Automatic_Obstacle_Avoidance_Mode.h"

static int readSonarAveragedCm() {
  long sum = 0;
  int validCount = 0;
  for (int i = 0; i < HAND_FOLLOW_SAMPLE_COUNT; i++) {
    int sample = (int)getSonar();
    if (sample > 0 && sample < MAX_DISTANCE) {
      sum += sample;
      validCount++;
    }
  }
  if (validCount == 0) {
    return MAX_DISTANCE;
  }
  return (int)(sum / validCount);
}

void updateHandFollowingMode() {
  static int lastDistanceCm = -1;
  // A single HC-SR04 ping is noisy (echo cross-talk, missed pulses). Averaging
  // several samples smooths that out instead of reacting to one bad reading.
  int rawDistanceCm = readSonarAveragedCm();
  int distanceCm = rawDistanceCm;

  bool invalidReading = (rawDistanceCm <= 0 || rawDistanceCm >= MAX_DISTANCE);
  // HC-SR04 echo cross-talk: while continuously re-triggering on a hand that is
  // close and slowly moving, a ping occasionally reads back a spurious huge
  // jump. Left unfiltered, that single bad reading looks like "no target" and
  // makes the car stop dead instead of continuing to back away. If we were
  // just tracking something close, ignore the jump and reuse the last good
  // reading instead of treating it as target loss.
  bool implausibleJump = (!invalidReading && lastDistanceCm > 0 &&
                           lastDistanceCm <= HAND_FOLLOW_TARGET_DISTANCE + HAND_FOLLOW_NOISE_JUMP_CM &&
                           (rawDistanceCm - lastDistanceCm) > HAND_FOLLOW_NOISE_JUMP_CM);

  if (invalidReading || implausibleJump) {
    if (lastDistanceCm <= 0) {
      // resetCarAction();
      // setBuzzer(false);
      return;
    }
    distanceCm = lastDistanceCm;
  } else {
    lastDistanceCm = distanceCm;
  }

  // if (distanceCm <= HAND_FOLLOW_EMERGENCY_DISTANCE) {
  //   resetCarAction();
  //   setBuzzer(true);
  //   return;
  // }

  int error = HAND_FOLLOW_TARGET_DISTANCE - distanceCm;

  if (abs(error) <= HAND_FOLLOW_TOLERANCE) {
    // resetCarAction();
    motorRun(0, 0);
    // setBuzzer(false);
  }


  int speed = 180;
			// if (distanceCm < 50) {
			// 	speed = 120;
			// } else if (distanceCm < 70) {
			// 	speed = 140;
			// } else if (distanceCm < 90) {
			// 	speed = 160;
			// } else {
			// 	speed = 180;
			// }

  if (error < -3) {
    motorRun(speed, speed);
  } else if (error > 3) {
    motorRun(-speed, -speed);
  }
}
