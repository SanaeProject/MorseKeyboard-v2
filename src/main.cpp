#include <Arduino.h>
#include "Morse.hpp"
#include "OledDisplay.hpp"
#include "E220.hpp"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HijelHID_BLEKeyboard.h>

#define ERROR(code) while(1){code; delay(1000);}
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

// Morse
#define SIGNAL_PIN      21

// 動作モード
enum class AppMode {
    MAIN_MENU,
    MORSE_KEYBOARD,
    LORA_COMMUNICATION,
    SETTINGS,
    RSSI_MONITOR,
    ABOUT
};

//SECTION Global Variables
AppMode currentMode = AppMode::MAIN_MENU; // default

E220                  e220;
MorseSignalReader     morse;
HijelHID_BLEKeyboard  keyboard("MorseKeyboard", "SanaeProject", 100);
Adafruit_SSD1306      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OledDisplay           oledDisplay(display, 1, SSD1306Color::White, Align::Left);

// Settings
// Settings
uint16_t tempAddress = 0;
uint8_t  tempChannel = 0;
uint16_t tempTargetAddress = 0;
uint16_t tempCryptKey = 0;

enum class SettingsItem {
    ADDRESS,
    CHANNEL,
    SEND_MODE,
    TARGET_ADDRESS,
    CRYPT_KEY
};

SettingsItem settingsItem = SettingsItem::ADDRESS;
String settingsInput = "";

// Lora
constexpr uint8_t MAX_MESSAGE_ROWS = 3;
uint8_t receivedMessageRow = 0;
uint8_t receivedMessageCol = 0;
String receivedMessage = "";

uint8_t sentMessageRow = 0;
uint8_t sentMessageCol = 0;
String sentMessage = "";

// RSSI Noise Monitor
constexpr size_t GRAPH_LENGTH = 5;
constexpr size_t GRAPH_REFRESH = 1000; // 1秒ごとに更新
int16_t noises[GRAPH_LENGTH];
size_t lastIdx = GRAPH_LENGTH-1;
Timer rssiTimer;
//!SECTION

//SECTION プロトタイプ宣言
void mainMenu();          //ANCHOR - Main Menu
void loraCommunication(); //ANCHOR - LoRa Communication
void morseKeyboard();     //ANCHOR - Morse Keyboard
void rssiMonitor();       //ANCHOR - RSSI Noise Monitor
void settings();          //ANCHOR - Settings
//!SECTION

//SECTION entry
//ANCHOR Setup
void setup(){
    // シリアル通信の初期化
    Serial.begin(115200);

    // キーボード初期化
    keyboard.begin();

    // ディスプレイの初期化
    if(display.begin(SSD1306_SWITCHCAPVCC, LCD_I2C_ADDRESS))
        Serial.println("SSD1306 allocated successfully!");
    else
        ERROR(Serial.println("SSD1306 allocation failed"));
    oledDisplay.init();
    oledDisplay.clear();

    // E220モジュールの初期化
    e220.setLogLevel(E220_LogLevel::INFO);
    if (!e220.begin(&Serial1, RX_PIN, TX_PIN, M0_PIN, M1_PIN, AUX_PIN, &Serial))
        ERROR(Serial.println("E220 begin failed"));
    if(!e220.readConfig())
        ERROR(Serial.println("E220 Failed to config read."));
    
    // 前回の引継ぎ
    tempAddress = e220.getDeviceAddress();
    tempChannel = e220.getFrequencyChannel();
    tempTargetAddress = tempAddress;
    tempCryptKey = e220.getCryptKey();

    const bool written = e220.setDeviceAddress(tempAddress)
        .setUARTSerialPortRate(E220_UARTSerialPortRate::RATE_9600)
        .setAirDataRate(E220_AirDataRate::BW125_1758BPS)
        .setPayloadLength(E220_PayloadLength::LENGTH_200)
        .setRSSINoiseEnable(true)
        .setTxPower(E220_TxPower_22S::POWER_13DBM)
        .setFrequencyChannel(tempChannel)
        .setRSSIByteEnable(false)
        .setSendMode(E220_SendMode::MODE_TRANSPARENT)
        .setWORCycle(E220_WORCycle::CYCLE_500MS)
        .setCryptKey(tempCryptKey)
        .writeConfig();
    if (!written)
        ERROR(Serial.println("E220 Failed to config write."));

    // Morse
    morse.begin(SIGNAL_PIN);

    // 完了画面
    Serial.println("===========Setup complete===========");
    oledDisplay.setTextSize(1);
    oledDisplay.setAlign(Align::Center)
        .clear()
        .setCursor(0, oledDisplay.getHeight() / 2)
        .println("Morse Keyboard v2")
        .setAlign(Align::Right)
        .println("by SanaeProject")
        .nextLine()
        .setAlign(Align::Left)
        .println("Author: SanaeProject")
        .println("Version: 1.0.0");
        
    delay(2000);
    oledDisplay.clear();
}

bool isPressed = false;
// ANCHOR Loop
void loop(){
    bool pressed = digitalRead(SIGNAL_PIN) == LOW;
    if(pressed && !isPressed){
        Serial.println("Button pressed");
        isPressed = true;
    } else if(!pressed && isPressed){
        Serial.println("Button released");
        isPressed = false;
    }

    switch(currentMode){
    case AppMode::MAIN_MENU:
        mainMenu();
        break;
    case AppMode::LORA_COMMUNICATION:
        loraCommunication();
        break;
    case AppMode::MORSE_KEYBOARD:
        morseKeyboard();
        break;
    case AppMode::RSSI_MONITOR:
        rssiMonitor();
        break;
    case AppMode::SETTINGS:
        settings();
        break;
    }

    delay(2);
}
//!SECTION

//SECTION pages
//ANCHOR - Main Menu
void mainMenu(){
    // setup
    oledDisplay.setTextSize(1)
        .setCursor(0, 0)
        .setAlign(Align::Center)
        .println("Morse Keyboard")
        .drawRect(0, oledDisplay.getCursorY(), oledDisplay.getWidth(), 1, SSD1306Color::White);

    // content
    oledDisplay.nextLine()
        .setAlign(Align::Left)
        .println("E(.)   :Keyboard")
        .println("T(-)   :LoRaComm")
        .println("A(.-)  :RSSIMonitor")
        .println("I(..)  :Settings")
        .println("M(--)  :About");

    char key = morse.getKey();
    AppMode previousMode = currentMode;
    switch(key){
    case 'e':
        currentMode = AppMode::MORSE_KEYBOARD;
        break;
    case 't':
        currentMode = AppMode::LORA_COMMUNICATION;
        break;
    case 'a':
        currentMode = AppMode::RSSI_MONITOR;
        break;
    case 'i':
        currentMode = AppMode::SETTINGS;
        break;
    case 'm':
        currentMode = AppMode::ABOUT;
        break;
    case MORSE_KEY_NONE:
        break;
    default:
        Serial.print("Invalid input: " + String(key));
        oledDisplay.setAlign(Align::Center)
            .clearLine()
            .print("Invalid input: " + String(key));

        break;
    }

    if(previousMode != currentMode)
        oledDisplay.clear();
}

// ANCHOR - LoRa Communication
void loraInsertBuffer(const char c, String& message, uint8_t& row, uint8_t& col){
    if(c == '\r') return; // 改行コードは無視

    message += c;
    col++;
    uint16_t width = (col + 1) * BASE_CHAR_WIDTH + oledDisplay.getPadding().left;
    bool isLineBreak = (c == '\n') || (width >= oledDisplay.getWidth());

    if(width >= oledDisplay.getWidth()){
        col = 0;
        message += '\n';
    }
    if(isLineBreak) row++;

    if(row >= MAX_MESSAGE_ROWS){ // 行数超過時に最古の1行を削除
        int firstNewline = message.indexOf('\n');
        if(firstNewline != -1)
            message = message.substring(firstNewline + 1);
        
        row--;
        oledDisplay.clear();
    }
}
void loraCommunication() {
    // 描画準備
    oledDisplay.setTextSize(1)
        .setCursor(0, 0);
    
    oledDisplay.setAlign(Align::Left)
        .println("Send:\n" + sentMessage)
        .setCursor(0, oledDisplay.getHeight() / 2)
        .fillRect(0, oledDisplay.getCursorY(), oledDisplay.getWidth(), oledDisplay.getHeight() / 2, SSD1306Color::Black)
        .drawRect(0, oledDisplay.getCursorY(), oledDisplay.getWidth(), 1, SSD1306Color::White)
        .println("Received:\n" + receivedMessage);
    oledDisplay.display();

    // 入力・送信処理
    char key = morse.getKey();
    if (key == '\e'){
        currentMode = AppMode::MAIN_MENU;
        oledDisplay.clear();
        return;
    }
    if (key != MORSE_KEY_NONE && key != '?') {
        Serial.print("Morse Key: ");
        Serial.println(key);

        if(e220.getSendMode() == E220_SendMode::MODE_TRANSPARENT)
            e220.send((uint8_t*)&key, 1);
        else
            e220.send((uint8_t*)&key, 1, tempTargetAddress,tempChannel);

        loraInsertBuffer(key, sentMessage, sentMessageRow, sentMessageCol);
    }

    // 受信データの処理
    while (e220.available()) {
        char c = e220.read();
        loraInsertBuffer(c, receivedMessage, receivedMessageRow, receivedMessageCol);
    }
}

//ANCHOR - Morse Keyboard
void morseKeyboard(){
    oledDisplay.setTextSize(1)
        .setCursor(0, 0)
        .setAlign(Align::Center)
        .println("Morse Keyboard")
        .drawRect(0, oledDisplay.getCursorY(), oledDisplay.getWidth(), 1, SSD1306Color::White)
        .nextLine();

    if(!keyboard.isConnected()){
        Serial.println("Keyboard is not connected.");
        oledDisplay.clearLine()
            .setAlign(Align::Left)
            .println("Keyboard is not connected.");
    }
    char key = morse.getKey();

    if(key == '\e'){
        currentMode = AppMode::MAIN_MENU;
        oledDisplay.clear();
        return;
    }
    if(key != MORSE_KEY_NONE){
        Serial.print("Morse Key: " + String(key));
        oledDisplay.clearLine()
            .setAlign(Align::Center)
            .println("Morse Key: " + String(key));
        keyboard.print(key);
    }
}

//ANCHOR - RSSI Monitor
size_t getFirstIdx(){
    return lastIdx+1 < GRAPH_LENGTH ? lastIdx+1 : 0;
}
size_t getNextIdx(size_t idx){
    return idx+1 < GRAPH_LENGTH ? idx+1 : 0;
}
void viewGraph(){
    const int16_t max = 0;
    const int16_t min = -110;
    const int16_t paddingX = BASE_CHAR_WIDTH * 4; // 4文字分パディング

    const int16_t x = paddingX,
    y      = oledDisplay.getHeight()/ 4,
    width  = oledDisplay.getWidth() - 2*paddingX,
    height = oledDisplay.getHeight()/ 4*3;
    
    // 初期化
    oledDisplay.setCursor(0, 0)
        .fillRect(x+1, y+1, width-2, height-2, SSD1306Color::Black);

    // 外枠
    oledDisplay.setCursor(0, y).print(String(min)) // 一番上
        .setCursor(0, y + height - BASE_CHAR_HEIGHT).print(String(max)) // 真ん中
        .setCursor(0, y+height/2 - BASE_CHAR_HEIGHT/2).print(String((max+min)/2)) // 一番下
        .drawLine(x, y+height/2, x+width, y+height/2)   // 真ん中線
        .setCursor(0, 0).drawRect(x, y, width, height); // 外枠

    const int16_t deltaGraphX = width / GRAPH_LENGTH;
    const int16_t circleRadius = 2;
    int16_t prevY = y + height/2 - (noises[lastIdx] - min) * height / (max - min);
    for(size_t cnt = 0, idx = getFirstIdx();cnt < GRAPH_LENGTH; cnt++, idx = getNextIdx(idx)){
        int16_t graphY = y + height/2 - (noises[idx] - min) * height / (max - min);
        int16_t graphX = x + cnt * deltaGraphX;

        graphY = constrain(graphY, y + circleRadius, y + height - circleRadius);
        oledDisplay.drawCircle(graphX, graphY, circleRadius);
        if(cnt > 0)
            oledDisplay.drawLine(x + (cnt-1) * deltaGraphX, prevY, graphX, graphY);
        prevY = graphY;
    }

    oledDisplay.display();
}
void rssiMonitor(){
    // エスケープ
    char key = morse.getKey();
    if(key == '\e'){
        currentMode = AppMode::MAIN_MENU;
        oledDisplay.clear();
    }

    // 1秒ごとに更新
    if(!rssiTimer.isRunning() || rssiTimer.elapsed() >= GRAPH_REFRESH){
        rssiTimer.start();
    }else
        return;

    oledDisplay.setTextSize(1)
        .setCursor(0, 0);

    int16_t rssi = e220.getRSSINoise();
    oledDisplay.clearLine()
        .setAlign(Align::Center)
        .println("RSSI: " + String(rssi) + " dBm");

    // 値を格納
    lastIdx = getFirstIdx();
    noises[lastIdx] = rssi;

    viewGraph();
}
//ANCHOR - Settings
//ANCHOR - Settings
void settings(){
    static SettingsItem item = SettingsItem::ADDRESS;
    static bool editing = false;
    static bool dirty = true; // 描画更新が必要かどうかのフラグ

    // --- 1. 入力処理 ---
    char key = morse.getKey();

    if(key != MORSE_KEY_NONE){
        dirty = true; // キー入力があったので画面を更新する

        // ESC (戻る / キャンセル)
        if(key == '\e'){
            if(editing){
                editing = false;
            }else{
                currentMode = AppMode::MAIN_MENU;
            }
            oledDisplay.clear();
            return;
        }

        // 編集中（値の変更）
        if(editing){
            switch(item){
            case SettingsItem::ADDRESS:
                if(key == 'e') { if(tempAddress < 0xFFFF) ++tempAddress; }
                else if(key == 't') { if(tempAddress > 0) --tempAddress; }
                break;

            case SettingsItem::CHANNEL:
                if(key == 'e') { if(tempChannel < 37) ++tempChannel; }
                else if(key == 't') { if(tempChannel > 0) --tempChannel; }
                break;

            case SettingsItem::SEND_MODE:
                if(key == 'e' || key == 't'){
                    if(e220.getSendMode() == E220_SendMode::MODE_TRANSPARENT)
                        e220.setSendMode(E220_SendMode::MODE_FIXED);
                    else
                        e220.setSendMode(E220_SendMode::MODE_TRANSPARENT);
                }
                break;

            case SettingsItem::TARGET_ADDRESS:
                if(key == 'e') { if(tempTargetAddress < 0xFFFF) ++tempTargetAddress; }
                else if(key == 't') { if(tempTargetAddress > 0) --tempTargetAddress; }
                break;

            case SettingsItem::CRYPT_KEY:
                if(key == 'e') { if(tempCryptKey < 0xFFFF) ++tempCryptKey; }
                else if(key == 't') { if(tempCryptKey > 0) --tempCryptKey; }
                break;
            }

            // Enter('\n')で決定
            if(key == '\n'){
                editing = false;
            }

            oledDisplay.clear();
        } 
        // 項目選択モード
        else {
            switch(key){
            case 'e': // (.) Address
                item = SettingsItem::ADDRESS;
                editing = true;
                break;

            case 't': // (-) Channel
                item = SettingsItem::CHANNEL;
                editing = true;
                break;

            case 'a': // (.-) SendMode
                item = SettingsItem::SEND_MODE;
                editing = true;
                break;

            case 'i': // (..) Target Address
                item = SettingsItem::TARGET_ADDRESS;
                editing = true;
                break;

            case 'm': // (--) Crypt Key
                item = SettingsItem::CRYPT_KEY;
                editing = true;
                break;

            case '\n': // 保存
                e220.setDeviceAddress(tempAddress)
                    .setFrequencyChannel(tempChannel)
                    .setCryptKey(tempCryptKey)
                    .writeConfig();

                oledDisplay.clear();
                break;
            }

            oledDisplay.clear();
        }
    }

    // --- 2. 描画処理（変化があった時だけ描画する） ---
    if(!dirty) return;
    dirty = false; // 描画処理を行うのでフラグを下げる

    oledDisplay.setTextSize(1)
        .setCursor(0, 0)
        .setAlign(Align::Center)
        .println("Settings")
        .drawRect(
            0,
            oledDisplay.getCursorY(),
            oledDisplay.getWidth(),
            1,
            SSD1306Color::White
        );

    oledDisplay.nextLine()
        .setAlign(Align::Left);

    if(!editing){
        oledDisplay.println("E(.)  : Address");
        oledDisplay.println("T(-)  : Channel");
        oledDisplay.println("A(.-) : SendMode");
        oledDisplay.println("I(..) : TargetAddr");
        oledDisplay.println("M(--) : CryptKey");
        oledDisplay.println("Enter : Save");
        oledDisplay.println("ESC   : Back");
    }else{
        switch(item){
        case SettingsItem::ADDRESS:
            oledDisplay.println("Address");
            oledDisplay.println("0x" + String(tempAddress, HEX));
            break;

        case SettingsItem::CHANNEL:
            oledDisplay.println("Channel");
            oledDisplay.println(String(tempChannel));
            break;

        case SettingsItem::SEND_MODE:
            oledDisplay.println("SendMode");
            oledDisplay.println(
                e220.getSendMode() == E220_SendMode::MODE_TRANSPARENT
                    ? "TRANSPARENT"
                    : "FIXED"
            );
            break;

        case SettingsItem::TARGET_ADDRESS:
            oledDisplay.println("Target Addr");
            oledDisplay.println("0x" + String(tempTargetAddress, HEX));
            break;

        case SettingsItem::CRYPT_KEY:
            oledDisplay.println("CryptKey");
            oledDisplay.println("0x" + String(tempCryptKey, HEX));
            break;
        }

        oledDisplay.println("")
            .println("E(.): UP / T(-): DOWN")
            .println("Enter: OK")
            .println("ESC: Cancel");
    }

    oledDisplay.display();
}
//!SECTION