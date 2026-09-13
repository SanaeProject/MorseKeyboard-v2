#include <Arduino.h>
#include "E220.hpp"

#define M0_PIN   0
#define M1_PIN   1
#define AUX_PIN  2

#define TX_PIN   16
#define RX_PIN   17

#define CHANNEL  11

E220 e220;
void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println("E220 RECEIVER");
    Serial.println("ESP32-C6");
    Serial.println("================================");

    e220.setLogLevel(E220_LogLevel::INFO);
    if (!e220.begin(&Serial1, RX_PIN, TX_PIN, M0_PIN, M1_PIN, AUX_PIN, &Serial)){
        Serial.println("E220 begin failed");
        return;
    }

    Serial.println("E220 begin OK");

    // Setup E220 configuration
    e220.setDeviceAddress(0x0000)
        .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
        .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
        .setPayloadLength(E220_PayloadLength::LENGTH_200)
        .setRSSINoiseEnable(true)
        .setTxPower(E220_TxPower_22S::POWER_13DBM)
        .setFrequencyChannel(CHANNEL)
        .setRSSIByteEnable(false)
        .setSendMode(E220_SendMode::MODE_TRANSPARENT)
        .setWORCycle(E220_WORCycle::CYCLE_500MS)
        .setCryptKey(0x0000);

    // print raw bytes for debugging
    // 期待値: C0 00 08 00 00 70 21 0A 03 00 00
    e220.printRawBytes();
    const bool result = e220.writeConfig();

    Serial.printf("writeConfig: %s\n", result ? "OK" : "FAILED");
    if (!result) return;

    Serial.printf("M0=%d M1=%d AUX=%d\n",
        digitalRead(M0_PIN),
        digitalRead(M1_PIN),
        digitalRead(AUX_PIN)
    );
    Serial.println("WAITING...");
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