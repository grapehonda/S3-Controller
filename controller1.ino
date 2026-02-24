#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Rotary.h>
#include <HardwareSerial.h>  // For Serial from Nano

#include "hex_img.h"  // Image headers
#include "Joy.h"
#include "modeselect.h"
#include "bg.h"
#include "splash.h"

// TFT_SCLK 10
// TFT_MOSI 11
// TFT_DC   12
// TFT_RST  13
// TFT_CS   14

// Menu buttons and encoder (INPUT_PULLUP)
#define BTN_SLEG 4
#define BTN_GAIT 5
#define BTN_GP   6
#define ENCODER_SW  7
#define ENCODER_DT  15
#define ENCODER_CLK 16

// Serial from Nano
HardwareSerial SerialNano(1);
const int RX_PIN = 18;  // ESP32 RX (connect to Nano TX)
const int TX_PIN = 17;  // ESP32 TX (connect to Nano RX, optional)
String incoming = "";

// ESP-NOW receiver MAC from your esp-now-tx sketch
uint8_t receiverMAC[] = {0x98, 0xA3, 0x16, 0x8F, 0xF5, 0xE0};

// Positions table for main UI sprites (absolute x, y coordinates for easy adjustment)
// Adjust these values to move sprites around.
// Slider arrows move vertically, but their X is fixed here.
// Joysticks: centers
// Screen is 480w x 320h
const int JOY_LEFT_X = 120;    // Left joystick center
const int JOY_RIGHT_X = 365;   // Right joystick center
const int JOY_Y = 160;         // Shared Y for both joysticks

const int SLIDER_LEFT_X = 5;   // Left slider arrow base
const int SLIDER_RIGHT_X = 475;// Right slider arrow base
const int SLIDER_TOP_Y = 78;   // Top Y for slider calculation (for arrow movement range)
const int SLIDER_RANGE_HEIGHT = 165; // Height range for slider arrows

const int CONTROLLER_X = 20;   // Pi/joystick controller icon
const int CONTROLLER_Y = 10;
const int DBLHT_X = 80;        // Double height icon
const int DBLHT_Y = 10;
const int DBLTRVL_X = 140;     // Double travel icon
const int DBLTRVL_Y = 10;
const int MODE_X = 190;        // Translate/Walk Mode icon
const int MODE_Y = 10;
const int BALNC_X = 300;       // Balance icon
const int BALNC_Y = 10;
const int METHOD_X = 420;      // Method 1/2 icon
const int METHOD_Y = 10;

const int POWER_X = 208;       // Power icon X (centered: (480-64)/2)
const int POWER_Y = 128;       // Power icon Y (centered: (320-64)/2)
const int GAIT_Y = 260;        // Y position for gait text

TFT_eSPI tft = TFT_eSPI();

Rotary encoder = Rotary(ENCODER_CLK, ENCODER_DT);
volatile int deltaAccum = 0;
volatile unsigned long lastISRTime = 0;

// Menu state
enum MenuState { NO_MENU, SLEG_MENU, GAIT_MENU, GP_MENU };
MenuState currentMenu = NO_MENU;
int tempMenuValue = 0;

// Gait names
const char* gaitNames[8] = {"Ripple 6", "Ripple 12", "Quadripple 9", "Tripod 4", "Tripod 6", "Tripod 8", "Wave 12", "Wave 18"};
#define NUM_GAITS 8

// Sequence names
const char* seqNames[11] = {"Off", "Seq 1", "Seq 2", "Seq 3", "Seq 4", "Seq 5", "Seq 6", "Seq 7", "Seq 8", "Seq 9", "Seq 10"};
int numSeqFiles = 10;

// Globals (local copies for UI)
byte g_fHexOn = 0;
byte PiControllerSwitch = 0;  // 0 = Pi, 255 = Controller
byte ModeSelect = 0;  // 0 = Translate, 255 = Walk
byte BalanceMode = 0;
byte DoubleTravel = 0;
byte DoubleLegLiftHeight = 0;
byte WalkMethod = 0;
byte SLHold = 0;
byte GaitPreset = 0;  // 0-7
byte SelectedLeg = 255;  // 255 = off, 0-5 = selected
byte GPSequence = 0;  // 0 = off, 1-10 = sequence

// Joystick/pot values (0-255, read directly)
byte leftFwdBack = 128;
byte leftLeftRight = 128;
byte leftTwist = 128;
byte rightUpDown = 128;
byte rightLeftRight = 128;
byte rightTwist = 128;
byte bodyHeight = 128;
byte gaitSpeed = 128;

// Previous values for update optimization
byte prevLeftFwdBack = 128;
byte prevLeftLeftRight = 128;
byte prevLeftTwist = 128;
byte prevRightUpDown = 128;
byte prevRightLeftRight = 128;
byte prevRightTwist = 128;
byte prevBodyHeight = 128;
byte prevGaitSpeed = 128;
byte prevSwitches[6] = {0,0,0,0,0,0};
byte prevGaitPreset = 0;
byte prev_g_fHexOn = 0;

// Button debouncing/tracking (now only 4 buttons: sleg, gait, gp, enc_sw)
unsigned long lastButtonTime[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;  // ms
const unsigned long longPressDelay = 500;  // ms for long press
bool longPressed[4] = {false, false, false, false};

// Layout variables
int sliderBottomY;
int joyR;
int joyDotR;
int orangeDotR;
int sliderBarH;
int slegY;
int seqHeaderY;
int seqNameY;

// Confirmation for single leg select
bool showConfirm = false;
unsigned long confirmStart = 0;
int confirmedLeg = 0;

// Sprite sizes
const int JOY_SIZE = 156; // JoyL/JoyR square size
const int BG_W = 480;
const int BG_H = 320;

// Hex image positions
int hexX;
int hexY;

// Flipped arrow sprite
uint16_t flippedarrowsprt[ARROWSPRT_WIDTH * ARROWSPRT_HEIGHT];

// Flipped joystick sprite
uint16_t flippedjoysprt[JOY_SIZE * JOY_SIZE];

// Function to restore background
void restoreBg(int x, int y, int w, int h) {
  for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
      int idx = (y + py) * BG_W + (x + px);
      uint16_t color = pgm_read_word(&bgsprt[idx]);
      tft.drawPixel(x + px, y + py, color);
    }
  }
}

// Function to restore joystick area (small rect over dot)
void restoreJoyArea(int rx, int ry, int rw, int rh, bool isLeft) {
  const uint16_t* jsprt = isLeft ? joysprt : flippedjoysprt;
  int joyStartX = isLeft ? JOY_LEFT_X - JOY_SIZE / 2 : JOY_RIGHT_X - JOY_SIZE / 2;
  int joyStartY = JOY_Y - JOY_SIZE / 2;
  for (int py = 0; py < rh; py++) {
    int screenY = ry + py;
    int sprY = screenY - joyStartY;
    if (sprY < 0 || sprY >= JOY_SIZE) continue;
    for (int px = 0; px < rw; px++) {
      int screenX = rx + px;
      int sprX = screenX - joyStartX;
      if (sprX < 0 || sprX >= JOY_SIZE) continue;
      uint16_t color = pgm_read_word(&jsprt[sprY * JOY_SIZE + sprX]);
      if (color != TFT_BLACK) {
        tft.drawPixel(screenX, screenY, color);
      } else {
        uint16_t bgColor = pgm_read_word(&bgsprt[screenY * BG_W + screenX]);
        tft.drawPixel(screenX, screenY, bgColor);
      }
    }
  }
}

// Function to draw outlined text
void drawOutlinedText(const char* text, int x, int y, uint16_t color, uint16_t outlineColor) {
  tft.setTextColor(outlineColor);
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      tft.setCursor(x + dx, y + dy);
      tft.print(text);
    }
  }
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.print(text);
}

// Function to center outlined text
void printCenteredOutlined(const char* text, int y, uint16_t color = TFT_WHITE, uint16_t outlineColor = TFT_BLACK) {
  int16_t x1 = 0, y1 = 0;
  uint16_t w = tft.textWidth(text);
  uint16_t h = tft.fontHeight();
  int x = (tft.width() - w) / 2;
  drawOutlinedText(text, x, y, color, outlineColor);
}

// Draw method sprite
void drawMethodSprite(byte state) {
  restoreBg(METHOD_X, METHOD_Y, METHOD1_WIDTH, METHOD1_HEIGHT);  // Restore bg
  const uint16_t* sprt = (state == 0) ? method1 : method2;
  int w = METHOD1_WIDTH;
  int h = METHOD1_HEIGHT;
  tft.pushImage(METHOD_X, METHOD_Y, w, h, sprt, TFT_BLACK);
}

// Draw controller sprite (Pi/Joystick)
void drawControllerSprite(byte state) {
  restoreBg(CONTROLLER_X, CONTROLLER_Y, PICON_WIDTH, PICON_HEIGHT);  // Restore bg
  const uint16_t* sprt = (state == 0) ? picon : joycon;
  int w = PICON_WIDTH;
  int h = PICON_HEIGHT;
  tft.pushImage(CONTROLLER_X, CONTROLLER_Y, w, h, sprt, TFT_BLACK);
}

// Draw power sprite
void drawPowerSprite(byte state) {
  int maxW = 64;
  int maxH = 64;
  restoreBg(POWER_X, POWER_Y, maxW, maxH);
  const uint16_t* sprt = (state == 255) ? onsprt : offsprt;
  int w = 64;
  int h = 64;
  int adjX = POWER_X + (maxW - w) / 2;
  int adjY = POWER_Y + (maxH - h) / 2;
  tft.pushImage(adjX, adjY, w, h, sprt, TFT_BLACK);
}

// Draw mode sprite (Translate/Walk)
void drawModeSprite(byte state) {
  int maxW = max(TRANSLATESPT_WIDTH, WALKSPT_WIDTH);
  int maxH = max(TRANSLATESPT_HEIGHT, WALKSPT_HEIGHT);
  restoreBg(MODE_X, MODE_Y, maxW, maxH);
  const uint16_t* sprt = (state == 0) ? translatespt : walkspt;
  int w = (state == 0) ? TRANSLATESPT_WIDTH : WALKSPT_WIDTH;
  int h = (state == 0) ? TRANSLATESPT_HEIGHT : WALKSPT_HEIGHT;
  int adjX = MODE_X + (maxW - w) / 2;
  int adjY = MODE_Y + (maxH - h) / 2;
  tft.pushImage(adjX, adjY, w, h, sprt, TFT_BLACK);
}

// Draw balance sprite
void drawBalncSprite(byte state) {
  if (state == 255) {
    tft.pushImage(BALNC_X, BALNC_Y, BALNC_WIDTH, BALNC_HEIGHT, balnc, TFT_BLACK);
  } else {
    restoreBg(BALNC_X, BALNC_Y, BALNC_WIDTH, BALNC_HEIGHT);
  }
}

// Draw double height sprite
void drawDblhtSprite(byte state) {
  if (state == 255) {
    tft.pushImage(DBLHT_X, DBLHT_Y, DBLHT_WIDTH, DBLHT_HEIGHT, dblht, TFT_BLACK);
  } else {
    restoreBg(DBLHT_X, DBLHT_Y, DBLHT_WIDTH, DBLHT_HEIGHT);
  }
}

// Draw double travel sprite
void drawDbltrvlSprite(byte state) {
  if (state == 255) {
    tft.pushImage(DBLTRVL_X, DBLTRVL_Y, DBLTRVL_WIDTH, DBLTRVL_HEIGHT, dbltrvl, TFT_BLACK);
  } else {
    restoreBg(DBLTRVL_X, DBLTRVL_Y, DBLTRVL_WIDTH, DBLTRVL_HEIGHT);
  }
}

