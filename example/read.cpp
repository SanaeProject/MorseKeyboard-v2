#include <Arduino.h>

#include "E220.hpp"

#define M0_PIN   0
#define M1_PIN   1
#define TX_PIN   2
#define RX_PIN   21
#define AUX_PIN  22
#define CHANNEL  25

E220 e220;

void setup() {
    // PCとのUSBシリアル
    Serial.begin(9600);
    delay(5000);

    // E220とのUART
    Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

    Serial.println();
    Serial.println("=== E220 RECEIVER ===");

    // E220初期化
    e220.begin(
        Serial1,
        M0_PIN,
        M1_PIN,
        AUX_PIN
    );

    // E220の設定
    e220.setCommand(E220_Command::WRITE_TEMP)
        .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
        .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
        .setTxPower(E220_TxPower_22S::POWER_13DBM)
        .setFrequencyChannel(CHANNEL)
        .setSendMode(E220_SendMode::MODE_FIXED)
        .writeConfig();

    // モジュールの設定を読み込む
    e220.readConfig();

    Serial.printf(
        "E220 Config: UART=%02X, AirDataRate=%02X, TxPower=%02X, FreqChannel=%02X\n",
        static_cast<uint8_t>(e220.getUARTSerialPortRate()),
        static_cast<uint8_t>(e220.getAirDataRate()),
        static_cast<uint8_t>(e220.getTxPower()),
        static_cast<uint8_t>(e220.getFrequencyChannel())
    );

    Serial.println("E220 initialized");
    Serial.println("Waiting for data...");

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

    if (e220.available() > 0) {

        Serial.print("RX: ");

        digitalWrite(LED_BUILTIN, HIGH);

        while (e220.available() > 0) {
            int data = e220.read();
            if (data >= 0) {
                Serial.write(static_cast<uint8_t>(data));
            }
        }

        Serial.println();

        digitalWrite(LED_BUILTIN, LOW);
    }
}