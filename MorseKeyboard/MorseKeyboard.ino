#include <Arduino.h>
#include "morse.hpp"
#include <HijelHID_BLEKeyboard.h>

#define SIGNAL_PIN      0
#define DAH_SIGNAL_PIN  1
#define BUZZER_PIN      8

MorseSignalReader morse;
HijelHID_BLEKeyboard keyboard("MorseKeyboard");

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  morse.begin(SIGNAL_PIN, DAH_SIGNAL_PIN);
  keyboard.begin();

  Serial.begin(9600);
}

void loop() {
  const char key = morse.getKey();
  const bool isPushed = 
    digitalRead(SIGNAL_PIN) == LOW ||
    digitalRead(DAH_SIGNAL_PIN) == LOW; // 押されてるか判定

  digitalWrite(BUZZER_PIN, isPushed ? HIGH: LOW); // ブザーを鳴らす

  // キー入力があった場合
  if(key != MORSE_KEY_NONE){
    Serial.print("key:");
    Serial.println(key);
    keyboard.print(key);
  }
}