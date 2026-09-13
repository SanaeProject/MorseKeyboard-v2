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

// 動作モード
enum class AppMode {
    MORSE_KEYBOARD,
    LORA_COMMUNICATION,
    SETTINGS,
    RSSI_MONITOR // 新規追加: RSSI ノイズ測定モード
};

AppMode currentMode = AppMode::MORSE_KEYBOARD;

E220                  e220;
MorseSignalReader     morse;
HijelHID_BLEKeyboard  keyboard("MorseKeyboard", "SanaeProject", 100);
Adafruit_SSD1306      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OledDisplay           oledDisplay(display, 1, SSD1306Color::White, Align::Left);

// バッファ管理
String loraTxBuffer = ""; // 送信履歴バッファ
String loraRxBuffer = ""; // 受信履歴バッファ
bool loraRxMessageComplete = false; // 受信メッセージが\nで完了したか

// RSSI測定用
int16_t currentRssi = 0; 

// 設定モード用変数
uint8_t tempChannel = CHANNEL;
uint16_t tempAddress = 0x0001;
int settingState = 0; // 0: メイン設定メニュー, 1: チャンネル設定, 2: アドレス設定

// 画面再描画関数
void updateDisplay();

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // キーボード初期化
    keyboard.begin();

    // ディスプレイの初期化
    if(!display.begin(SSD1306_SWITCHCAPVCC, LCD_I2C_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        while(1); // 失敗時は停止
    } else {
        Serial.println("SSD1306 allocated successfully!");
    }
    oledDisplay.init();

    // E220モジュールの初期化
    e220.setLogLevel(E220_LogLevel::INFO);
    if (!e220.begin(&Serial1, RX_PIN, TX_PIN, M0_PIN, M1_PIN, AUX_PIN, &Serial)){
        Serial.println("E220 begin failed");
    } else {
        e220.setDeviceAddress(tempAddress)
            .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
            .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
            .setPayloadLength(E220_PayloadLength::LENGTH_200)
            .setRSSINoiseEnable(true) // RSSI Noise取得を有効化
            .setTxPower(E220_TxPower_22S::POWER_13DBM)
            .setFrequencyChannel(tempChannel)
            .setRSSIByteEnable(false)
            .setSendMode(E220_SendMode::MODE_TRANSPARENT) // 透過モード
            .setWORCycle(E220_WORCycle::CYCLE_500MS)
            .setCryptKey(0x0000)
            .writeConfig();
    }

    // Morse
    morse.begin(SIGNAL_PIN, DAH_SIGNAL_PIN);

    Serial.println("===========Setup complete===========");
    updateDisplay();
}

void loop()
{
    char key = morse.getKey();

    // --- 各モード処理 ---
    switch(currentMode) {
        case AppMode::MORSE_KEYBOARD: {
            if (key != MORSE_KEY_NONE) {
                Serial.print("Detected key: ");
                Serial.println(key);

                // BLEキーボードへ入力送信
                keyboard.print(key);

                // モード切替判定コマンド
                if (key == 'c') {
                    currentMode = AppMode::SETTINGS;
                    settingState = 0;
                    updateDisplay();
                } else if (key == 'b') {
                    currentMode = AppMode::LORA_COMMUNICATION;
                    updateDisplay();
                } else if (key == 'd') {
                    currentMode = AppMode::RSSI_MONITOR;
                    updateDisplay();
                } else {
                    updateDisplay();
                }
            }
            break;
        }

        case AppMode::LORA_COMMUNICATION: {
            // --- 送信処理（一文字入力ごとに即時送信） ---
            if (key != MORSE_KEY_NONE) {
                // E220モジュールへ1バイト即時送信
                uint8_t sendChar = static_cast<uint8_t>(key);
                e220.send(&sendChar, 1);

                // 送信表示用の履歴に追加
                if (key == '\b') { // バックスペース
                    if (loraTxBuffer.length() > 0) {
                        loraTxBuffer.remove(loraTxBuffer.length() - 1);
                    }
                } else if (key == '\n') { // 改行（確定）
                    loraTxBuffer = ""; // \nを送信した時点で即リセット
                } else { // 通常の文字
                    loraTxBuffer += key;
                    if (loraTxBuffer.length() > 20) { // 画面溢れ防止
                        loraTxBuffer.remove(0, 1);
                    }
                }
                updateDisplay();
            }

            // --- 受信処理（1バイトずつ読み取って画面更新） ---
            bool hasNewRxData = false;
            while (e220.available() > 0) {
                int value = e220.read();
                if (value < 0) break;

                char c = static_cast<char>(value);
                if (c == '\r') continue; // \r は無視

                if (c == '\n') { // 改行（確定）
                    loraRxMessageComplete = true; // 次の文字が来るまで表示は残す
                } else if (c == '\b') { // バックスペース
                    if (loraRxMessageComplete) {
                        loraRxBuffer = "";
                        loraRxMessageComplete = false;
                    } else if (loraRxBuffer.length() > 0) {
                        loraRxBuffer.remove(loraRxBuffer.length() - 1);
                    }
                } else { // 通常の文字
                    if (loraRxMessageComplete) {
                        loraRxBuffer = ""; // 新しい文字が来たら前のメッセージ表示をリセット
                        loraRxMessageComplete = false;
                    }
                    loraRxBuffer += c;
                    if (loraRxBuffer.length() > 20) { // 画面溢れ防止
                        loraRxBuffer.remove(0, 1);
                    }
                }
                hasNewRxData = true;
            }

            if (hasNewRxData) {
                updateDisplay();
            }
            break;
        }

        case AppMode::SETTINGS: {
            if (key != MORSE_KEY_NONE) {
                if (settingState == 0) {
                    // メイン設定メニュー
                    if (key == 'a') {
                        settingState = 1; // チャンネル変更
                    } else if (key == 'b') {
                        settingState = 2; // アドレス変更
                    } else if (key == 'c') {
                        // 設定書き込み
                        e220.setFrequencyChannel(tempChannel);
                        e220.setDeviceAddress(tempAddress);
                        e220.writeConfig();
                        currentMode = AppMode::MORSE_KEYBOARD;
                    } else if (key == 'd') {
                        // キャンセルして戻る
                        currentMode = AppMode::MORSE_KEYBOARD;
                    }
                } else if (settingState == 1) {
                    // チャンネル変更 (a:+1, b:-1, c:決定)
                    if (key == 'a') tempChannel++;
                    else if (key == 'b' && tempChannel > 0) tempChannel--;
                    else if (key == 'c') settingState = 0;
                } else if (settingState == 2) {
                    // アドレス変更 (a:+1, b:-1, c:決定)
                    if (key == 'a') tempAddress++;
                    else if (key == 'b' && tempAddress > 0) tempAddress--;
                    else if (key == 'c') settingState = 0;
                }
                updateDisplay();
            }
            break;
        }

        case AppMode::RSSI_MONITOR: {
            // 'a'キーでメインに戻る
            if (key != MORSE_KEY_NONE) {
                if (key == 'a') {
                    currentMode = AppMode::MORSE_KEYBOARD;
                    updateDisplay();
                    break;
                }
            }

            // 1秒間隔でRSSIを更新
            static uint32_t lastRssiUpdate = 0;
            if (millis() - lastRssiUpdate > 1000) {
                lastRssiUpdate = millis();
                currentRssi = e220.getRSSINoise();
                updateDisplay();
            }
            break;
        }
    }
}

// -------------------------------------------------------------
// 画面の更新処理
// -------------------------------------------------------------
void updateDisplay()
{
    display.clearDisplay();

    switch(currentMode) {
        case AppMode::MORSE_KEYBOARD:
            oledDisplay.setCursor(0, 0);
            oledDisplay.write("=== BLE KEYBOARD ===");
            oledDisplay.write("a: .-  b: -...", 2);
            oledDisplay.write("b:LoRa c:Config", 4);
            oledDisplay.write("d:RSSI Monitor", 6);
            break;

        case AppMode::LORA_COMMUNICATION:
            // 上画面: 送信文字エリア
            oledDisplay.setCursor(0, 0);
            oledDisplay.write("TX: " + loraTxBuffer);

            // 中央の区切り線
            oledDisplay.drawLine(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2);

            // 下画面: 受信文字エリア
            oledDisplay.setCursor(0, (SCREEN_HEIGHT / 2) + 4);
            oledDisplay.write("RX: " + loraRxBuffer);
            break;

        case AppMode::SETTINGS:
            oledDisplay.setCursor(0, 0);
            if (settingState == 0) {
                oledDisplay.write("=== E220 CONFIG ===");
                oledDisplay.write("a: CH (" + String(tempChannel) + ")", 2);
                oledDisplay.write("b: Addr (" + String(tempAddress) + ")", 4);
                oledDisplay.write("c: Save  d: Exit", 6);
            } else if (settingState == 1) {
                oledDisplay.write("Set Channel: " + String(tempChannel));
                oledDisplay.write("a: +1  b: -1", 3);
                oledDisplay.write("c: OK", 5);
            } else if (settingState == 2) {
                oledDisplay.write("Set Addr: " + String(tempAddress));
                oledDisplay.write("a: +1  b: -1", 3);
                oledDisplay.write("c: OK", 5);
            }
            break;

        case AppMode::RSSI_MONITOR: {
            oledDisplay.setCursor(0, 0);
            oledDisplay.write("=== RSSI Noise ===");
            
            String rssiStr = "Value: " + String(currentRssi) + " dBm";
            oledDisplay.write(rssiStr, 2);

            // バーグラフの外枠を描画 (100px幅)
            display.drawRect(14, 32, 100, 10, SSD1306_WHITE);
            
            // 値をバーグラフの幅(0〜100)にマッピング (-130dBmを0%, -30dBmを100%と仮定)
            int barWidth = map(currentRssi, -130, -30, 0, 100);
            barWidth = constrain(barWidth, 0, 100); // 範囲外のはみ出しを防ぐ
            
            // バーグラフの中身を塗りつぶす
            display.fillRect(14, 32, barWidth, 10, SSD1306_WHITE);

            oledDisplay.write("a: Back to Menu", 6);
            break;
        }
    }

    display.display();
}