#include <Arduino.h>
#include "E220.hpp"

#define M0_PIN  0
#define M1_PIN  1
#define TX_PIN  2
#define RX_PIN  21
#define AUX_PIN 22
#define CHANNEL 25

E220 e220;

void setup() {
    // PCとのUSBシリアル
    Serial.begin(9600);

    delay(5000); // Wait for Serial to initialize

    // E220とのUART
    Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

    Serial.println();
    Serial.println("=== E220 TRANSMITTER ===");

    // E220初期化
    e220.begin(
        Serial1,
        M0_PIN,
        M1_PIN,
        AUX_PIN
    );

    // 無線設定
    e220.setCommand(E220_Command::WRITE_TEMP)
        .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
        .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
        .setTxPower(E220_TxPower_22S::POWER_13DBM)
        .setFrequencyChannel(CHANNEL)
        .setSendMode(E220_SendMode::MODE_TRANSPARENT)
        .writeConfig();

    Serial.println("----E220 Config written.----");
    e220.readConfig(); // モジュールの設定を読み込む

    Serial.println("E220 initialized");
    Serial.println("Start transmitting...");

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("TX: Hello, World!");

    if (e220.send("Hello, World!")) {
        Serial.println("TX: success");
    } else {
        Serial.println("TX: failed");
    }
    digitalWrite(LED_BUILTIN, LOW);

    delay(1000);
}