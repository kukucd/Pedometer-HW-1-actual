#include <Arduino.h>
#include <math.h>
#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ST7789.h>

void setReports(sh2_SensorId_t reportType, long report_interval);

#define BNO08X_RESET -1

enum menuState {
  StepMenu, //0
  DistanceMenu, //1
  StrideMenu, //2
  AccelerationMenu, //3
  mCount //4
};

menuState menuMode = StepMenu;

volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;
volatile long prevChangeTimeThree = 0;

long debounceTime = 50;

volatile bool menuButtonFlag = false;
volatile bool increaseButtonFlag = false;
volatile bool decreaseButtonFlag = false;

void IRAM_ATTR buttonToChangeMenu(){
  long now = millis();

  if (now > prevChangeTime + debounceTime){
    menuButtonFlag = true;
    prevChangeTime = now;
  }
}

void IRAM_ATTR buttonToIncrease(){
  long now = millis();

  if (now > prevChangeTimeTwo + debounceTime){
    increaseButtonFlag = true;
    prevChangeTimeTwo = now;
  }
}

void IRAM_ATTR buttonToDecrease(){
  long now = millis();

  if (now > prevChangeTimeThree + debounceTime){
    decreaseButtonFlag = true;
    prevChangeTimeThree = now;
  }
}

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240,135);

int steps = 0;

float strideLength = 30.0;
float distance = 0.0;

float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;

void setup(void) {
  Serial.begin(115200);

  while (!Serial)
    delay(10);

  display.init(135,240);
  display.setRotation(3);
  canvas.setTextColor(ST77XX_GREEN);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);

  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), buttonToDecrease, FALLING);

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToIncrease, RISING);

  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangeMenu, RISING);

  Serial.println("Adafruit BNO08x test!");

  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x chip");

    while (1) {
      delay(10);
    }
  }

  Serial.println("BNO08x Found!");

  setReports(SH2_ACCELEROMETER, 10000);
  setReports(SH2_STEP_COUNTER, 10000);
}

void loop() {

  if (menuButtonFlag){
    menuButtonFlag = false;

    menuMode = (menuState)(((int)menuMode + 1) % (int)menuState::mCount);
  }

  if (increaseButtonFlag){
    increaseButtonFlag = false;

    if (menuMode == StrideMenu){
      strideLength += 1.0;
    }
  }

  if (decreaseButtonFlag){
    decreaseButtonFlag = false;

    if (menuMode == StrideMenu){
      strideLength -= 1.0;

      if (strideLength < 1.0){
        strideLength = 1.0;
      }
    }
  }

  if (bno08x.wasReset()) {
    Serial.println("Sensor was reset");

    setReports(SH2_ACCELEROMETER, 10000);
    setReports(SH2_STEP_COUNTER, 10000);
  }

  if (bno08x.getSensorEvent(&sensorValue)) {

    if (sensorValue.sensorId == SH2_STEP_COUNTER) {
      steps = sensorValue.un.stepCounter.steps;

      Serial.print("Steps: ");
      Serial.println(steps);
    }

    if (sensorValue.sensorId == SH2_ACCELEROMETER) {
      accelX = sensorValue.un.accelerometer.x;
      accelY = sensorValue.un.accelerometer.y;
      accelZ = sensorValue.un.accelerometer.z;
    }
  }

  distance = (steps * strideLength) / 12.0;

  canvas.fillScreen(ST77XX_BLACK);

  if (menuMode == StepMenu) {

    canvas.setCursor(0,20);
    canvas.print("Pedometer");

    canvas.setCursor(0,50);
    canvas.print("Steps: ");
    canvas.print(steps);
  }

  if (menuMode == DistanceMenu) {

    canvas.setCursor(0,20);
    canvas.print("Distance");

    canvas.setCursor(0,50);
    canvas.print(distance);
    canvas.print(" ft");
  }

  if (menuMode == StrideMenu) {

    canvas.setCursor(0,20);
    canvas.print("Stride Length");

    canvas.setCursor(0,50);
    canvas.print(strideLength);
    canvas.print(" in");

    canvas.setCursor(0,80);
    canvas.print("D1 +  D0 -");
  }

  if (menuMode == AccelerationMenu) {

    canvas.setCursor(0,20);
    canvas.print("Acceleration");

    canvas.setCursor(0,50);
    canvas.print("X: ");
    canvas.print(accelX);

    canvas.setCursor(0,70);
    canvas.print("Y: ");
    canvas.print(accelY);

    canvas.setCursor(0,90);
    canvas.print("Z: ");
    canvas.print(accelZ);
  }

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);

  delay(100);
}

void setReports(sh2_SensorId_t reportType, long report_interval) {
  Serial.println("Setting desired reports");

  if (!bno08x.enableReport(reportType, report_interval)) {
    Serial.println("Could not enable report");
  }
  else {
    Serial.println("Set report... success!");
  }
}