#include <Arduino.h>
#include "morse.hpp"
#include "Oled_Display.hpp"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HijelHID_BLEKeyboard.h>

#define OLED_RESET      -1
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define LCD_I2C_ADDRESS 0x3c
#define I2C_SDA_PIN     22
#define I2C_SCL_PIN     23

#define SIGNAL_PIN      0
#define DAH_SIGNAL_PIN  1
#define BUZZER_PIN      8

MorseSignalReader     morse;
HijelHID_BLEKeyboard  keyboard("MorseKeyboard", "SanaeProject", 100);
Adafruit_SSD1306      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OledDisplay           oledDisplay(display, 1, SSD1306Color::White, Align::Center);

void setup() {
  // 初期設定
  Serial.begin(9600);
  morse.begin(SIGNAL_PIN, DAH_SIGNAL_PIN);
  keyboard.begin();

  // I2Cの初期化
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // ディスプレイの初期化
  if(!display.begin(SSD1306_SWITCHCAPVCC, LCD_I2C_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while(1); // 失敗時は停止
  }else{
    Serial.println("SSD1306 allocated successfully!");
  }

  oledDisplay.init();
}

void loop() {
  const char key = morse.getKey();
  const bool isPushed = 
    digitalRead(SIGNAL_PIN) == LOW ||
    digitalRead(DAH_SIGNAL_PIN) == LOW; // 押されてるか判定

  oledDisplay
    .setAlign(Align::Center)
    .setTextSize(2)
    .print("hello", 0);

  // キー入力があった場合
  if(key != MORSE_KEY_NONE){
    Serial.print("key:");
    Serial.println(key);
    keyboard.print(key);

    oledDisplay
      .setAlign(Align::Left)
      .setTextSize(1)
      .clear()
      .print("key:" + String(key), 2);
  }
}