// Draw gait type text (at GAIT_Y)
void drawGaitType(byte preset) {
  tft.setTextFont(2);
  tft.setTextSize(1);
  const char* oldLabel = gaitNames[prevGaitPreset];
  const char* label = gaitNames[preset];
  int16_t x1 = 0, y1 = 0;
  uint16_t oldW = tft.textWidth(oldLabel);
  uint16_t w = tft.textWidth(label);
  uint16_t h = tft.fontHeight();
  int tx = (tft.width() - w) / 2;
  int oldTx = (tft.width() - oldW) / 2;
  int ty = GAIT_Y;
  int padding = 7;

  // Erase old text by restoring bg
  restoreBg(oldTx - padding, GAIT_Y, oldW + padding * 2, h);

  // Draw new text directly (no grey bubble)
  drawOutlinedText(label, tx, ty, TFT_WHITE, TFT_BLACK);
  tft.setTextFont(0);  // Reset
}

// Update left joystick (erase old, draw new)
void updateLeftJoystick(byte fwdBack, byte leftRight, byte twist, byte prevFwdBack, byte prevLeftRight, byte prevLeftTwist) {
  bool changed = (fwdBack != prevFwdBack || leftRight != prevLeftRight || twist != prevLeftTwist);
  if (changed) {
    // Compute old white pos
    int oldWhiteX = JOY_LEFT_X + map(prevLeftRight, 0, 255, -joyR, joyR);
    int oldWhiteY = JOY_Y + map(prevFwdBack, 0, 255, joyR, -joyR);
    // Old orange
    float oldAngle = 180 + (prevLeftTwist / 255.0) * 180;
    int oldOrangeX = JOY_LEFT_X + joyR * cos(oldAngle * PI / 180);
    int oldOrangeY = JOY_Y + joyR * sin(oldAngle * PI / 180);

    // Restore old white area
    restoreJoyArea(oldWhiteX - joyDotR - 1, oldWhiteY - joyDotR - 1, joyDotR * 2 + 2, joyDotR * 2 + 2, true);

    // Restore old orange area
    restoreJoyArea(oldOrangeX - orangeDotR - 1, oldOrangeY - orangeDotR - 1, orangeDotR * 2 + 2, orangeDotR * 2 + 2, true);

    // Draw new white dot
    int newWhiteX = JOY_LEFT_X + map(leftRight, 0, 255, -joyR, joyR);
    int newWhiteY = JOY_Y + map(fwdBack, 0, 255, joyR, -joyR);
    tft.fillCircle(newWhiteX, newWhiteY, joyDotR, TFT_WHITE);

    // Twist orange dot
    float angle = 180 + (twist / 255.0) * 180;
    int newX = JOY_LEFT_X + joyR * cos(angle * PI / 180);
    int newY = JOY_Y + joyR * sin(angle * PI / 180);
    tft.fillCircle(newX, newY, orangeDotR, TFT_ORANGE);
  }
}

// Update right joystick (similar)
void updateRightJoystick(byte upDown, byte leftRight, byte twist, byte prevUpDown, byte prevLeftRight, byte prevRightTwist) {
  bool changed = (upDown != prevUpDown || leftRight != prevLeftRight || twist != prevRightTwist);
  if (changed) {
    // Compute old white pos
    int oldWhiteX = JOY_RIGHT_X + map(prevLeftRight, 0, 255, -joyR, joyR);
    int oldWhiteY = JOY_Y + map(prevUpDown, 0, 255, joyR, -joyR);
    // Old orange
    float oldAngle = 180 + (prevRightTwist / 255.0) * 180;
    int oldOrangeX = JOY_RIGHT_X + joyR * cos(oldAngle * PI / 180);
    int oldOrangeY = JOY_Y + joyR * sin(oldAngle * PI / 180);

    // Restore old white area
    restoreJoyArea(oldWhiteX - joyDotR - 1, oldWhiteY - joyDotR - 1, joyDotR * 2 + 2, joyDotR * 2 + 2, false);

    // Restore old orange area
    restoreJoyArea(oldOrangeX - orangeDotR - 1, oldOrangeY - orangeDotR - 1, orangeDotR * 2 + 2, orangeDotR * 2 + 2, false);

    // Draw new white dot
    int newWhiteX = JOY_RIGHT_X + map(leftRight, 0, 255, -joyR, joyR);
    int newWhiteY = JOY_Y + map(upDown, 0, 255, joyR, -joyR);
    tft.fillCircle(newWhiteX, newWhiteY, joyDotR, TFT_WHITE);

    // Twist orange dot
    float angle = 180 + (twist / 255.0) * 180;
    int newX = JOY_RIGHT_X + joyR * cos(angle * PI / 180);
    int newY = JOY_Y + joyR * sin(angle * PI / 180);
    tft.fillCircle(newX, newY, orangeDotR, TFT_ORANGE);
  }
}

// Update slider (left=true for body, false for gait)
void updateSlider(bool isLeft, byte value, byte prevValue) {
  bool changed = (value != prevValue);
  if (changed) {
    int old_y = map(prevValue, 0, 255, sliderBottomY, SLIDER_TOP_Y);  // Invert: 0 at bottom, 255 at top
    int new_y = map(value, 0, 255, sliderBottomY, SLIDER_TOP_Y);
    int x = isLeft ? SLIDER_LEFT_X : SLIDER_RIGHT_X - ARROWSPRT_WIDTH;
    const uint16_t* sprt = isLeft ? arrowsprt : flippedarrowsprt;
    int w = ARROWSPRT_WIDTH;
    int h = ARROWSPRT_HEIGHT;

    // Erase old
    int padding = 1;
    int eraseW = w + padding * 2;
    int eraseH = h + padding * 2;
    int eraseY = old_y - h / 2 - padding;
    int eraseX = isLeft ? SLIDER_LEFT_X - padding : SLIDER_RIGHT_X - w - padding;
    restoreBg(eraseX, eraseY, eraseW, eraseH);  // Restore bg

    // Draw new sprite
    tft.pushImage(x, new_y - h / 2, w, h, sprt, TFT_BLACK);
  }
}

// Draw hexapod (now using image with overlays)
void drawHexapod(int selected = -1) {
  tft.pushImage(hexX, hexY, HEX_WIDTH, HEX_HEIGHT, Hex);  // Repush image to erase old overlays
  // Overlay dynamic elements (adjusted positions)
  // Positions: RR (1), RM (2), RF (3), LR (4), LM (5), LF (6)
  int legPositions[6][2] = {
    {300, 220}, // RR
    {300, 110}, // RM
    {300, 20}, // RF
    {50, 220}, // LR
    {50, 110}, // LM
    {50, 20} // LF
  };

  int jointR = 8; // Highlight radius

  for (int i = 0; i < 6; i++) {
    uint16_t color = (i + 1 == selected) ? TFT_RED : TFT_WHITE;
    int x = legPositions[i][0] + hexX;
    int y = legPositions[i][1] + hexY;

    // Highlight circle on leg
    tft.fillCircle(x, y, jointR, color);

    // Label number
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(color);
    char legNum[2];
    sprintf(legNum, "%d", i + 1);
    uint16_t w = tft.textWidth(legNum);
    uint16_t h = tft.fontHeight();
    int labelTx = x - w / 2;
    int labelTy = y + jointR + 5; // Below circle
    tft.setCursor(labelTx, labelTy);
    tft.print(legNum);
    tft.setTextFont(0);
  }
}

void rotaryISR() {
  unsigned char result = encoder.process();
  if (result == DIR_CW) {
    deltaAccum++;
  } else if (result == DIR_CCW) {
    deltaAccum--;
  }
}

void drawGaitMenu(int selected) {
  uint16_t boxColor = tft.color565(32, 32, 32);
  tft.fillRect(40, 60, 400, 200, boxColor);
  tft.drawRect(40, 60, 400, 200, TFT_WHITE);
  int visible = 5;
  int topIndex = max(0, selected - visible / 2);
  tft.setTextFont(2);
  tft.setTextSize(1);
  for (int i = 0; i < visible; i++) {
    int idx = topIndex + i;
    if (idx >= NUM_GAITS) break;
    int y = 80 + i * 30;
    if (idx == selected) {
      tft.fillRect(50, y - 15, 380, 25, TFT_WHITE);
      tft.setTextColor(TFT_BLACK);
    } else {
      tft.setTextColor(TFT_WHITE);
    }
    tft.setCursor(60, y - 10);
    tft.print(gaitNames[idx]);
  }
  tft.setTextColor(TFT_WHITE);
}

// Function to process incoming Serial from Nano
void processIncoming(String msg) {
  if (msg.startsWith("Control:")) {
    String data = msg.substring(8);
    data.trim();
    char buf[80];
    data.toCharArray(buf, 80);
    int v[16];
    int cnt = sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                     &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7],
                     &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);
    if (cnt == 16) {
      leftFwdBack = v[0];
      leftLeftRight = v[1];
      leftTwist = v[2];
      rightUpDown = v[3];
      rightLeftRight = v[4];
      rightTwist = v[5];
      bodyHeight = v[6];
      gaitSpeed = v[7];
      g_fHexOn = v[8];
      PiControllerSwitch = v[9];
      ModeSelect = v[10];
      BalanceMode = v[11];
      DoubleTravel = v[12];
      DoubleLegLiftHeight = v[13];
      WalkMethod = v[14];
      SLHold = v[15];
    }
  }
}

void setup() {
  Serial.begin(115200); // For debug

  // Serial from Nano
  SerialNano.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Menu buttons and encoder
  const int menuPins[4] = {BTN_SLEG, BTN_GAIT, BTN_GP, ENCODER_SW};
  for (int p : menuPins) pinMode(p, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  encoder.begin();
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), rotaryISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), rotaryISR, CHANGE);

  // TFT init
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);  // This swaps the two bytes per pixel on the fly
  tft.fillScreen(TFT_BLACK);

  // ESP-NOW init
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setChannel(1);
  while (!WiFi.STA.started()) delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (1);
  }

  // TFT splash
  tft.pushImage(0, 0, SPLASH_WIDTH, SPLASH_HEIGHT, splash);

  // Calculate layout (minimal)
  sliderBottomY = SLIDER_TOP_Y + SLIDER_RANGE_HEIGHT;
  joyR = tft.height() / 6;
  joyDotR = joyR / 8;
  orangeDotR = joyDotR / 2;
  sliderBarH = joyDotR * 4;  // Slightly bigger pointers
  slegY = 30; // Fixed, adjust as needed
  seqHeaderY = 40;
  seqNameY = 60;

