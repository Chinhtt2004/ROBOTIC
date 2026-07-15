#ifndef HAND_FOLLOWING_MODE_H
#define HAND_FOLLOWING_MODE_H

// Ngoài khoảng này (cm) coi như không phát hiện được tay ở hướng đang xét.
#define HAND_FOLLOW_MAX_DETECT_DISTANCE 20
#define HAND_FOLLOW_TARGET_DISTANCE 12
#define HAND_FOLLOW_TOLERANCE 3
#define HAND_FOLLOW_EMERGENCY_DISTANCE 8
// MOTOR_PWM_DEAD (Freenove_4WD_Car_for_Arduino.h) chỉ cắt PWM dưới 10, nhưng
// động cơ thực tế cần PWM cao hơn hẳn mới đủ lực thắng ma sát tĩnh để quay.
// Lùi cần lực lớn hơn ngay từ đầu (thường đang đứng gần sát/gần dừng) nên
// tách min speed riêng với tiến.
#define HAND_FOLLOW_FORWARD_MIN_SPEED 50
#define HAND_FOLLOW_BACKWARD_MIN_SPEED 90
#define HAND_FOLLOW_MAX_SPEED 120
#define HAND_FOLLOW_SAMPLE_COUNT 5

// Bám hướng liên tục: mỗi chu kỳ quét 3 góc trái/giữa/phải quanh vị trí giữa
// (giống cách Automatic_Obstacle_Avoidance_Mode lấy 3 điểm) để biết tay đang
// lệch về hướng nào.
#define HAND_FOLLOW_SCAN_ANGLE_INTERVAL 25
#define HAND_FOLLOW_SERVO_PRE_DELAY_MS 100
#define HAND_FOLLOW_SERVO_SETTLE_MS 80

// Quét toàn dải khi cả 3 hướng trên đều không thấy tay (mất dấu hoàn toàn).
#define HAND_FOLLOW_LOST_SCAN_MAX_OFFSET 60
#define HAND_FOLLOW_LOST_SCAN_STEP 20

#define HAND_FOLLOW_TURN_SPEED 130
#define HAND_FOLLOW_TURN_STEP_MS 250

void updateHandFollowingMode();

#endif
