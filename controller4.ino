  // Encoder delta
  noInterrupts();
  int delta = deltaAccum;
  deltaAccum = 0;
  interrupts();

  // Handle buttons and menus
  if (currentMenu == NO_MENU) {
    // Update UI only if changed
    if (leftFwdBack != prevLeftFwdBack || leftLeftRight != prevLeftLeftRight || leftTwist != prevLeftTwist) {
      updateLeftJoystick(leftFwdBack, leftLeftRight, leftTwist, prevLeftFwdBack, prevLeftLeftRight, prevLeftTwist);
      prevLeftFwdBack = leftFwdBack;
      prevLeftLeftRight = leftLeftRight;
      prevLeftTwist = leftTwist;
    }
    if (rightUpDown != prevRightUpDown || rightLeftRight != prevRightLeftRight || rightTwist != prevRightTwist) {
      updateRightJoystick(rightUpDown, rightLeftRight, rightTwist, prevRightUpDown, prevRightLeftRight, prevRightTwist);
      prevRightUpDown = rightUpDown;
      prevRightLeftRight = rightLeftRight;
      prevRightTwist = rightTwist;
    }
    if (bodyHeight != prevBodyHeight) {
      updateSlider(true, bodyHeight, prevBodyHeight);
      prevBodyHeight = bodyHeight;
    }
    if (gaitSpeed != prevGaitSpeed) {
      updateSlider(false, gaitSpeed, prevGaitSpeed);
      prevGaitSpeed = gaitSpeed;
    }
  }

  // Button 0: S-Leg menu (was index 1)
  if (menuButtonPressed[0] && currentMenu == NO_MENU) {
    currentMenu = SLEG_MENU;
    SelectedLeg = 255;
    tempMenuValue = 1;
    tft.fillScreen(TFT_BLACK);  // Full clear for menu
    tft.pushImage(hexX, hexY, HEX_WIDTH, HEX_HEIGHT, Hex);  // Push centered hexapod image
    drawHexapod(tempMenuValue);  // Overlay highlights
    tft.setTextFont(2);
    tft.setTextSize(1);
    char buf[20];
    sprintf(buf, "Select Leg: %d", tempMenuValue);
    printCenteredOutlined(buf, slegY);
    tft.setTextFont(0);
  }

  // Button 1: Gait menu (was index 3)
  if (menuButtonPressed[1] && currentMenu == NO_MENU) {
    currentMenu = GAIT_MENU;
    tempMenuValue = GaitPreset;
    tft.fillScreen(TFT_BLACK);  // Full clear for menu
    drawGaitMenu(tempMenuValue);
  }

  // Button 2: GP menu (was index 4)
  if (menuButtonPressed[2] && currentMenu == NO_MENU) {
    currentMenu = GP_MENU;
    tempMenuValue = GPSequence;
    tft.fillScreen(TFT_BLACK);  // Full clear for menu
    tft.setTextFont(2);
    tft.setTextSize(1);
    int16_t x1 = 0, y1 = 0;
    uint16_t w = tft.textWidth("Select Sequence:");
    uint16_t h = tft.fontHeight();
    int ty1 = seqHeaderY;
    printCenteredOutlined("Select Sequence:", ty1);
    const char* buf = seqNames[tempMenuValue];
    w = tft.textWidth(buf);
    h = tft.fontHeight();
    int ty2 = seqNameY;
    printCenteredOutlined(buf, ty2);
    tft.setTextFont(0);
  }

  // Handle encoder in menus
  if (currentMenu != NO_MENU) {
    if (delta != 0) {
      tempMenuValue += delta;
      if (currentMenu == SLEG_MENU) {
        tempMenuValue = constrain(tempMenuValue, 1, 6);
        tft.fillRect(0, slegY, tft.width(), 20, TFT_BLACK); // Erase text with black
        drawHexapod(tempMenuValue);  // Redraw image and overlays
        char buf[20];
        sprintf(buf, "Select Leg: %d", tempMenuValue);
        tft.setTextFont(2);
        tft.setTextSize(1);
        printCenteredOutlined(buf, slegY);
        tft.setTextFont(0);
      } else if (currentMenu == GAIT_MENU) {
        tempMenuValue = constrain(tempMenuValue, 0, 7);
        drawGaitMenu(tempMenuValue);
      } else if (currentMenu == GP_MENU) {
        tempMenuValue = constrain(tempMenuValue, 0, numSeqFiles);
        const char* buf = seqNames[tempMenuValue];
        tft.fillRect(0, seqNameY, tft.width(), 20, TFT_BLACK);
        tft.setTextFont(2);
        tft.setTextSize(1);
        int16_t x1 = 0, y1 = 0;
        uint16_t w = tft.textWidth(buf);
        uint16_t h = tft.fontHeight();
        int ty = seqNameY;
        printCenteredOutlined(buf, ty);
        tft.setTextFont(0);
      }
    }
  } 
  // Encoder switch confirm (index 3)
  if (menuButtonPressed[3] && currentMenu != NO_MENU) {
    if (currentMenu == SLEG_MENU) {
      SelectedLeg = tempMenuValue - 1;
      showConfirm = true;
      confirmStart = millis();
      confirmedLeg = tempMenuValue;
      // Draw confirmation text immediately
      tft.fillRect(0, slegY, tft.width(), 20, TFT_BLACK); // Erase with black
      char buf[20];
      sprintf(buf, "Leg %d Selected", confirmedLeg);
      tft.setTextFont(2);
      tft.setTextSize(1);
      printCenteredOutlined(buf, slegY);
      tft.setTextFont(0);
      // stay in menu
    } else if (currentMenu == GAIT_MENU) {
      GaitPreset = tempMenuValue;
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

      prevGaitPreset = GaitPreset;

    // NOTE GP has nothing to do with Gait Preset(confusing, i know...) GPPlayer is for playback of pre-made sequences
    // Gait Preset is for selecting one of the 8 different walking gait modes.
    // GP not setup yet on hexapod side, leave commented out for now.
    // } else if (currentMenu == GP_MENU) {
    // GPSequence = tempMenuValue;
    //  stay in menu
    }
  }

 // Handle confirmation timeout for SLEG_MENU
  if (currentMenu == SLEG_MENU && showConfirm) {
    if (millis() - confirmStart > 1000) {
      showConfirm = false;
      // Redraw select text
      tft.fillRect(0, slegY, tft.width(), 20, TFT_BLACK); // Erase with black
      char buf[20];
      sprintf(buf, "Select Leg: %d", tempMenuValue);
      tft.setTextFont(2);
      tft.setTextSize(1);
      printCenteredOutlined(buf, slegY);
      tft.setTextFont(0);
      // Redraw hexapod to update any changes
      drawHexapod(tempMenuValue);
    }
  }

  // Build and send control string via ESP-NOW
  char buf[100];
  sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
          leftFwdBack, leftLeftRight, leftTwist, rightUpDown, rightLeftRight, rightTwist,
          bodyHeight, gaitSpeed, g_fHexOn, PiControllerSwitch, ModeSelect, BalanceMode,
          DoubleTravel, DoubleLegLiftHeight, WalkMethod, SLHold, GaitPreset, SelectedLeg, GPSequence);
  esp_now_send(receiverMAC, (uint8_t*)buf, strlen(buf));

  delay(50);  // Adjust for loop rate to match original refresh (e.g., 20Hz)
}
