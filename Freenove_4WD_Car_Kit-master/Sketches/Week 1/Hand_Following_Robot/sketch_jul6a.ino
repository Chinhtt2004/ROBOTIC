/**********************************************************************
  Filename    : Automatic_Obstacle_Avoidance.ino
  Product     : Freenove 4WD Car for UNO
  Description : Automatic Obstacle Avoidance mode.
  Auther      : www.freenove.com
  Modification: 2019/08/15
**********************************************************************/
#include <Servo.h>
#define PIN_SERVO           2       

#define MOTOR_DIRECTION     0 //If the direction is reversed, change 0 to 1
#define PIN_DIRECTION_LEFT  4
#define PIN_DIRECTION_RIGHT 3
#define PIN_MOTOR_PWM_LEFT  6
#define PIN_MOTOR_PWM_RIGHT 5

#define PIN_SONIC_TRIG      7
#define PIN_SONIC_ECHO      8
#define TARGET_DISTANCE        15      // Khoảng cách mong muốn
#define DISTANCE_TOLERANCE      2      // Sai số ±2 cm
#define EMERGENCY_DISTANCE      8      // Phanh khẩn cấp
#define PIN_BATTERY         A0

#define MAX_DISTANCE    1000   
#define SONIC_TIMEOUT   (MAX_DISTANCE*60)
#define SOUND_VELOCITY    340   //soundVelocity: 340m/s
Servo servo;
byte servoOffset = 0;
int speedOffset;//batteryVoltageCompensationToSpeed

void setup() {
  pinMode(PIN_DIRECTION_LEFT, OUTPUT);
  pinMode(PIN_MOTOR_PWM_LEFT, OUTPUT);
  pinMode(PIN_DIRECTION_RIGHT, OUTPUT);
  pinMode(PIN_MOTOR_PWM_RIGHT, OUTPUT);

  pinMode(PIN_SONIC_TRIG, OUTPUT);// set trigPin to output mode
  pinMode(PIN_SONIC_ECHO, INPUT); // set echoPin to input mode
  servo.attach(PIN_SERVO);
  servo.write(90);
  calculateVoltageCompensation();
}

void loop() {
  updateAutomaticObstacleAvoidance();

  Serial.print("Distance: ");
  Serial.print(getSonar());
  Serial.println(" cm");

delay(30);
}

void updateAutomaticObstacleAvoidance()
{
    int distance = getSonar();

    // ================= PHANH KHẨN CẤP =================
    if(distance <= EMERGENCY_DISTANCE)
    {
        motorRun(0,0);
        return;
    }

    // ================= TAY QUÁ GẦN =================
    if(distance < TARGET_DISTANCE - DISTANCE_TOLERANCE)
    {
        motorRun(-80,-80);
    }

    // ================= GIỮ KHOẢNG CÁCH =================
    else if(distance <= TARGET_DISTANCE + DISTANCE_TOLERANCE)
    {
        motorRun(0,0);
    }

    // ================= TAY QUÁ XA =================
    else
    {
        int speed;

        if(distance > 80)
            speed = 180;
        else if(distance > 50)
            speed = 150;
        else if(distance > 30)
            speed = 120;
        else if(distance > 20)
            speed = 80;
        else
            speed = 50;

        motorRun(speed + speedOffset,
                 speed + speedOffset);
    }

    delay(30);
}

float getSonar() {
  unsigned long pingTime;
  float distance;
  digitalWrite(PIN_SONIC_TRIG, HIGH); // make trigPin output high level lasting for 10μs to triger HC_SR04,
  delayMicroseconds(10);
  digitalWrite(PIN_SONIC_TRIG, LOW);
  pingTime = pulseIn(PIN_SONIC_ECHO, HIGH, SONIC_TIMEOUT); // Wait HC-SR04 returning to the high level and measure out this waitting time
  if (pingTime != 0)
    distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000; // calculate the distance according to the time
  else
    distance = MAX_DISTANCE;
  return distance; // return the distance value
}

void calculateVoltageCompensation() {
  float voltageOffset = 8.4 - getBatteryVoltage();
  speedOffset = voltageOffset * 20;
}

void motorRun(int speedl, int speedr) {
  int dirL = 0, dirR = 0;
  if (speedl > 0) {
    dirL = 0 ^ MOTOR_DIRECTION;
  } else {
    dirL = 1 ^ MOTOR_DIRECTION;
    speedl = -speedl;
  }

  if (speedr > 0) {
    dirR = 1 ^ MOTOR_DIRECTION;
  } else {
    dirR = 0 ^ MOTOR_DIRECTION;
    speedr = -speedr;
  }

  digitalWrite(PIN_DIRECTION_LEFT, dirL);
  digitalWrite(PIN_DIRECTION_RIGHT, dirR);
  analogWrite(PIN_MOTOR_PWM_LEFT, speedl);
  analogWrite(PIN_MOTOR_PWM_RIGHT, speedr);
}


float getBatteryVoltage() {
  pinMode(PIN_BATTERY, INPUT);
  int batteryADC = analogRead(PIN_BATTERY);
  float batteryVoltage = batteryADC / 1023.0 * 5.0 * 4;
  return batteryVoltage;
}
