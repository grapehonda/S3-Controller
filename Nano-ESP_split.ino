#include <Arduino.h>

const int analogPins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};  // 8 analogs
const int switchPins[6] = {2, 3, 4, 5, 6, 7};  // toggle switches
const int buttonPins[2] = {8, 9};  // Walk method (8), SLHold (9)

byte g_fHexOn = 0;
byte PiControllerSwitch = 0;  // 0 = Pi, 255 = Controller
byte ModeSelect = 0;  // 0 = Translate, 255 = Walk
byte BalanceMode = 0;
byte DoubleTravel = 0;
byte DoubleLegLiftHeight = 0;
byte WalkMethod = 0;
byte SLHold = 0;

// Joystick/pot values (0-255)
byte leftFwdBack = 128;
byte leftLeftRight = 128;
byte leftTwist = 128;
byte rightUpDown = 128;
byte rightLeftRight = 128;
byte rightTwist = 128;
byte bodyHeight = 128;
byte gaitSpeed = 128;

// Button debouncing/tracking
unsigned long lastButtonTime[2] = {0, 0};
const unsigned long debounceDelay = 50;  // ms

void setup() {
  Serial.begin(115200);  // To ESP32-S3

  // Pins
  for (int p : analogPins) pinMode(p, INPUT);
  for (int p : switchPins) pinMode(p, INPUT_PULLUP);
  for (int p : buttonPins) pinMode(p, INPUT_PULLUP);

  // Defaults
  g_fHexOn = 0;
  PiControllerSwitch = 0;
  ModeSelect = 0;
  BalanceMode = 0;
  DoubleTravel = 0;
  DoubleLegLiftHeight = 0;
  WalkMethod = 0;
  SLHold = 0;
}

void loop() {
  // Read analogs
  leftFwdBack = map(analogRead(analogPins[0]), 0, 1023, 0, 255);
  leftLeftRight = map(analogRead(analogPins[1]), 0, 1023, 0, 255);
  leftTwist = map(analogRead(analogPins[2]), 0, 1023, 0, 255);
  rightUpDown = map(analogRead(analogPins[3]), 0, 1023, 0, 255);
  rightLeftRight = map(analogRead(analogPins[4]), 0, 1023, 0, 255);
  rightTwist = map(analogRead(analogPins[5]), 0, 1023, 0, 255);
  bodyHeight = map(analogRead(analogPins[6]), 0, 1023, 0, 255);
  gaitSpeed = map(analogRead(analogPins[7]), 0, 1023, 0, 255);

  // Read switches
  g_fHexOn = (digitalRead(switchPins[0]) == LOW) ? 255 : 0;
  PiControllerSwitch = (digitalRead(switchPins[1]) == LOW) ? 255 : 0;
  ModeSelect = (digitalRead(switchPins[2]) == LOW) ? 255 : 0;
  BalanceMode = (digitalRead(switchPins[3]) == LOW) ? 255 : 0;
  DoubleTravel = (digitalRead(switchPins[4]) == LOW) ? 255 : 0;
  DoubleLegLiftHeight = (digitalRead(switchPins[5]) == LOW) ? 255 : 0;

  // Buttons press detection
  bool buttonPressed[2] = {false, false};
  for (int i = 0; i < 2; i++) {
    bool reading = (digitalRead(buttonPins[i]) == LOW);
    if (reading && (millis() - lastButtonTime[i] > debounceDelay)) {
      buttonPressed[i] = true;
      lastButtonTime[i] = millis();
    } else if (!reading) {
      lastButtonTime[i] = 0;
    }
  }

  // Button 0: Walk method toggle
  if (buttonPressed[0]) {
    WalkMethod = (WalkMethod == 0) ? 255 : 0;
  }

  // Button 1: SL Hold toggle
  if (buttonPressed[1]) {
    SLHold = (SLHold == 0) ? 255 : 0;
  }

  // Send trimmed control string to Serial
  char buf_stm[80];
  sprintf(buf_stm, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
          leftFwdBack, leftLeftRight, leftTwist, rightUpDown, rightLeftRight, rightTwist,
          bodyHeight, gaitSpeed, g_fHexOn, PiControllerSwitch, ModeSelect, BalanceMode,
          DoubleTravel, DoubleLegLiftHeight, WalkMethod, SLHold);
  Serial.print("Control:");
  Serial.print(buf_stm);

  delay(50);
}
