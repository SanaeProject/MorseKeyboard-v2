#include <Arduino.h>
#include "morse.hpp"
#include "Oled_Display.hpp"
#include "E220.hpp"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HijelHID_BLEKeyboard.h>

// I2C OLED display settings
#define OLED_RESET      -1
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define LCD_I2C_ADDRESS 0x3c
#define I2C_SDA_PIN     22
#define I2C_SCL_PIN     23

// E220 module pin settings
#define M0_PIN   0
#define M1_PIN   1
#define AUX_PIN  2
#define TX_PIN   16
#define RX_PIN   17
#define CHANNEL  11

// Morse
#define SIGNAL_PIN      19
#define DAH_SIGNAL_PIN  20
#define BUZZER_PIN      8

MorseSignalReader     morse;
HijelHID_BLEKeyboard  keyboard("MorseKeyboard", "SanaeProject", 100);
Adafruit_SSD1306      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OledDisplay           oledDisplay(display, 1, SSD1306Color::White, Align::Center);
E220 e220;

String writeBuffer = "";
String receiveBuffer = "";

void setup()
{
    Serial.begin(9500);
    delay(5000);

    // キーボード初期化
    keyboard.begin();

    // ディスプレイの初期化
    if(!display.begin(SSD1306_SWITCHCAPVCC, LCD_I2C_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        while(1); // 失敗時は停止
    }else{
        Serial.println("SSD1306 allocated successfully!");
    }
    oledDisplay.init();

    // E220モジュールの初期化
    e220.setLogLevel(E220_LogLevel::INFO); // ログレベルをINFOに設定
    if (!e220.begin(&Serial1, RX_PIN, TX_PIN, M0_PIN, M1_PIN, AUX_PIN, &Serial)){
        Serial.println("E220 begin failed");
        return;
    }
    e220.setDeviceAddress(0x0001)
        .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
        .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
        .setPayloadLength(E220_PayloadLength::LENGTH_200)
        .setRSSINoiseEnable(true)
        .setTxPower(E220_TxPower_22S::POWER_13DBM)
        .setFrequencyChannel(CHANNEL)
        .setRSSIByteEnable(false)
        .setSendMode(E220_SendMode::MODE_FIXED)
        .setWORCycle(E220_WORCycle::CYCLE_500MS)
        .setCryptKey(0x0000);

    // Morse
    morse.begin(SIGNAL_PIN, DAH_SIGNAL_PIN);

    Serial.println("===========Setup complete===========");
    oledDisplay.clear();
    oledDisplay.print("Setup complete!");
}

void loop()
{
    char key = morse.getKey();
    if(key != MORSE_KEY_NONE){
        Serial.print("Detected key: ");
        keyboard.print(key);
    }
    while(e220.available() > 0){
        const int value = e220.read();

        if (value < 0) break; // 読み込みエラーの場合はループを抜ける
        
        const uint8_t data = static_cast<uint8_t>(value);
        Serial.print("Received data: ");
        Serial.print(data, HEX);
        if(data == '\n') break; // 改行コードが来たらループを抜ける
        if(data == '\r') continue; // キャリッジリターンは無視する

        receiveBuffer += static_cast<char>(data);
    }

    if(receiveBuffer.endsWith("\n")){
        oledDisplay.clear();
        keyboard.print(receiveBuffer);
        receiveBuffer = "";
    }
}