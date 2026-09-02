#include <Arduino.h>
#include "morse.hpp"

MorseSignalReader morse;
void setup() {
  morse.begin(D0);
  Serial.begin(9600);
}

void loop() {
  char key = morse.getKey();
  if(key != MORSE_KEY_NONE){
    Serial.print("key:");
    Serial.println(key);
    delay(500);
  }
}