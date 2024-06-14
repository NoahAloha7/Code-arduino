//door Noah Berghout :)

#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>
#include <Servo.h>
// Libraries en zo

Servo servod;

bool isOpen = true;
bool isMoving = false;

int ledl = 13;
int ledh = 12;
int button = 11;
int buttonState = 0;

int temperatuur = 20;

static unsigned long cooldownStartTime = 0;
static bool servoTurned = false;
static bool inCooldown = false;

const int stepsPerRevolution = 2048;
// variabelen

Stepper myStepper(stepsPerRevolution, 4, 5, 9, 10);
// pins stappenmotor

VR myVR(2, 3);

uint8_t records[7];
uint8_t buf[64];

#define aanRecord    (0)
#define uitRecord    (1)
#define openRecord   (2)
#define dichtRecord  (3)
#define warmerRecord (4)
#define kouderRecord (5)
#define helpRecord   (6)
// Commando's invoegen

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1
#define echoPin 6
#define trigPin 7
// pins Display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void printSignature(uint8_t *buf, int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (buf[i] > 0x19 && buf[i] < 0x7F) {
      Serial.write(buf[i]);
    } else {
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

void printVR(uint8_t *buf) {
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if (buf[0] == 0xFF) {
    Serial.print("NONE");
  } else if (buf[0] & 0x80) {
    Serial.print("UG ");
    Serial.print(buf[0] & (~0x80), DEC);
  } else {
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if (buf[3] > 0) {
    printSignature(buf + 4, buf[3]);
  } else {
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}
// zooi die nodig is voor de spraakherkenning

void setup() {

  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(45, 2);
  display.println("20");
  display.display();
// startgegevens display

  myVR.begin(9600);
  Serial.println("Elechouse Voice Recognition V3 Module\r\nControl LED sample");

  pinMode(ledl, OUTPUT);
  pinMode(ledh, OUTPUT);
  pinMode(button, INPUT);
// of led en knop in- of output is

  if (myVR.clear() == 0) {
    Serial.println("Recognizer cleared.");
  } else {
    Serial.println("Not find VoiceRecognitionModule.");
    Serial.println("Please check connection and restart Arduino.");
    while (1);
  }

  if (myVR.load((uint8_t)aanRecord) >= 0) Serial.println("aan loaded");
  if (myVR.load((uint8_t)uitRecord) >= 0) Serial.println("uit loaded");
  if (myVR.load((uint8_t)openRecord) >= 0) Serial.println("open loaded");
  if (myVR.load((uint8_t)dichtRecord) >= 0) Serial.println("dicht loaded");
  if (myVR.load((uint8_t)warmerRecord) >= 0) Serial.println("warmer loaded");
  if (myVR.load((uint8_t)kouderRecord) >= 0) Serial.println("kouder loaded");
  if (myVR.load((uint8_t)helpRecord) >= 0) Serial.println("help loaded");

  servod.attach(8);
  servod.write(25);
}
// bericht in monitor of ze goed zijn geüpload

void loop() {
  int ret = myVR.recognize(buf, 50);
  if (ret > 0) {
    switch (buf[1]) {
      case openRecord:
        if (!isMoving && !isOpen) {
          isMoving = true;
          myStepper.setSpeed(15);
          myStepper.step(-0.8 * stepsPerRevolution);
          isOpen = true;
          isMoving = false;
        }
        break; // stappenmotor draait iets minder dan 1x met de klok mee als je 'open' zegt
      case dichtRecord:
        if (!isMoving && isOpen) {
          isMoving = true;
          myStepper.setSpeed(15);
          myStepper.step(0.8 * stepsPerRevolution);
          isOpen = false;
          isMoving = false;
        }
        break; // stappenmotor draait iets minder dan 1x tegen de klok in als je 'dicht' zegt
      case aanRecord:
        digitalWrite(ledl, HIGH);
        break; // gele led gaat aan bij 'aan' commando
      case uitRecord:
        digitalWrite(ledl, LOW);
        break; // gele led gaat uit bij 'uit' commando
      case helpRecord:
        digitalWrite(ledh, HIGH);
        break; // rode led gaat aan bij 'help' commando
      case warmerRecord:
        temperatuur += 2;
        display.clearDisplay();
        display.setCursor(45, 2);
        display.print(temperatuur);
        display.display();
        break; // getal display gaat met 2 omhoog bij 'warmer' commando
      case kouderRecord:
        temperatuur -= 2;
        display.clearDisplay();
        display.setCursor(45, 2);
        display.print(temperatuur);
        display.display();
        break; // getal gaat met 2 omlaag bij 'kouder' commando
      default:
        Serial.println("Record function undefined");
        break;
    }
    printVR(buf);
  }

  buttonState = digitalRead(button);
  if (buttonState == HIGH) {
    digitalWrite(ledh, LOW);
  }
  // knopje laat led uit gaan

  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;

  if (distance < 15 && !servoTurned && !inCooldown) {
    servod.write(110);
    cooldownStartTime = millis();
    servoTurned = true;
    inCooldown = true;
  }

  if (inCooldown && (millis() - cooldownStartTime >= 10000)) {
    inCooldown = false;
  }

  if (distance < 15 && servoTurned && !inCooldown) {
    servod.write(25);
    cooldownStartTime = millis();
    servoTurned = false;
    inCooldown = true;
  }
}
// code voor de servo die het deurtje aanstuurt
