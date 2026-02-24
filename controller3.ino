  // Hex image centered
  hexX = (tft.width() - HEX_WIDTH) / 2;
  hexY = (tft.height() - HEX_HEIGHT) / 2;

  // Defaults
  g_fHexOn = 0;
  PiControllerSwitch = 0;
  ModeSelect = 0;
  BalanceMode = 0;
  DoubleTravel = 0;
  DoubleLegLiftHeight = 0;
  WalkMethod = 0;
  SLHold = 0;
  GaitPreset = 0;
  SelectedLeg = 255;
  GPSequence = 0;

  // Create flipped arrow sprite
  for (int i = 0; i < ARROWSPRT_WIDTH * ARROWSPRT_HEIGHT; i++) {
    flippedarrowsprt[i] = pgm_read_word(&arrowsprt[ARROWSPRT_WIDTH * ARROWSPRT_HEIGHT - 1 - i]);
  }

  // Create flipped joystick sprite
  for (int i = 0; i < JOY_SIZE * JOY_SIZE; i++) {
    flippedjoysprt[i] = pgm_read_word(&joysprt[JOY_SIZE * JOY_SIZE - 1 - i]);
  }

  delay(1000);

  // Draw composed background: bg sprite + other sprites
  tft.pushImage(0, 0, BG_W, BG_H, bgsprt);
  // Draw joystick circles (centered)
  tft.pushImage(JOY_LEFT_X - JOY_SIZE / 2, JOY_Y - JOY_SIZE / 2, JOY_SIZE, JOY_SIZE, joysprt, TFT_BLACK);
  tft.pushImage(JOY_RIGHT_X - JOY_SIZE / 2, JOY_Y - JOY_SIZE / 2, JOY_SIZE, JOY_SIZE, flippedjoysprt, TFT_BLACK);

  // Draw main UI on top
  byte initialStates[6] = {WalkMethod, PiControllerSwitch, ModeSelect, BalanceMode, DoubleTravel, DoubleLegLiftHeight};
  drawMethodSprite(WalkMethod);
  drawControllerSprite(PiControllerSwitch);
  drawPowerSprite(g_fHexOn);
  drawModeSprite(ModeSelect);
  drawBalncSprite(BalanceMode);
  drawDblhtSprite(DoubleLegLiftHeight);
  drawDbltrvlSprite(DoubleTravel);
  drawGaitType(GaitPreset);

  // Draw initial white dots
  int initialWhiteLX = JOY_LEFT_X + map(128, 0, 255, -joyR, joyR);
  int initialWhiteLY = JOY_Y + map(128, 0, 255, joyR, -joyR);
  tft.fillCircle(initialWhiteLX, initialWhiteLY, joyDotR, TFT_WHITE);

  int initialWhiteRX = JOY_RIGHT_X + map(128, 0, 255, -joyR, joyR);
  int initialWhiteRY = JOY_Y + map(128, 0, 255, joyR, -joyR);
  tft.fillCircle(initialWhiteRX, initialWhiteRY, joyDotR, TFT_WHITE);

  // Draw initial orange dots
  float initialAngle = 180 + (128 / 255.0) * 180;
  int leftOrangeX = JOY_LEFT_X + joyR * cos(initialAngle * PI / 180);
  int leftOrangeY = JOY_Y + joyR * sin(initialAngle * PI / 180);
  tft.fillCircle(leftOrangeX, leftOrangeY, orangeDotR, TFT_ORANGE);

  int rightOrangeX = JOY_RIGHT_X + joyR * cos(initialAngle * PI / 180);
  int rightOrangeY = JOY_Y + joyR * sin(initialAngle * PI / 180);
  tft.fillCircle(rightOrangeX, rightOrangeY, orangeDotR, TFT_ORANGE);

  // Initial sliders (with prev as 0 to force changed)
  updateSlider(true, bodyHeight, 0);
  updateSlider(false, gaitSpeed, 0);
  prevBodyHeight = bodyHeight;
  prevGaitSpeed = gaitSpeed;
}

void loop() {
  // Read and process incoming from Nano
  while (SerialNano.available() > 0) {
    char c = SerialNano.read();
    incoming += c;
    if (c == '\n') {
      processIncoming(incoming);
      incoming = "";
    }
  }

  // Update status bars and method if changed
  byte currentSwitches[6] = {WalkMethod, PiControllerSwitch, ModeSelect, BalanceMode, DoubleTravel, DoubleLegLiftHeight};
  if (currentMenu == NO_MENU) {
    if (currentSwitches[0] != prevSwitches[0]) {
      drawMethodSprite(currentSwitches[0]);
      prevSwitches[0] = currentSwitches[0];
    }
    if (currentSwitches[1] != prevSwitches[1]) {
      drawControllerSprite(currentSwitches[1]);
      prevSwitches[1] = currentSwitches[1];
    }
    if (currentSwitches[2] != prevSwitches[2]) {
      drawModeSprite(currentSwitches[2]);
      prevSwitches[2] = currentSwitches[2];
    }
    if (currentSwitches[3] != prevSwitches[3]) {
      drawBalncSprite(currentSwitches[3]);
      prevSwitches[3] = currentSwitches[3];
    }
    if (currentSwitches[4] != prevSwitches[4]) {
      drawDbltrvlSprite(currentSwitches[4]);
      prevSwitches[4] = currentSwitches[4];
    }
    if (currentSwitches[5] != prevSwitches[5]) {
      drawDblhtSprite(currentSwitches[5]);
      prevSwitches[5] = currentSwitches[5];
    }
    if (g_fHexOn != prev_g_fHexOn) {
      drawPowerSprite(g_fHexOn);
      prev_g_fHexOn = g_fHexOn;
    }
  }

  // Buttons press detection (only 4 menu buttons)
  bool menuButtonPressed[4] = {false};
  for (int i = 0; i < 4; i++) {
    const int menuButtonPins[4] = {BTN_SLEG, BTN_GAIT, BTN_GP, ENCODER_SW};
    bool reading = (digitalRead(menuButtonPins[i]) == LOW);
    if (reading) {
      unsigned long currentTime = millis();
      if (currentTime - lastButtonTime[i] > debounceDelay) {
        if (lastButtonTime[i] == 0) {  // First press
          menuButtonPressed[i] = true;
          lastButtonTime[i] = currentTime;
        }
      }
    } else {
      lastButtonTime[i] = 0;
      longPressed[i] = false;
    }
  }

  // Long press detection for menu buttons
  int menuButtons[3] = {0, 1, 2};  // indices for SLEG, GAIT, GP
  MenuState menuStates[3] = {SLEG_MENU, GAIT_MENU, GP_MENU};
  for (int j = 0; j < 3; j++) {
    int i = menuButtons[j];
    const int menuButtonPins[4] = {BTN_SLEG, BTN_GAIT, BTN_GP, ENCODER_SW};
    bool reading = (digitalRead(menuButtonPins[i]) == LOW);
    if (reading && lastButtonTime[i] > 0 && (millis() - lastButtonTime[i] > longPressDelay) && !longPressed[i]) {
      if (currentMenu == menuStates[j]) {
        MenuState oldMenu = currentMenu;
        currentMenu = NO_MENU;
        tft.pushImage(0, 0, BG_W, BG_H, bgsprt);  // Redraw bg
        tft.pushImage(JOY_LEFT_X - JOY_SIZE / 2, JOY_Y - JOY_SIZE / 2, JOY_SIZE, JOY_SIZE, joysprt, TFT_BLACK);
        tft.pushImage(JOY_RIGHT_X - JOY_SIZE / 2, JOY_Y - JOY_SIZE / 2, JOY_SIZE, JOY_SIZE, flippedjoysprt, TFT_BLACK);
        byte currentStates[6] = {WalkMethod, PiControllerSwitch, ModeSelect, BalanceMode, DoubleTravel, DoubleLegLiftHeight};
        drawMethodSprite(WalkMethod);
        drawControllerSprite(PiControllerSwitch);
        drawPowerSprite(g_fHexOn);
        drawModeSprite(ModeSelect);
        drawBalncSprite(BalanceMode);
        drawDblhtSprite(DoubleLegLiftHeight);
        drawDbltrvlSprite(DoubleTravel);
        drawGaitType(GaitPreset);
        // Force draw current dots
        int whiteLX = JOY_LEFT_X + map(leftLeftRight, 0, 255, -joyR, joyR);
        int whiteLY = JOY_Y + map(leftFwdBack, 0, 255, joyR, -joyR);
        tft.fillCircle(whiteLX, whiteLY, joyDotR, TFT_WHITE);

        int whiteRX = JOY_RIGHT_X + map(rightLeftRight, 0, 255, -joyR, joyR);
        int whiteRY = JOY_Y + map(rightUpDown, 0, 255, joyR, -joyR);
        tft.fillCircle(whiteRX, whiteRY, joyDotR, TFT_WHITE);

        float angleL = 180 + (leftTwist / 255.0) * 180;
        int orangeLX = JOY_LEFT_X + joyR * cos(angleL * PI / 180);
        int orangeLY = JOY_Y + joyR * sin(angleL * PI / 180);
        tft.fillCircle(orangeLX, orangeLY, orangeDotR, TFT_ORANGE);

        float angleR = 180 + (rightTwist / 255.0) * 180;
        int orangeRX = JOY_RIGHT_X + joyR * cos(angleR * PI / 180);
        int orangeRY = JOY_Y + joyR * sin(angleR * PI / 180);
        tft.fillCircle(orangeRX, orangeRY, orangeDotR, TFT_ORANGE);

        // Force sliders
        updateSlider(true, bodyHeight, 0);
        updateSlider(false, gaitSpeed, 0);
        prevBodyHeight = bodyHeight;
        prevGaitSpeed = gaitSpeed;

        longPressed[i] = true;
        if (oldMenu == SLEG_MENU) {
          SelectedLeg = 255; // Reset single leg on exit
        } else if (oldMenu == GP_MENU) {
          GPSequence = 0;
        }
      }
    }
  }

