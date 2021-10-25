#include <M5Stack.h>
#include "assets.h";

#define KEYBOARD_I2C_ADDR     0X08
#define KEYBOARD_INT          5

uint8_t key_val;

template<class T>
struct State {
  T currentValue;
  T beforeValue;
};

int MAX = 60 * 40;
int SIDE_ME = 1;
int SIDE_OPPONENT = 2;
int INPUT_LENGTH = 4;

int remain = MAX;
int count = 0;

bool isRunning = false;
bool isCharging = false;

unsigned long int beforeTime = 0;

int inputData[4] = { -1, -1, -1, -1 };

State<bool> isComplementMode = {true, false};
State<int> minutes = {0, -1};
State<int> seconds = {0, -1};
State<int> battery = {0, -1};
State<int> inputRatio = {-1, 1};
State<int> myLifePoint = {8000, 7999};
State<int> opponentLifePoint = {8000, 7999};
State<int> focusSide = {SIDE_ME, -1};
State<String> stringInputData = {"", ""};

void setup() {
  M5.begin();
  dacWrite(25, 0);
  M5.Power.begin();
  Wire.begin();
  pinMode(KEYBOARD_INT, INPUT_PULLUP);
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setBrightness(100);
  M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_play);
  M5.Lcd.drawBitmap(150, 210, 20, 20, bitmap_icons_icon_log);
  M5.Lcd.drawBitmap(245, 210, 20, 20, bitmap_icons_icon_reset);
  M5.Lcd.drawBitmap(260, 7, 16, 9, bitmap_battery);
  M5.Lcd.drawBitmap(112, 5, 16, 16, bitmap_timer);
  M5.Lcd.fillRect(157, 72, 2, 30, LIGHTGREY);
  M5.Lcd.fillTriangle(0, 0, 20, 0, 0, 20, BLACK);
  M5.Lcd.fillTriangle(320, 240, 320 - 20, 240, 320, 240 - 20, BLACK);
  isCharging = M5.Power.isCharging();
  beforeTime = millis();
}

void loop() {
  M5.update();
  minutes.currentValue = remain / 60;
  seconds.currentValue = remain % 60;

  // Draw timer minutes & seconds
  if (isUpdated(minutes)) {
    minutes.beforeValue = minutes.currentValue;
    M5.Lcd.fillRect( 132, 5, 22, 20, WHITE);
    drawNumberSprite(minutes.currentValue, 146, 13, 12, 16, 2, bitmap_numbers);
  }
  if (isUpdated(seconds)) {
    seconds.beforeValue = seconds.currentValue;
    M5.Lcd.fillRect(164, 5, 24, 20, WHITE);
    String text = String("0" + String(seconds.currentValue));
    drawNumberSprite(text.substring(text.length() -2, text.length()), 176, 13, 12, 16, 2, bitmap_numbers);
  }

  // Draw timer colon
  if (count == 0) {
    M5.Lcd.drawBitmap(160 - 6, 5, 12, 16, bitmap_colon);
  } else if (count == 25) {
    M5.Lcd.fillRect(160 - 6, 5, 12, 16, WHITE);
  }

  // Draw battery status
  battery.currentValue = M5.Power.getBatteryLevel();
  if (isUpdated(battery)) {
    battery.beforeValue = battery.currentValue;

    M5.Lcd.fillRect(276, 5, 44, 20, WHITE);

    // Draw Battery Icon
    if (battery.currentValue == 100) {
      drawNumberSprite(battery.currentValue, 296, 12, 10, 14, 3, bitmap_10px_numbers);
    } else {
      String text = String("0" + String(battery.currentValue));
      drawNumberSprite(text.substring(text.length() -2, text.length()), 297, 12, 10, 14, 3, bitmap_10px_numbers);
    }

    M5.Lcd.fillRect(260 + 1, 7 + 1, 11, 7, GREEN);

    M5.Lcd.drawBitmap(308, 5, 10, 14, bitmap_10px_percent);
  }

  // Draw charging status
  bool currentIsCharging = M5.Power.isCharging();
  if (isCharging != currentIsCharging) {
    isCharging = currentIsCharging;
    if (isCharging) {
      M5.Lcd.fillRect(252, 10, 5, 6, GREEN);
    } else {
      M5.Lcd.fillRect(252, 10, 5, 6, DARKGREY);
    }
  }

  if (isRunning) {
    if (remain == 0) {
      dacWrite(25, 0x50);
      M5.Speaker.begin();
      M5.Speaker.setVolume(2);
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 4; i++) {
          M5.Speaker.tone(1000, 60);
          delay(60);
          M5.Speaker.mute();
          delay(60);
        }
        delay(700);
      }
      remain = MAX;
      dacWrite(25, 0);
      isRunning = false;
    } else {
      count ++;
      if (count == 50) {
        remain--;
        count = 0;
        M5.Lcd.wakeup();
      }
    }
  }

  checkButtonAction();

  if (digitalRead(KEYBOARD_INT) == LOW)
  {
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);
    while (Wire.available())
    {
      uint8_t key_val = Wire.read();
      if (key_val != 0)
      {
        bool executed = false;
        if (isDigit(key_val))
        {
          for (int i = 0; i < INPUT_LENGTH; i++) {
            if (inputData[i] == -1) {
              inputData[i] = key_val;
              executed = true;
              break;
            }
          }
        }

        switch (key_val) {
          case '=': {
              String v = "";
              for (int i = 0; i < INPUT_LENGTH; i++) {
                if (inputData[i] != -1) {
                  v += (char)inputData[i];
                }
              }
              stringInputData.currentValue = v;
              if (
                isComplementMode.currentValue &&
                (stringInputData.currentValue.length() == 1 || stringInputData.currentValue.length() == 2)
              ) {
                stringInputData.currentValue.concat("00");
              }
              if (focusSide.currentValue == SIDE_ME) {
                switch (inputRatio.currentValue) {
                  case -1: {
                      myLifePoint.currentValue = max(0, myLifePoint.currentValue - ((int)stringInputData.currentValue.toInt()));
                      break;
                    }
                  case 0: {
                      myLifePoint.currentValue = stringInputData.currentValue.toInt();
                      break;
                    }
                  case 1: {
                      myLifePoint.currentValue += stringInputData.currentValue.toInt();
                      break;
                    }
                }
              } else {
                switch (inputRatio.currentValue) {
                  case -1: {
                      opponentLifePoint.currentValue = max(0, opponentLifePoint.currentValue - ((int)stringInputData.currentValue.toInt()));
                      break;
                    }
                  case 0: {
                      opponentLifePoint.currentValue = stringInputData.currentValue.toInt();
                      break;
                    }
                  case 1: {
                      opponentLifePoint.currentValue += stringInputData.currentValue.toInt();
                      break;
                    }
                }
              }
              for (int i = 0; i < INPUT_LENGTH; i++) {
                inputData[i] = -1;
              }
              inputRatio.currentValue = -1;
              executed = true;
              break;
            }
          case 96: {
              // +/- button
              for (int j = 0; j < 2; j++) {
                for (int i = 0; i < INPUT_LENGTH; i++) {
                  if (inputData[i] == -1) {
                    inputData[i] = 48;
                    break;
                  }
                }
              }
              executed = true;
              break;
            }
          case 'A': {
              for (int i = 0; i < INPUT_LENGTH; i++) {
                inputData[i] = -1;
              }
              executed = true;
              break;
            }
          case 'M': {
              focusSide.currentValue = (focusSide.currentValue == 1) ? 2 : 1;
              for (int i = 0; i < INPUT_LENGTH; i++) {
                inputData[i] = -1;
              }
              executed = true;
              break;
            }
          case '%': {
            isComplementMode.currentValue = !isComplementMode.currentValue;
            break;
          }
          case '-': {
              inputRatio.currentValue = -1;
              break;
            }
          case '.': {
              inputRatio.currentValue = 0;
              break;
            }
          case '+': {
              inputRatio.currentValue = 1;
              break;
            }
        }

        if (executed) {
          String v = "";
          for (int i = 0; i < INPUT_LENGTH; i++) {
            if (inputData[i] != -1) {
              v += (char)inputData[i];
            }
          }
          stringInputData.currentValue = v;
          break;
        }

        break;
      }
      delay(1);
    }
  }

  if (isUpdated(stringInputData) || isUpdated(inputRatio) || isUpdated(isComplementMode)) {
    M5.Lcd.setTextSize(2);
    isComplementMode.beforeValue = isComplementMode.currentValue;
    inputRatio.beforeValue = inputRatio.currentValue;
    stringInputData.beforeValue = stringInputData.currentValue;
    M5.Lcd.fillRect(0, 155, 320, 45, LIGHTGREY);
    switch (inputRatio.currentValue) {
      case -1: {
          M5.Lcd.drawString("-", 10, 169);
          break;
        }
      case 0: {
          M5.Lcd.drawString("=", 10, 169);
          break;
        }
      case 1: {
          M5.Lcd.drawString("+", 10, 169);
          break;
        }
    }
    M5.Lcd.setTextColor(BLACK);
    if (isComplementMode.currentValue && (stringInputData.currentValue.length() == 1 || stringInputData.currentValue.length() == 2) ) {
      M5.Lcd.drawString(stringInputData.currentValue, 25, 169);
      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.drawString("00", 21 + (stringInputData.currentValue.length() * 15), 169);
    } else {
      M5.Lcd.drawString(stringInputData.currentValue, 25, 169);
    }
    M5.Lcd.setTextColor(BLACK);
  }

  if (isUpdated(myLifePoint)) {
    if (myLifePoint.currentValue == 8000) {
      myLifePoint.beforeValue = 8000;
    }
    if (myLifePoint.beforeValue > myLifePoint.currentValue) {
      if (abs(myLifePoint.beforeValue - myLifePoint.currentValue) > 100) {
        myLifePoint.beforeValue -= 100;
      } else {
        myLifePoint.beforeValue -= abs(myLifePoint.beforeValue - myLifePoint.currentValue);
      }
    } else {
      if (abs(myLifePoint.beforeValue - myLifePoint.currentValue) > 100) {
        myLifePoint.beforeValue += 100;
      }
      else {
        myLifePoint.beforeValue += abs(myLifePoint.beforeValue - myLifePoint.currentValue);
      }
    }
    drawNumberSprite(myLifePoint.beforeValue, 86, 87, 20, 25, 5, bitmap_20px_numbers);
  }

  if (isUpdated(opponentLifePoint)) {
    if (opponentLifePoint.currentValue == 8000) {
      opponentLifePoint.beforeValue = 8000;
    }
    if (opponentLifePoint.beforeValue > opponentLifePoint.currentValue) {
      if (abs(opponentLifePoint.beforeValue - opponentLifePoint.currentValue) > 100) {
        opponentLifePoint.beforeValue -= 100;
      } else {
        opponentLifePoint.beforeValue -= abs(opponentLifePoint.beforeValue - opponentLifePoint.currentValue);
      }
    } else {
      if (abs(opponentLifePoint.beforeValue - opponentLifePoint.currentValue) > 100) {
        opponentLifePoint.beforeValue += 100;
      }
      else {
        opponentLifePoint.beforeValue += abs(opponentLifePoint.beforeValue - opponentLifePoint.currentValue);
      }
    }
    drawNumberSprite(opponentLifePoint.beforeValue, 80 + 160, 87, 20, 25, 5, bitmap_20px_numbers);
  }

  if (isUpdated(focusSide)) {
    focusSide.beforeValue = focusSide.currentValue;
    if (focusSide.currentValue == SIDE_ME) {
      M5.Lcd.fillRect(310, 80 - 3, 10, 20, WHITE);
      M5.Lcd.fillRect(  0, 80 - 3, 6, 20, RED);
    }
    if (focusSide.currentValue == SIDE_OPPONENT) {
      M5.Lcd.fillRect(  0, 80 - 3, 6, 20, WHITE);
      M5.Lcd.fillRect(320 - 6, 80 - 3, 6, 20, RED);
    }
  }

  int waitTime = 20 - (millis() - beforeTime);
  if (waitTime >= 0) {
    delay(waitTime);
  }
  beforeTime = millis();
}

void resetState() {
  isRunning = false;
  remain = MAX;
  minutes.beforeValue = -1;
  seconds.beforeValue = -1;
  count = 0;
  myLifePoint.currentValue = 8000;
  opponentLifePoint.currentValue = 8000;
  M5.Lcd.fillRect(60, 210, 20, 20, WHITE);
  M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_play);
}

bool isUpdated(State<int> state) {
  return state.currentValue != state.beforeValue;
}

bool isUpdated(State<bool> state) {
  return state.currentValue != state.beforeValue;
}

bool isUpdated(State<String> state) {
  return state.currentValue != state.beforeValue;
}

void checkButtonAction() {
  // Press [START/STOP]
  if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200)) {
    M5.Lcd.fillRect(60, 210, 20, 20, WHITE);
    if (isRunning) {
      M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_play);
    } else {
      M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_pause);
    }
    isRunning = !isRunning;
  }

  // Press [LOG]
  if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)) {
    //    isRunning = false;
  }

  // Press [RESET]
  if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
    resetState();
  }
}

void drawNumberSprite(int value, int centerX, int centerY, int w, int h, int maxCount, const uint16_t* sprites[10]) {
  drawNumberSprite(String(value), centerX, centerY, w, h, maxCount, sprites);
}

void drawNumberSprite(String spriteString, int centerX, int centerY, int w, int h, int maxCount, const uint16_t* sprites[10]) {
  // Clean up background
  M5.Lcd.fillRect(centerX - ((maxCount * w) / 2), centerY - (h / 2), maxCount * w, h, WHITE);

  int spriteLength = spriteString.length();
  int left = centerX - ((spriteLength * w) / 2);
  int top = centerY - (h / 2);

  for (int i = 0; i < spriteLength; i++) {
    int num = spriteString.charAt(i) - '0';
    M5.Lcd.drawBitmap(left + (i * (w - 2)), top, w, h, sprites[num]);
  }
}
