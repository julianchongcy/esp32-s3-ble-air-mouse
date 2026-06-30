#include <Wire.h>
#include <MPU6050_light.h>
#include <HijelHID_BLEMouse.h>

#define LEFT_BUTTON 1
#define RIGHT_BUTTON 2
#define MODE_BUTTON 0

MPU6050 mpu(Wire);
HijelBLEMouse mouse("Julian Air Mouse", "Julian");

float deadzone = 6.0;

float sensitivityX = 0.9;
float sensitivityY = 3.0;

float smoothX = 0;
float smoothY = 0;

bool scrollMode = false;

float scrollSensitivity = 0.30;
float scrollDeadzone = 10.0;
float scrollVelocity = 0;

unsigned long lastScrollTime = 0;
int scrollDelay = 80;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(8, 9);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(MODE_BUTTON, INPUT_PULLUP);

  byte status = mpu.begin();
  while (status != 0) {
    Serial.println("MPU6050 not detected");
    delay(1000);
  }

  Serial.println("Keep device still. Calibrating...");
  delay(1000);
  mpu.calcOffsets();
  Serial.println("Calibration done.");

  mouse.setBatteryLevel(100);
  mouse.begin();

  Serial.println("Julian Air Mouse advertising...");
}

void loop() {
  mpu.update();

  if (!mouse.isPaired()) {
    Serial.println("Waiting for BLE pairing...");
    delay(500);
    return;
  }

  float x = mpu.getAngleX();
  float y = mpu.getAngleY();

  int moveX = 0;
  int moveY = 0;

  float absY = abs(y);

  if (y > deadzone) {
    moveX = pow(absY - deadzone, 1.4) * sensitivityX;
  } else if (y < -deadzone) {
    moveX = -pow(absY - deadzone, 1.4) * sensitivityX;
  }

  if (x > deadzone) {
    moveY = (x - deadzone) * sensitivityY;
  } else if (x < -deadzone) {
    moveY = (x + deadzone) * sensitivityY;
  }

  moveY = -moveY;

  moveX = constrain(moveX, -45, 45);
  moveY = constrain(moveY, -35, 35);

  smoothX = smoothX * 0.55 + moveX * 0.45;
  smoothY = smoothY * 0.55 + moveY * 0.45;

  if (!scrollMode) {
    mouse.move((int)smoothX, (int)smoothY);
  } else {
    if (millis() - lastScrollTime > scrollDelay) {

      if (abs(x) < scrollDeadzone) {
        scrollVelocity *= 0.8;
      } else {
        scrollVelocity = scrollVelocity * 0.75 + (x * scrollSensitivity) * 0.25;
      }

      int scrollAmount = (int)scrollVelocity;

      if (scrollAmount > 1) scrollAmount = 1;
      if (scrollAmount < -1) scrollAmount = -1;

      if (scrollAmount != 0) {
        mouse.scroll(scrollAmount);
        lastScrollTime = millis();
      }
    }
  }

  if (digitalRead(LEFT_BUTTON) == LOW) {
    mouse.press(MouseButton::Left);
  } else {
    mouse.release(MouseButton::Left);
  }

  if (digitalRead(RIGHT_BUTTON) == LOW) {
    mouse.press(MouseButton::Right);
  } else {
    mouse.release(MouseButton::Right);
  }

  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(MODE_BUTTON);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    scrollMode = !scrollMode;
    scrollVelocity = 0;

    Serial.print("Scroll Mode: ");
    Serial.println(scrollMode);

    delay(300);
  }

  lastButtonState = currentButtonState;

  Serial.print("X: ");
  Serial.print(x);
  Serial.print(" | Y: ");
  Serial.print(y);
  Serial.print(" | moveX: ");
  Serial.print(moveX);
  Serial.print(" | moveY: ");
  Serial.print(moveY);
  Serial.print(" | scrollMode: ");
  Serial.println(scrollMode);

  delay(20);
}
