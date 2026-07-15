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

// Quay servo tới một góc (độ, 90 = giữa xe) và đọc khoảng cách tại đó.
static int sampleAtAngle(int angleDeg) {
  delay(HAND_FOLLOW_SERVO_PRE_DELAY_MS);
  servo.write(constrain(angleDeg + servoOffset, 0, 180));
  delay(HAND_FOLLOW_SERVO_SETTLE_MS);
  return readSonarAveragedCm();
}

static bool isValidHandDistance(int distanceCm) {
  return distanceCm > 0 && distanceCm < HAND_FOLLOW_MAX_DETECT_DISTANCE;
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
    if (isValidHandDistance(sampleAtAngle(rightAngle))) {
      return rightAngle;
    }
    if (isValidHandDistance(sampleAtAngle(leftAngle))) {
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
      Serial.print(F("[HandFollow] C="));
      Serial.print(distCenter);
      Serial.println(F(" in tolerance -> stop"));
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

    Serial.print(F("[HandFollow] C="));
    Serial.print(distCenter);
    Serial.print(F(" error="));
    Serial.print(error);
    Serial.print(F(" speed="));
    Serial.print(speed);
    Serial.println(error < 0 ? F(" -> FORWARD") : F(" -> BACKWARD"));

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
  bool validLeft = isValidHandDistance(distLeft);

  Serial.print(F("[HandFollow] scan L="));
  Serial.print(distLeft);
  Serial.print(validLeft ? F("(ok)") : F("(--)"));
  Serial.print(F(" C="));
  Serial.print(distCenter);
  Serial.print(F("(--)"));
  Serial.print(F(" R="));
  Serial.print(distRight);
  Serial.println(validRight ? F("(ok)") : F("(--)"));

  if (!validRight && !validLeft) {
    // Mất dấu ở cả 3 hướng bám gần -> dừng xe và quét toàn dải tìm lại tay.
    Serial.println(F("[HandFollow] LOST -> stop + full scan"));
    motorRun(0, 0);
    int foundAngle = scanForHand();
    if (foundAngle < 0) {
      Serial.println(F("[HandFollow] full scan: not found, waiting"));
      servo.write(90 + servoOffset);
      return; // Đứng yên chờ; lần gọi kế tiếp của loop sẽ tự quét lại.
    }
    Serial.print(F("[HandFollow] full scan: found at angle "));
    Serial.println(foundAngle);
    if (foundAngle != 90) {
      turnTowardAngle(foundAngle);
    }
    servo.write(90 + servoOffset);
    return;
  }

  servo.write(90 + servoOffset);

  // Ưu tiên bên gần hơn; chỉ xoay khi giữa không thấy nhưng một bên thấy.
  if (validLeft && (!validRight || distLeft <= distRight)) {
    Serial.println(F("[HandFollow] center lost -> turn LEFT"));
    turnTowardAngle(leftAngle);
  } else {
    Serial.println(F("[HandFollow] center lost -> turn RIGHT"));
    turnTowardAngle(rightAngle);
  }
}
