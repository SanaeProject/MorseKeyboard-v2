#include <Arduino.h>

#define M0_PIN   0
#define M1_PIN   1
#define AUX_PIN  2

#define E220_TX  16   // ESP32 TX -> E220 RXD
#define E220_RX  17   // ESP32 RX <- E220 TXD

HardwareSerial E220(1);


// ============================================================
// AUX待ち
// ============================================================

bool waitAUX(uint32_t timeout = 3000)
{
    uint32_t start = millis();

    while (digitalRead(AUX_PIN) == LOW)
    {
        if (millis() - start >= timeout)
        {
            return false;
        }

        delay(1);
    }

    return true;
}


// ============================================================
// モード切替
// ============================================================

void setConfigMode()
{
    digitalWrite(M0_PIN, HIGH);
    digitalWrite(M1_PIN, HIGH);

    delay(20);

    if (!waitAUX())
    {
        Serial.println("AUX timeout: CONFIG");
    }

    delay(20);
}


void setNormalMode()
{
    digitalWrite(M0_PIN, LOW);
    digitalWrite(M1_PIN, LOW);

    delay(20);

    if (!waitAUX())
    {
        Serial.println("AUX timeout: NORMAL");
    }

    delay(20);
}


// ============================================================
// HEX表示
// ============================================================

void printHex(const uint8_t* data, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        if (data[i] < 0x10)
        {
            Serial.print('0');
        }

        Serial.print(data[i], HEX);
        Serial.print(' ');
    }

    Serial.println();
}


// ============================================================
// 設定読み出し
// ============================================================

bool readConfig(uint8_t* response, size_t& length)
{
    uint8_t command[] =
    {
        0xC1,
        0x00,
        0x08
    };

    while (E220.available())
    {
        E220.read();
    }

    Serial.println("READ CONFIG");
    Serial.print("Command: ");
    printHex(command, sizeof(command));

    E220.write(command, sizeof(command));
    E220.flush();

    length = 0;

    uint32_t start = millis();

    while (millis() - start < 1000)
    {
        while (E220.available())
        {
            uint8_t data = E220.read();

            if (length < 11)
            {
                response[length++] = data;
            }
        }

        if (length >= 11)
        {
            break;
        }

        delay(1);
    }

    Serial.print("Response: ");
    printHex(response, length);

    Serial.print("Bytes: ");
    Serial.println(length);

    return length == 11;
}


// ============================================================
// Transparent + Channel 10 に設定
// ============================================================

bool configureE220()
{
    setConfigMode();

    /*
        C0 00 08
        00 00        Address = 0x0000

        70           REG0
                     UART = 9600
                     AirDataRate = SF9 / BW125

        21           REG1
                     SubPacket = 200
                     RSSI ambient noise flag = 0
                     TX Power = 13 dBm

        0A           REG2
                     Channel = 10

        03           REG3
                     RSSI byte = 0
                     Transparent = 0
                     WOR cycle = 2000 ms

        00 00        Encryption key = 0
    */

    uint8_t command[] =
    {
        0xC0,
        0x00,
        0x08,

        0x00,
        0x00,
        0x70,
        0x21,
        0x0B,
        0x03,
        0x00,
        0x00
    };

    Serial.println();
    Serial.println("WRITE CONFIG");
    Serial.print("Command: ");
    printHex(command, sizeof(command));

    E220.write(command, sizeof(command));
    E220.flush();

    delay(100);

    while (E220.available())
    {
        E220.read();
    }

    // --------------------------------------------------------
    // 書き込み後に必ず読み戻す
    // --------------------------------------------------------

    uint8_t response[11];
    size_t length = 0;

    if (!readConfig(response, length))
    {
        Serial.println("ERROR: configuration read failed.");
        return false;
    }

    // --------------------------------------------------------
    // 期待値確認
    // --------------------------------------------------------

    bool ok = true;

    // response:
    // C1 00 08 AA BB REG0 REG1 CH REG3 CRYPT_H CRYPT_L

    if (response[4] != 0x00)
    {
        Serial.println("ERROR: address H mismatch");
        ok = false;
    }

    if (response[5] != 0x70)
    {
        Serial.printf(
            "ERROR: REG0 mismatch: 0x%02X\n",
            response[5]
        );
        ok = false;
    }

    if (response[6] != 0x21)
    {
        Serial.printf(
            "ERROR: REG1 mismatch: 0x%02X\n",
            response[6]
        );
        ok = false;
    }

    if (response[7] != 0x0A)
    {
        Serial.printf(
            "ERROR: Channel mismatch: %d\n",
            response[7]
        );
        ok = false;
    }

    if (response[8] != 0x03)
    {
        Serial.printf(
            "ERROR: REG3 mismatch: 0x%02X\n",
            response[8]
        );
        ok = false;
    }

    if (response[9] != 0x00 ||
        response[10] != 0x00)
    {
        Serial.println("ERROR: Encryption key mismatch");
        ok = false;
    }

    if (ok)
    {
        Serial.println();
        Serial.println("================================");
        Serial.println("E220 CONFIGURATION OK");
        Serial.println("Mode    : TRANSPARENT");
        Serial.println("Channel : 10");
        Serial.println("UART    : 9600");
        Serial.println("AirRate : SF9 / BW125");
        Serial.println("================================");
    }
    else
    {
        Serial.println();
        Serial.println("================================");
        Serial.println("E220 CONFIGURATION FAILED");
        Serial.println("================================");
    }

    setNormalMode();

    return ok;
}


// ============================================================
// setup
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println("E220 MINIMAL TRANSMITTER");
    Serial.println("ESP32-C6");
    Serial.println("================================");

    pinMode(M0_PIN, OUTPUT);
    pinMode(M1_PIN, OUTPUT);
    pinMode(AUX_PIN, INPUT);

    // ESP32-C6:
    // TX = GPIO16
    // RX = GPIO17

    E220.begin(
        9600,
        SERIAL_8N1,
        E220_RX,
        E220_TX
    );

    Serial.println("UART READY");

    if (!configureE220())
    {
        Serial.println("STOP.");
        return;
    }

    Serial.println();
    Serial.println("START TRANSPARENT TRANSMISSION");
}


// ============================================================
// loop
// ============================================================

void loop()
{
    static uint32_t lastSend = 0;

    if (millis() - lastSend < 1000)
    {
        return;
    }

    lastSend = millis();

    const char message[] = "HELLO\r\n";

    if (!waitAUX())
    {
        Serial.println("AUX BUSY");
        return;
    }

    Serial.print("TX: ");
    Serial.print(message);

    size_t written = E220.write(
        reinterpret_cast<const uint8_t*>(message),
        sizeof(message) - 1
    );

    E220.flush();

    Serial.print("Written: ");
    Serial.println(written);

    waitAUX();
}