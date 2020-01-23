#include <AccelStepper.h> // Advanced stepper motor control
#include <Wire.h> // i2c communication


#define motorPin1  8      // IN1 on the ULN2003 driver
#define motorPin2  9      // IN2 on the ULN2003 driver
#define motorPin3  10     // IN3 on the ULN2003 driver
#define motorPin4  11     // IN4 on the ULN2003 driver

#define MotorInterfaceType 8

AccelStepper stepper = AccelStepper(MotorInterfaceType, motorPin1, motorPin3, motorPin2, motorPin4);


int alarmState = -1;

unsigned long timers[2];

void pressPowerON();
void debug();

void setup() {
  // Prepariation
  Serial.begin(115200);

  // SLAVE!
  Wire.begin(8);                /* join i2c bus with address 8 */
  Wire.onReceive(receiveEvent); /* register receive event */

  // Stepper motor
  stepper.setMaxSpeed(1000);
}

void loop() {
  debug();
  if (alarmState == 1) {
    pressPowerON();
    while (alarmState == 1) {
      delay(1);
      // waiting for a 0...
    }
  }
  delay(1000);
}

//==============================================================
//                i2c Communication
//==============================================================
// function that executes whenever data is received from master
void receiveEvent() {
  while (Wire.available() > 0) {
    alarmState = (int)Wire.read() - 48;    // Wire.read() returns a char => (0=48,1=49,2=50....)
  }
}


//==============================================================
//                Stepper control
//==============================================================
void pressPowerON() {
  // Press
  stepper.setCurrentPosition(0);
  while (stepper.currentPosition() != 250) {
    stepper.setSpeed(500);
    stepper.runSpeed();
  }
  delay(100);
  // Release
  while (stepper.currentPosition() != 0) {
    stepper.setSpeed(-500);
    stepper.runSpeed();
  }
  delay(100);
}

//==============================================================
//                    DEBUG
//==============================================================
void debug() {
  if (alarmState == 1) {
    Serial.println("Alarm is on! Wake up you lazy monkey... ring ring ding dong heuheueu");
  } else {
    Serial.println("ZZZzzzzz....");
  }
}
