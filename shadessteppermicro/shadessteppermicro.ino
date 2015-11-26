

#define WRTC
#include <Wire.h>
#ifdef WRTC
#include "RTClib.h"
#endif
#include <AccelStepper.h>

#define LBTNPIN 6
#define RBTNPIN 7

#define LEDPIN 13
#define MAXSPEED 500
#define MAXPOS 11650
#define MIDPOS 5431

#define LIMITPIN 8
#define LIMITISHIT digitalRead(LIMITPIN)==LOW

#define MODE_IDLE 0
#define MODE_JOG 1
#define MODE_OPEN 2
#define MODE_BOUNCE 8
#define MODE_HOME 9
#define MODE_ERROR 10

byte ah = 7;
byte am = 30;
boolean alarmdone = false;



boolean timer1Hit = false; // if timer has been triggered //will be reset at 00.00

int mode = MODE_ERROR;

int lastxspd = 0;

boolean stprsOff = true;
boolean homingDone = false;
boolean forcehome = false;

#ifdef WRTC
//realtimne clock
RTC_DS1307 rtc;
#endif
// Define a stepper and the pins it will use
AccelStepper stepper(AccelStepper::FULL4WIRE, 2, 4, 3, 5); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

void setup()
{

  pinMode(LBTNPIN, INPUT_PULLUP);
  pinMode(RBTNPIN, INPUT_PULLUP);
  pinMode(LIMITPIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
  LedOff();
  Serial.begin(38400);
#ifdef WRTC
  Wire.begin();
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  } else {
    Serial.print("RTC is ready! Time:");    
    PrintDateTime();
    Serial.print("Alarm: ");
    Serial.print(ah);
    Serial.print(":");
    Serial.println(am);
  }
#endif


  stepper.setMaxSpeed(700);
  stepper.setAcceleration(1200);
  Serial.print("waiting for action");
  mode = MODE_IDLE;
  if (LIMITISHIT) {
    FlashLed(5);
    homingDone = true;
    //doMoveUntilLimitFree();
  } else {
    mode = MODE_HOME;
    FlashLed(2);
  }

}



void loop()
{
  long mill = millis();
  int m1000slc = mill % 1000;
  int lval = digitalRead(LBTNPIN);
  int rval = digitalRead(RBTNPIN);
  if (mode != MODE_ERROR && (lval == LOW || rval == LOW )) {
    mode = MODE_JOG;
  }
  if (forcehome) {
    forcehome = false;
    doHome();
  }

  switch (mode) {
    case MODE_ERROR:
      FlashLed(1);
      break;
    case MODE_IDLE:
      if(m1000slc==0){
         DateTime now = rtc.now();
        byte hh = now.hour();
        byte mm = now.minute();
        byte ss = now.second();
        if (!alarmdone) {
          if (hh == ah && mm == am) {
            alarmdone = true;
            Serial.println("ALARM");
            forcehome = true; // do not use mode because we dont trust it
          }
        } else {
          //alarmdone amd jump
          if (hh == 0 && mm == 0) {
            alarmdone = false;
            Serial.println("ALARM RESET");
          }
        };
        FlashLed(1);
      }     
      break;
    case MODE_JOG:
      doJogMode();
      break;
    case MODE_BOUNCE:
      //doMoveUntilLimitFree();//blocking
      break;
    case MODE_OPEN:
      doOpen();
      break;
    case MODE_HOME:
      doHome();//blocking
      break;
  }

}


void doJogMode() {

  stepper.enableOutputs();
  int lval = digitalRead(LBTNPIN);
  int rval = digitalRead(RBTNPIN);
  if (lval == LOW && rval == HIGH) {
    stepper.setSpeed(-MAXSPEED);
    while (lval == LOW) {
      stepper.runSpeed();
      lval = digitalRead(LBTNPIN);
    }
  } else if (lval == HIGH && rval == LOW) {
    stepper.setSpeed(MAXSPEED);
    while (rval == LOW) {
      stepper.runSpeed();
      rval = digitalRead(RBTNPIN);
    }
  }
  stepper.disableOutputs();
  mode = MODE_IDLE;
}

void doOpen() {
  Serial.println("open");
  stepper.enableOutputs();

  stepper.moveTo(MIDPOS);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  stepper.disableOutputs();
  mode = MODE_IDLE;
}

void doHome() {
  stepper.enableOutputs();

  stepper.setSpeed(-MAXSPEED);

  LedOn();
  while (!LIMITISHIT) { //   stepper.setCurrentPosition(2048);
    stepper.runSpeed();
  }
  stepper.setCurrentPosition(0);
  FlashLed(2);
  stepper.setSpeed(MAXSPEED);
  Serial.print("curpos");
  Serial.println(stepper.currentPosition());
  while (stepper.currentPosition() < 1024) {
    stepper.runSpeed();
  }
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(1200);

  stepper.moveTo(MIDPOS);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  mode = MODE_IDLE;
  FlashLed(2);
  LedOff();
  stepper.disableOutputs();
  homingDone = true;
}


void doMoveToMidPos() {

  stepper.moveTo(MIDPOS);
  while (stepper.distanceToGo() != 0 && !(LIMITISHIT)) {
    stepper.run();
  }
}

void LedOn() {
  digitalWrite(LEDPIN, HIGH);
}
void LedOff() {
  digitalWrite(LEDPIN, LOW);
}

void FlashLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LEDPIN, HIGH);
    delay(100);
    digitalWrite(LEDPIN, LOW);
    delay(200);
  }

}
#ifdef WRTC
void PrintDateTime() {

  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void PrintDateTime(DateTime now) {

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}
#endif



void xdoMoveUntilLimitFree() {
  stepper.enableOutputs();
  stepper.setSpeed(300);
  while (LIMITISHIT) {
    stepper.runSpeed();
  }
  stepper.setCurrentPosition(0);
  stepper.move(100);
  stepper.setCurrentPosition(0);
  mode = MODE_IDLE;
}


