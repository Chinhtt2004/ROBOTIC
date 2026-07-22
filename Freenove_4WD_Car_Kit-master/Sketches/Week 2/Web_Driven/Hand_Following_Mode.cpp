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
  // Bù sai số hệ thống do TX/RX không đồng tâm (xem HAND_FOLLOW_DISTANCE_CALIBRATION_CM).
  return (int)(sum / validCount) + HAND_FOLLOW_DISTANCE_CALIBRATION_CM;
}

// Quay servo tới một góc (độ, 90 = giữa xe) và đọc khoảng cách tại đó.
static int sampleAtAngle(int angleDeg) {
  delay(HAND_FOLLOW_SERVO_PRE_DELAY_MS);
  servo.write(constrain(angleDeg + servoOffset, 0, 180));
  delay(HAND_FOLLOW_SERVO_SETTLE_MS);
  return readSonarAveragedCm();
}

static bool isValidHandDistanceWithLimit(int distanceCm, int maxDetectCm) {
  return distanceCm > 0 && distanceCm < maxDetectCm;
}

static bool isValidHandDistance(int distanceCm) {
  return isValidHandDistanceWithLimit(distanceCm, HAND_FOLLOW_MAX_DETECT_DISTANCE);
}

// Góc servo > 90 là hướng trái (xem turnTowardAngle): dùng ngưỡng phát hiện
// rộng hơn cho hướng này để bù cảm biến kém nhạy bên trái.
static bool isValidHandDistanceAtAngle(int distanceCm, int angleDeg) {
  int maxDetectCm = (angleDeg > 90) ? HAND_FOLLOW_LEFT_MAX_DETECT_DISTANCE : HAND_FOLLOW_MAX_DETECT_DISTANCE;
  return isValidHandDistanceWithLimit(distanceCm, maxDetectCm);
}

// Quét toàn dải góc, bắt đầu từ giữa và mở rộng dần ra hai bên, để tìm lại
// tay khi cả 3 hướng bám gần đều không thấy gì. Trả về góc servo tìm thấy
// tay, hoặc -1 nếu quét hết mà không thấy.
static int scanForHand() {
  if (isValidHandDistance(sampleAtAngle(90))) {
    return 90;
  }
  for (int offset = HAND_FOLLOW_LOST_SCAN_STEP; offset <= HAND_FOLLOW_LOST_SCAN_MAX_OFFSET;
       offset += HAND_FOLLOW_LOST_SCAN_STEP) {
    int rightAngle = 90 - offset;
    int leftAngle = 90 + offset;
    if (isValidHandDistanceAtAngle(sampleAtAngle(rightAngle), rightAngle)) {
      return rightAngle;
    }
    if (isValidHandDistanceAtAngle(sampleAtAngle(leftAngle), leftAngle)) {
      return leftAngle;
    }
  }
  return -1;
}

// Xoay tại chỗ hướng về góc servo đã phát hiện tay. Thời gian xoay tỉ lệ với
// độ lệch so với hướng giữa xe, để lệch càng nhiều thì xoay càng lâu.
static void turnTowardAngle(int angleDeg) {
  int offset = angleDeg - 90;
  if (offset == 0) {
    return;
  }
  int steps = abs(offset) / HAND_FOLLOW_SCAN_ANGLE_INTERVAL;
  if (steps < 1) {
    steps = 1;
  }
  int turnMs = HAND_FOLLOW_TURN_STEP_MS * steps;
  if (offset > 0) {
    // Tay ở góc servo > 90 (bên trái) -> xoay trái.
    motorRun(-HAND_FOLLOW_TURN_SPEED, HAND_FOLLOW_TURN_SPEED);
  } else {
    motorRun(HAND_FOLLOW_TURN_SPEED, -HAND_FOLLOW_TURN_SPEED);
  }
  delay(turnMs);
  motorRun(0, 0);
}

void updateHandFollowingMode() {
  int rightAngle = 90 - HAND_FOLLOW_SCAN_ANGLE_INTERVAL;
  int leftAngle = 90 + HAND_FOLLOW_SCAN_ANGLE_INTERVAL;

  // Đọc và xử lý hướng giữa TRƯỚC TIÊN, ra lệnh dừng/tiến/lùi ngay lập tức.
  // Việc quét thêm 2 bên (bên dưới) chỉ phục vụ xác định hướng khi mất dấu,
  // không được phép trì hoãn quyết định dừng xe.
  int distCenter = sampleAtAngle(90);
  bool validCenter = isValidHandDistance(distCenter);

  if (validCenter) {
    int error = HAND_FOLLOW_TARGET_DISTANCE - distCenter;

    if (abs(error) <= HAND_FOLLOW_TOLERANCE) {
      motorRun(0, 0);
      servo.write(90 + servoOffset);
      return;
    }

    // Tốc độ tỉ lệ liên tục theo khoảng cách: tay càng xa (gần biên phát
    // hiện) thì đuổi theo càng nhanh, càng gần thì lùi/tiến càng chậm. Lùi
    // và tiến dùng min speed riêng vì lực khởi động cần thiết khác nhau.
    int minSpeed = (error < 0) ? HAND_FOLLOW_FORWARD_MIN_SPEED : HAND_FOLLOW_BACKWARD_MIN_SPEED;
    int speed = map(constrain(distCenter, 0, HAND_FOLLOW_MAX_DETECT_DISTANCE),
                     0, HAND_FOLLOW_MAX_DETECT_DISTANCE,
                     minSpeed, HAND_FOLLOW_MAX_SPEED);

    if (error < 0) {
      motorRun(speed, speed);
    } else {
      motorRun(-speed, -speed);
    }
    servo.write(90 + servoOffset);
    return;
  }

  // Giữa không thấy tay -> quét thêm 2 bên để tìm hướng lệch.
  int distRight = sampleAtAngle(rightAngle);
  int distLeft = sampleAtAngle(leftAngle);
  bool validRight = isValidHandDistance(distRight);
  bool validLeft = isValidHandDistanceAtAngle(distLeft, leftAngle);

  if (!validRight && !validLeft) {
    // Mất dấu ở cả 3 hướng bám gần -> dừng xe và quét toàn dải tìm lại tay.
    motorRun(0, 0);
    int foundAngle = scanForHand();
    if (foundAngle < 0) {
      servo.write(90 + servoOffset);
      return; // Đứng yên chờ; lần gọi kế tiếp của loop sẽ tự quét lại.
    }
    if (foundAngle != 90) {
      turnTowardAngle(foundAngle);
    }
    servo.write(90 + servoOffset);
    return;
  }

  servo.write(90 + servoOffset);

  // Ưu tiên bên gần hơn; chỉ xoay khi giữa không thấy nhưng một bên thấy.
  if (validLeft && (!validRight || distLeft <= distRight)) {
    turnTowardAngle(leftAngle);
  } else {
    turnTowardAngle(rightAngle);
  }
}
