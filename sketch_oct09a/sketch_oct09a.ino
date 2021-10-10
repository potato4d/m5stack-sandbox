#include <M5Stack.h>

int MAX = 60 * 40;
int remain = MAX;
int beforeMinutes = -1;
int beforeSeconds = -1;
int battery = -1;
int count = 0;
bool isRunning = false;
bool isCharging = false;
unsigned long int beforeTime = 0;

void setup() {
  // put your setup code here, to run once:
  M5.begin();  //Init M5Core.  初始化 M5Core
  M5.Power.begin(); //Init Power module.  初始化电源模块
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setBrightness(100);

  M5.Lcd.fillRect(30, 210 - 5, 80, 30, DARKCYAN);
  M5.Lcd.drawString("START", 55, 222 - 5);

  M5.Lcd.fillRect(120, 210 - 5, 80, 30, RED);
  M5.Lcd.drawString("STOP", 150, 222 - 5);

  M5.Lcd.fillRect(215, 210 - 5, 80, 30, DARKGREY);
  M5.Lcd.drawString("RESET", 240, 222 - 5);

  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(6);

  M5.Lcd.fillRect(250,  8, 14,  2, BLACK);
  M5.Lcd.fillRect(250, 16, 14,  2, BLACK);
  M5.Lcd.fillRect(250,  8,  2, 10, BLACK);
  M5.Lcd.fillRect(262,  8,  2, 10, BLACK);
  M5.Lcd.fillRect(264, 11,  2,  4, BLACK);
  M5.Lcd.fillRect(252, 10, 5, 6, DARKGREY);

  isCharging = M5.Power.isCharging();
  beforeTime = millis();
}

String zeroPadding(int num,int zeroCount){
  char buf[16];
  char param[5] = {'%','0',(char)(zeroCount+48),'d','\0'};
  sprintf(buf,param,num);
  return buf;
}

void loop() {
  M5.update();
  int minutes = remain / 60;
  int seconds = remain % 60;

  if (!(minutes == beforeMinutes && seconds == beforeSeconds )) {
    M5.Lcd.setTextSize(6);
  }
  
  if (minutes != beforeMinutes) {
    beforeMinutes = minutes;
    M5.Lcd.fillRect( 75, 95, 70, 50, WHITE);
    M5.Lcd.drawString(zeroPadding(minutes, 2), 75 , 95);
  }

  if (seconds != beforeSeconds) {
    beforeSeconds = seconds;
    M5.Lcd.fillRect(170, 95, 80, 50, WHITE);
    M5.Lcd.drawString(zeroPadding(seconds, 2), 182 , 95);
  }
  
  if (count == 0) {
    M5.Lcd.setTextSize(6);
    M5.Lcd.drawString(":", 146 , 95);
  } else if (count == 50) {
    M5.Lcd.setTextSize(6);
    M5.Lcd.fillRect(146, 95, 20, 30, WHITE);
  }

  int currentBattery = M5.Power.getBatteryLevel();
  if (battery != currentBattery) {
    battery = currentBattery;

    // Draw Battery Percent
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(270, 5, 50, 20, WHITE);
    M5.Lcd.drawString(zeroPadding(battery, 3) + "%", 270 , 5);

    // Draw Battery Icon
    if (battery > 75) {
      M5.Lcd.fillRect(252, 10, 10, 6, GREEN);
    } else if (battery > 50) {
      M5.Lcd.fillRect(252, 10, 10, 6, WHITE);
      M5.Lcd.fillRect(252, 10, 7, 6, GREEN);
    } else if (battery > 25) {
      M5.Lcd.fillRect(252, 10, 10, 6, WHITE);
      M5.Lcd.fillRect(252, 10, 5, 6, YELLOW);
    } else {
      M5.Lcd.fillRect(252, 10, 10, 6, WHITE);
      M5.Lcd.fillRect(252, 10, 3, 6, RED);
    }
  }
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
      M5.Speaker.begin();
      M5.Speaker.setVolume(1);
      for (int j=0;j<3;j++) {
        for (int i=0;i<4;i++) {
          M5.Speaker.tone(1000, 60);
          delay(60);
          M5.Speaker.mute();
          delay(60);
        }
        delay(700);
      }
      isRunning = false;
    } else {
      count ++;
      if (count == 100) {
        remain--;
        count = 0;
        M5.Lcd.wakeup();
      }
    }
  }

  if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200)) {
    isRunning = true;
  }

  if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)) {
    isRunning = false;
  }

  if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
    isRunning = false;
    remain = MAX;
    beforeMinutes = -1;
    beforeSeconds = -1;
    count = 0;
  }

  delay(10 - (millis() - beforeTime));
  beforeTime = millis();
}
