#include <map>
#include <M5Stack.h>
#include "assets.h";

#define KEYBOARD_I2C_ADDR     0X08
#define KEYBOARD_INT          5
#define DAC_AUDIO 25
#define CHAR_PLUS_MINUS 96
#define INPUT_LENGTH 4

uint8_t key_val;

template<class T>
struct State {
  T current;
  T before;
};

int MAX = 60 * 40;
int remain = MAX;
int count = 0;

bool isRunning = false;
unsigned long int beforeTime = 0;

int inputData[4] = { -1, -1, -1, -1 };
int frame = 0;
bool isAnimating = false;

State<bool> isComplementMode = {true, false};
State<int> minutes = {0, -1};
State<int> seconds = {0, -1};
State<int> battery = {0, -1};
State<int> inputRatio = { -1, 1};
State<int> myLifePoint = {8000, 7999};
State<int> opponentLifePoint = {8000, 7999};
State<int> focusSide = {1, -1};
State<String> stringInputData = {"", ""};

std::map <char, int> operatorToNumber;
std::map <int, char> numberToOperator;
std::map <char, int> FOCUS_SIDE;

void setup() {
  M5.begin();
  dacWrite(DAC_AUDIO, 0);
  M5.Power.begin();
  Wire.begin();
  pinMode(KEYBOARD_INT, INPUT_PULLUP);
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setBrightness(100);
  M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_play);
  M5.Lcd.drawBitmap(150, 210, 20, 20, bitmap_icons_icon_log);
  M5.Lcd.drawBitmap(245, 210, 20, 20, bitmap_icons_icon_reset);
  M5.Lcd.drawBitmap(261, 7, 16, 9, bitmap_battery);
  M5.Lcd.drawBitmap(112, 5, 16, 16, bitmap_timer);
  M5.Lcd.fillRect(157, 72, 2, 30, LIGHTGREY);
  M5.Lcd.fillTriangle(0, 0, 20, 0, 0, 20, BLACK);
  M5.Lcd.fillTriangle(320, 240, 320 - 20, 240, 320, 240 - 20, BLACK);

  operatorToNumber.insert(std::make_pair('-', -1));
  operatorToNumber.insert(std::make_pair('.',  0));
  operatorToNumber.insert(std::make_pair('=',  0));
  operatorToNumber.insert(std::make_pair('+',  1));

  numberToOperator.insert(std::make_pair(-1, '-'));
  numberToOperator.insert(std::make_pair( 0, '='));
  numberToOperator.insert(std::make_pair( 1, '+'));

  FOCUS_SIDE.insert(std::make_pair('me', 1));
  FOCUS_SIDE.insert(std::make_pair('opponent', 2));
  beforeTime = millis();
}

void loop() {
  M5.update();
  minutes.current = remain / 60;
  seconds.current = remain % 60;

  // Draw timer minutes & seconds
  if (isUpdated(minutes)) {
    minutes.before = minutes.current;
    M5.Lcd.fillRect( 132, 5, 22, 20, WHITE);
    drawNumberSprite(minutes.current, 146, 13, 12, 16, 2, bitmap_numbers);
  }
  if (isUpdated(seconds)) {
    seconds.before = seconds.current;
    M5.Lcd.fillRect(164, 5, 24, 20, WHITE);
    String text = String("0" + String(seconds.current));
    drawNumberSprite(text.substring(text.length() - 2, text.length()), 176, 13, 12, 16, 2, bitmap_numbers);
  }

  // Draw timer colon
  if (count == 0) {
    M5.Lcd.drawBitmap(160 - 6, 5, 12, 16, bitmap_colon);
  } else if (count == 25) {
    M5.Lcd.fillRect(160 - 6, 5, 12, 16, WHITE);
  }

  // Draw battery status
  battery.current = M5.Power.getBatteryLevel();
  if (isUpdated(battery)) {
    battery.before = battery.current;

    M5.Lcd.fillRect(276, 5, 44, 20, WHITE);

    // Draw Battery Icon
    if (battery.current == 100) {
      drawNumberSprite(battery.current, 296, 12, 10, 14, 3, bitmap_10px_numbers);
    } else {
      String text = String("0" + String(battery.current));
      drawNumberSprite(text.substring(text.length() - 2, text.length()), 297, 12, 10, 14, 3, bitmap_10px_numbers);
    }

    int fillColor = battery.current > 50 ? GREEN : battery.current > 25 ? YELLOW : RED;
    M5.Lcd.fillRect(261 + 1, 7 + 1, 11, 7, fillColor);
    M5.Lcd.drawBitmap(308, 5, 10, 14, bitmap_10px_percent);
  }

  if (isRunning) {
    if (remain == 0) {
      dacWrite(DAC_AUDIO, 0x50);
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
              stringInputData.current = v;
              if (isComplementMode.current && range(stringInputData.current.length(), 1, 2)) {
                stringInputData.current.concat("00");
              }
              int value = stringInputData.current.toInt();
              if (focusSide.current == FOCUS_SIDE.at('me')) {
                switch (inputRatio.current) {
                  case -1: {
                      myLifePoint.current = max(0, myLifePoint.current - value);
                      break;
                    }
                  case 0: {
                      myLifePoint.current = value;
                      break;
                    }
                  case 1: {
                      myLifePoint.current += value;
                      break;
                    }
                }
              } else {
                switch (inputRatio.current) {
                  case -1: {
                      opponentLifePoint.current = max(0, opponentLifePoint.current - value);
                      break;
                    }
                  case 0: {
                      opponentLifePoint.current = value;
                      break;
                    }
                  case 1: {
                      opponentLifePoint.current += value;
                      break;
                    }
                }
              }
              for (int i = 0; i < INPUT_LENGTH; i++) {
                inputData[i] = -1;
              }
              inputRatio.current = -1;
              executed = true;
              break;
            }
          case CHAR_PLUS_MINUS: {
              // +/- button
              for (int j = 0; j < 2; j++) {
                for (int i = 0; i < INPUT_LENGTH; i++) {
                  if (inputData[i] == -1) {
                    inputData[i] = 48; // '0'
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
              focusSide.current = (focusSide.current == FOCUS_SIDE.at('me')) ? FOCUS_SIDE.at('opponent') : FOCUS_SIDE.at('me');
              for (int i = 0; i < INPUT_LENGTH; i++) {
                inputData[i] = -1;
              }
              executed = true;
              break;
            }
          case '%': {
              isComplementMode.current = !isComplementMode.current;
              break;
            }
          case '-':
          case '.':
          case '+': {
              inputRatio.current = operatorToNumber.at(key_val);
              break;
            }
          default: {
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
          stringInputData.current = v;
          break;
        }

        break;
      }
      delay(1);
    }
  }

  if (isUpdated(stringInputData) || isUpdated(inputRatio) || isUpdated(isComplementMode)) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(BLACK);
    isComplementMode.before = isComplementMode.current;
    inputRatio.before = inputRatio.current;
    stringInputData.before = stringInputData.current;
    M5.Lcd.fillRect(0, 155, 320, 45, LIGHTGREY);
    M5.Lcd.drawString(String(numberToOperator.at(inputRatio.current)), 10, 169);
    if (isComplementMode.current && range(stringInputData.current.length(), 1, 2) ) {
      M5.Lcd.drawString(stringInputData.current, 25, 169);
      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.drawString("00", 22 + (stringInputData.current.length() * 15), 169);
    } else {
      M5.Lcd.drawString(stringInputData.current, 25, 169);
    }
    M5.Lcd.setTextColor(BLACK);
  }

  if (
    isUpdated(myLifePoint) || isUpdated(opponentLifePoint)
  ) {
    if (!(myLifePoint.current == 8000 && opponentLifePoint.current == 8000)) {
      if (!isAnimating) {
        isAnimating = true;
        dacWrite(DAC_AUDIO, 0x50);
        M5.Speaker.begin();
        M5.Speaker.setVolume(2);
      }
      if (frame % 6 == 1) {
        M5.Speaker.tone(1000, 5);
      }
      if (frame % 6 == 5) {
        M5.Speaker.mute();
      }
    }
  } else {
    if (isAnimating) {
      isAnimating = false;
      dacWrite(25, 0);
      M5.Speaker.mute();
    }
  }

  if (isUpdated(myLifePoint)) {
    if (myLifePoint.current == 8000) {
      myLifePoint.before = 8000;
    }
    if (myLifePoint.before > myLifePoint.current) {
      if (abs(myLifePoint.before - myLifePoint.current) > 100) {
        myLifePoint.before -= 100;
      } else {
        myLifePoint.before -= abs(myLifePoint.before - myLifePoint.current);
      }
    } else {
      if (abs(myLifePoint.before - myLifePoint.current) > 100) {
        myLifePoint.before += 100;
      }
      else {
        myLifePoint.before += abs(myLifePoint.before - myLifePoint.current);
      }
    }
    drawNumberSprite(myLifePoint.before, 86, 87, 20, 25, 5, bitmap_20px_numbers);
  }

  if (isUpdated(opponentLifePoint)) {
    if (opponentLifePoint.current == 8000) {
      opponentLifePoint.before = 8000;
    }
    if (opponentLifePoint.before > opponentLifePoint.current) {
      if (abs(opponentLifePoint.before - opponentLifePoint.current) > 100) {
        opponentLifePoint.before -= 100;
      } else {
        opponentLifePoint.before -= abs(opponentLifePoint.before - opponentLifePoint.current);
      }
    } else {
      if (abs(opponentLifePoint.before - opponentLifePoint.current) > 100) {
        opponentLifePoint.before += 100;
      }
      else {
        opponentLifePoint.before += abs(opponentLifePoint.before - opponentLifePoint.current);
      }
    }
    drawNumberSprite(opponentLifePoint.before, 80 + 160, 87, 20, 25, 5, bitmap_20px_numbers);
  }

  if (isUpdated(focusSide)) {
    focusSide.before = focusSide.current;
    if (focusSide.current == FOCUS_SIDE.at('me')) {
      M5.Lcd.fillRect(310, 80 - 3, 10, 20, WHITE);
      M5.Lcd.fillRect(  0, 80 - 3, 6, 20, RED);
    }
    if (focusSide.current == FOCUS_SIDE.at('opponent')) {
      M5.Lcd.fillRect(  0, 80 - 3, 6, 20, WHITE);
      M5.Lcd.fillRect(320 - 6, 80 - 3, 6, 20, RED);
    }
  }

  int waitTime = 20 - (millis() - beforeTime);
  if (waitTime >= 0) {
    delay(waitTime);
  }
  beforeTime = millis();
  frame ++;
}

void resetState() {
  isRunning = false;
  remain = MAX;
  minutes.before = -1;
  seconds.before = -1;
  count = 0;
  myLifePoint.current = 8000;
  opponentLifePoint.current = 8000;
  M5.Lcd.fillRect(60, 210, 20, 20, WHITE);
  M5.Lcd.drawBitmap(60, 210, 20, 20, bitmap_icons_icon_play);
}

template<typename T>
bool isUpdated(State<T> state) {
  return state.current != state.before;
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

bool range(int value, int minValue, int maxValue) {
  return (minValue <= value) && (value <= maxValue);
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
