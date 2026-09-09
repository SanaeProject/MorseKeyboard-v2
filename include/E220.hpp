/**
 * @note 参考文献: https://support.dragon-torch.tech/docs/lora/E220_ver.2.0/E220_ver.2.0_9
 */
#ifndef E220_HPP
#define E220_HPP

#include <Arduino.h>

#ifndef DEBUG_MODE
    #define DEBUG_MODE false
#endif
#define DEBUG(CODE) if (DEBUG_MODE) { CODE; }

//SECTION - 列挙型
//ANCHOR - モード
enum class E220_Mode : uint8_t {
    NORMAL = 0, // ノーマルモード
    CONFIG = 3  // コンフィグモード
};
//ANCHOR - コマンド
enum class E220_Command : uint8_t {
    WRITE_PERM = 0xC0, // 永続書き込み (EEPROM保存)
    READ       = 0xC1, // レジスタ読み出し
    ACK        = 0xC1, // E220からの応答ヘッダー
    WRITE_TEMP = 0xC2  // 一時書き込み (RAM保存)
};

//SECTION - REG0
//ANCHOR - UARTシリアル通信速度
enum class E220_UARTSerialPortRate : uint8_t {
    RATE_1200   = 0x00, // 000 1200bps
    RATE_2400   = 0x20, // 001 2400bps
    RATE_4800   = 0x40, // 010 4800bps
    RATE_9600   = 0x60, // 011 9600bps
    RATE_19200  = 0x80, // 100 19200bps
    RATE_38400  = 0xa0, // 101 38400bps
    RATE_57600  = 0xc0, // 110 57600bps
    RATE_115200 = 0xe0, // 111 115200bps
};
//ANCHOR - 空中通信速度
enum class E220_AirDataRate : uint8_t {
    // BW = 125kHz
    BW125_15625BPS = 0x00, // 00000 -> SF5,  15.625kbps
    BW125_9375BPS  = 0x04, // 00100 -> SF6,  9.375kbps
    BW125_5469BPS  = 0x08, // 01000 -> SF7,  5.469kbps
    BW125_3125BPS  = 0x0C, // 01100 -> SF8,  3.125kbps
    BW125_1758BPS  = 0x10, // 10000 -> SF9,  1.758kbps (Default)

    // BW = 250kHz
    BW250_31250BPS = 0x01, // 00001 -> SF5,  31.250kbps
    BW250_18750BPS = 0x05, // 00101 -> SF6,  18.750kbps
    BW250_10938BPS = 0x09, // 01001 -> SF7,  10.938kbps
    BW250_6250BPS  = 0x0D, // 01101 -> SF8,  6.250kbps
    BW250_3516BPS  = 0x11, // 10001 -> SF9,  3.516kbps
    BW250_1953BPS  = 0x15, // 10101 -> SF10, 1.953kbps

    // BW = 500kHz
    BW500_62500BPS = 0x02, // 00010 -> SF5,  62.500kbps
    BW500_37500BPS = 0x06, // 00110 -> SF6,  37.500kbps
    BW500_21875BPS = 0x0A, // 01010 -> SF7,  21.875kbps
    BW500_12500BPS = 0x0E, // 01110 -> SF8,  12.500kbps
    BW500_7031BPS  = 0x12, // 10012 -> SF9,  7.031kbps
    BW500_3906BPS  = 0x16, // 10110 -> SF10, 3.906kbps
    BW500_2148BPS  = 0x1A  // 11010 -> SF11, 2.148kbps
};
//!SECTION

//SECTION - REG1
//ANCHOR - ペイロード長
enum class E220_PayloadLength : uint8_t {
    LENGTH_200 = 0x00, // 00 200byte
    LENGTH_128 = 0x40, // 01 128byte
    LENGTH_64  = 0x80, // 10 64byte
    LENGTH_32  = 0xC0, // 11 32byte
};
//ANCHOR - 送信出力
enum class E220_TxPower_22S : uint8_t {
    NOT_AVAILABLE = 0x00,
    POWER_13DBM   = 0x01,
    POWER_7DBM_V1 = 0x02,
    POWER_0DBM    = 0x03,
    POWER_1DBM    = 0x04,
    POWER_2DBM    = 0x05,
    POWER_3DBM    = 0x06,
    POWER_4DBM    = 0x07,
    POWER_5DBM    = 0x08,
    POWER_6DBM    = 0x09,
    POWER_7DBM    = 0x0A,
    POWER_8DBM    = 0x0B,
    POWER_9DBM    = 0x0C,
    POWER_10DBM   = 0x0D,
    POWER_11DBM   = 0x0E,
    POWER_12DBM   = 0x0F
};
//!SECTION

//SECTION - REG3
//ANCHOR - 送信モード
enum class E220_SendMode : uint8_t {
    MODE_TRANSPARENT = 0x00,
    MODE_FIXED       = 0x40
};
//ANCHOR - WORサイクル
enum class E220_WORCycle : uint8_t {
    CYCLE_500MS  = 0x00,
    CYCLE_1000MS = 0x01,
    CYCLE_1500MS = 0x02,
    CYCLE_2000MS = 0x03,
    CYCLE_2500MS = 0x04,
    CYCLE_3000MS = 0x05,
};
//!SECTION
//!SECTION

//ANCHOR - 設定フォーマット
/**
 * @brief E220の設定フォーマット
 */
struct __attribute__((packed)) E220_ConfigFormat {
    E220_Command command;
    uint8_t registerAddress;
    uint8_t length;
    
    uint8_t ADDH;
    uint8_t ADDL;
    uint8_t REG0;
    uint8_t REG1;
    uint8_t REG2;
    uint8_t REG3;

    uint8_t CRYPT_H;
    uint8_t CRYPT_L;
};
//ANCHOR - 設定パケット
union E220_ConfigPacket {
    uint8_t bytes[11];
    E220_ConfigFormat format;
};

//ANCHOR - E220クラス
class E220 {
private:
    E220_ConfigPacket _config;
    HardwareSerial* _serial;

    int8_t _m0Pin;
    int8_t _m1Pin;
    int8_t _auxPin;

    /**
     * @brief モジュールの最大チャンネル数を取得する
     * @return 最大チャンネル数
     * @note データレートによって最大チャンネル数が変わる
     *  BW125kHzでは920.6～928.0MHz (200kHz間隔38波 CH:0～37)
     *  BW250kHzでは920.7～927.9MHz (200kHz間隔37波 CH:0～36)
     *  BW500kHzでは920.8～926.8MHz (200kHz間隔31波 CH:0～30)
     */
    uint8_t _getMaxChannel() const {
        uint8_t airDataRate = this->_config.format.REG0 & 0x1F;
        switch(static_cast<E220_AirDataRate>(airDataRate)) {
            case E220_AirDataRate::BW125_15625BPS:
            case E220_AirDataRate::BW125_9375BPS:
            case E220_AirDataRate::BW125_5469BPS:
            case E220_AirDataRate::BW125_3125BPS:
            case E220_AirDataRate::BW125_1758BPS:
                return 37;
            case E220_AirDataRate::BW250_31250BPS:
            case E220_AirDataRate::BW250_18750BPS:
            case E220_AirDataRate::BW250_10938BPS:
            case E220_AirDataRate::BW250_6250BPS:
            case E220_AirDataRate::BW250_3516BPS:
            case E220_AirDataRate::BW250_1953BPS:
                return 36;
            case E220_AirDataRate::BW500_62500BPS:
            case E220_AirDataRate::BW500_37500BPS:
            case E220_AirDataRate::BW500_21875BPS:
            case E220_AirDataRate::BW500_12500BPS:
            case E220_AirDataRate::BW500_7031BPS:
            case E220_AirDataRate::BW500_3906BPS:
            case E220_AirDataRate::BW500_2148BPS:
                return 30;
        }
        return 0;
    }

    /**
     * @brief AUXピンが待機状態になるまで待機する
     * @return AUXピンが待機状態になった場合はtrue、タイムアウトした場合はfalse
     * @note AUXピンが接続されていない場合は20ms待機する
     * @note AUXピンが接続されている場合は、AUXピンがHIGHになるまで待機する
     */
    bool _waitAUX() const {
        const uint32_t start = millis();
        if (_auxPin >= 0) {
            while (digitalRead(_auxPin) == LOW) {
                if (millis() - start > this->timeout) {
                    DEBUG(Serial.println("E220 Error: Timeout waiting for AUX pin to go HIGH"));
                    return false;
                }
                delay(1);
            }
        } else {
            delay(20);
        }
        return true;
    }

    /**
     * @brief モジュールの動作モードを設定する
     * @param mode 設定するモード (0:ノーマルモード, 3:コンフィグモード)
     * @note M0ピンとM1ピンが接続されていない場合は何もしない
     */
    bool _setMode(E220_Mode mode) const {
        if (!_waitAUX()) return false;
        if (_m0Pin >= 0 && _m1Pin >= 0) {
            switch (mode) {
                case E220_Mode::NORMAL:
                    digitalWrite(_m0Pin, LOW);
                    digitalWrite(_m1Pin, LOW);
                    break;
                case E220_Mode::CONFIG:
                    digitalWrite(_m0Pin, HIGH);
                    digitalWrite(_m1Pin, HIGH);
                    break;
            }
        }
        bool result = _waitAUX();
        delay(50); // モード切替後にAUXが安定するまで少し待つ
        return result;
    }
    /**
     * @brief シリアルバッファをクリアする
     */
    void _clearSerialBuffer() const {
        while (_serial->available()) _serial->read();
    }
    /**
     * @brief モジュールからの応答を読み取る
     * @param buffer 応答を格納するバッファ
     * @param length バッファの長さ
     * @return 読み取りが成功した場合はtrue、失敗した場合はfalse
     */
    bool _readResponse(E220_ConfigPacket* config) const {
        if (!_serial) return false;

        memset(config, 0, sizeof(config->bytes));

        unsigned long start = millis();
        while (millis() - start < this->timeout) {
            if (_serial->available() >= sizeof(config->bytes)) { // 11バイト揃うのを待つ
                _serial->readBytes(config->bytes, sizeof(config->bytes));
                if (config->format.command == E220_Command::ACK)
                    return true;
            }
        }

        DEBUG(Serial.println("E220 Error: Timeout waiting for response"));
        return false;
    }

public:
    uint32_t timeout = 1000; // 読み込みのタイムアウト時間 (ms)


    E220() : _serial(nullptr), _m0Pin(-1), _m1Pin(-1), _auxPin(-1) {
        memset(_config.bytes, 0, sizeof(_config.bytes));
        _config.format.command = E220_Command::WRITE_PERM;
        _config.format.registerAddress = 0x00;
        _config.format.length = 0x08;
    }

    /**
     * @brief E220モジュールの初期化
     * @param serial 使用するHardwareSerialオブジェクト
     * @param m0Pin M0ピンのGPIO番号 (省略可)
     * @param m1Pin M1ピンのGPIO番号 (省略可)
     * @param auxPin AUXピンのGPIO番号 (省略可)
     * @note m0Pinとm1Pinが接続されていない場合は、モジュールの動作モードを変更できない
     * @note auxPinが接続されていない場合は、モジュールの応答を待つことができないため、writeConfig()やreadConfig()の成功判定が正確でなくなる可能性がある
     */
    bool begin(HardwareSerial& serial, int8_t m0Pin = -1, int8_t m1Pin = -1, int8_t auxPin = -1) {
        _serial = &serial;
        _m0Pin = m0Pin;
        _m1Pin = m1Pin;
        _auxPin = auxPin;

        if (_m0Pin >= 0) pinMode(_m0Pin, OUTPUT);
        if (_m1Pin >= 0) pinMode(_m1Pin, OUTPUT);
        if (_auxPin >= 0) pinMode(_auxPin, INPUT_PULLUP);

        if (!_setMode(E220_Mode::NORMAL)) {
            DEBUG(Serial.println("E220 Error: Failed to set normal mode"));
            return false;
        } // デフォルトはノーマルモード

        this->_clearSerialBuffer(); // シリアルバッファをクリアする
        return this->readConfig(); // モジュールの設定を読み込む
    }
    /**
     * @brief データを送信する
     * @param data 送信するデータ
     * @param length 送信するデータの長さ
     * @return 指定した長さをすべて送信できた場合はtrue
     * @note E220は通常モードで受信したUARTデータをそのまま送信する
     * @note 送信モードが固定モードの場合は、透過モードで送信しない。
     */
    bool send(const uint8_t* data, size_t length) {
        if (!_serial || (!data && length > 0)) return false;
        if (length == 0) return true;
        if (!_setMode(E220_Mode::NORMAL)) return false;

        if(this->getSendMode() != E220_SendMode::MODE_TRANSPARENT) return false; // 送信モードが固定モードの場合は、透過モードで送信しない

        const size_t written = _serial->write(data, length);
        _serial->flush();
        _waitAUX();
        return written == length;
    }
    /**
     * @brief データを送信する (アドレスとチャンネルを指定)
     * @param data 送信するデータ
     * @param length 送信するデータの長さ
     * @param targetAddress 送信先アドレス (0x0000～0xFFFF)
     * @param targetChannel 送信先チャンネル (0～37)
     * @note 送信モードが透過モードの場合は、アドレスとチャンネルを指定して送信できない
     */
    bool send(const uint8_t* data, size_t length, uint16_t targetAddress, uint8_t targetChannel) {
        if (!_serial || (!data && length > 0)) return false;
        if (length == 0) return true;
        if (!_setMode(E220_Mode::NORMAL)) return false;

        if(this->getSendMode() == E220_SendMode::MODE_TRANSPARENT) return false; // 送信モードが透過モードの場合は、アドレスとチャンネルを指定して送信できない

        const uint8_t header_size = 3;
        const size_t packetLength = header_size + length;

        uint8_t* packet = new uint8_t[packetLength];
        packet[0] = static_cast<uint8_t>(targetAddress >> 8);   // 高位アドレス
        packet[1] = static_cast<uint8_t>(targetAddress & 0xFF); // 低位アドレス
        packet[2] = targetChannel;                               // チャンネル
        memcpy(packet + header_size, data, length);

        const size_t written = _serial->write(packet, packetLength);
        _serial->flush();
        if (!_waitAUX()) return false;

        delete[] packet;

        return written == packetLength;
    }
    /**
     * @brief null終端文字列を送信する
     * @param data 送信する文字列
     * @return 文字列をすべて送信できた場合はtrue
     * @note 送信モードが固定モードの場合は、透過モードで送信しない
     */
    bool send(const char* data) {
        if (!data) return false;
        return send(reinterpret_cast<const uint8_t*>(data), strlen(data));
    }
    /**
     * @brief null終端文字列を送信する (アドレスとチャンネルを指定)
     * @param data 送信する文字列
     * @param targetAddress 送信先アドレス (0x0000～0xFFFF)
     * @param targetChannel 送信先チャンネル (0～37)
     * @return 文字列をすべて送信できた場合はtrue
     * @note 送信モードが透過モードの場合は、アドレスとチャンネルを指定して送信できない
     */
    bool send(const char* data, uint16_t targetAddress, uint8_t targetChannel) {
        if (!data) return false;
        return send(reinterpret_cast<const uint8_t*>(data), strlen(data), targetAddress, targetChannel);
    }
    /**
     * @brief 受信バッファにあるデータ量を取得する
     * @return 受信可能なバイト数
     */
    int available() const {
        return _serial ? _serial->available() : 0;
    }
    /**
     * @brief 受信バッファから1バイト読み取る
     * @return 受信したバイト、データがない場合は-1
     */
    int read() {
        return _serial ? _serial->read() : -1;
    }
    /**
     * @brief 受信バッファから文字列を読み取る
     * @return 受信した文字列
     */
    String receiveString() {
        String result = "";
        while (available()) {
            int value = read();
            if (value < 0) break;
            result += static_cast<char>(value);
        }
        return result;
    }
    /**
     * @brief 受信データを読み取る
     * @param buffer 受信データを格納するバッファ
     * @param length 最大読み取りバイト数
     * @param timeout 読み取りを待つ最大時間 (ms)。0の場合は非ブロッキング
     * @return 読み取ったバイト数
     */
    size_t receive(uint8_t* buffer, size_t length, uint32_t timeout = 0) {
        if (!_serial || !buffer || length == 0) return 0;

        const unsigned long start = millis();
        size_t received = 0;
        while (received < length) {
            if (_serial->available()) {
                const int value = _serial->read();
                if (value >= 0) buffer[received++] = static_cast<uint8_t>(value);
                continue;
            }

            if (timeout == 0 || millis() - start >= timeout) break;
            delay(1);
        }
        return received;
    }
    /**
     * @brief モジュールに設定を書き込む
     * @param saveType 書き込みタイプ (WRITE_PERM: 永続書き込み, WRITE_TEMP: 一時書き込み)
     * @return 書き込みが成功した場合はtrue、失敗した場合はfalse
     * @note AUXピンが接続されていない場合は、書き込みが成功したかどうかの判定が正確でなくなる可能性がある
     */
    bool writeConfig(E220_Command saveType = E220_Command::WRITE_PERM) {
        if (!_serial)
            return false;
        
        if (!_setMode(E220_Mode::CONFIG)) {
            DEBUG(Serial.println("Failed to set config mode"));
            return false;
        } // コンフィグモードへ移行

        _config.format.command = saveType;
        _config.format.registerAddress = 0x00;
        _config.format.length = 0x08;

        this->_clearSerialBuffer(); // コマンド送信前にシリアルバッファをクリアする

        // 11バイト送信
        _serial->write(_config.bytes, sizeof(_config.bytes));
        _serial->flush();

        // 書込み完了まち
        this->_waitAUX();

        // 返答待ち (ACKチェック)
        bool success = this->_readResponse(&_config);

        _setMode(E220_Mode::NORMAL); // ノーマルモードに戻す
        return success;
    }
    /**
     * @brief モジュールから設定を読み込む
     * @return 読み込みが成功した場合はtrue、失敗した場合はfalse
     * @note AUXピンが接続されていない場合は、読み込みが成功したかどうかの判定が正確でなくなる可能性がある
     */
    bool readConfig()
    {
        if (!_setMode(E220_Mode::CONFIG)) {
            Serial.println("Failed to set config mode");
            return false;
        }
        _clearSerialBuffer();

        uint8_t cmd[] = {static_cast<uint8_t>(E220_Command::READ), 0x00, 0x08};
        _serial->write(cmd, sizeof(cmd));
        _serial->flush();

        return _readResponse(&_config);
    }

    //SECTION Setters
    /**
     * @brief コマンドを設定する
     * @param command 設定するコマンド
     * @return *this
     */
    E220& setCommand(E220_Command command) {
        this->_config.format.command = command;
        return *this;
    }
    /**
     * @brief デバイスアドレスを設定する
     * @param address 設定するアドレス
     * @return *this
     */
    E220& setDeviceAddress(uint16_t address) {
        this->_config.format.ADDH = (address >> 8) & 0xFF;
        this->_config.format.ADDL = address & 0xFF;
        return *this;
    }
    /**
     * @brief 暗号化キーを設定する
     * @param key 設定するキー
     * @return *this
     */
    E220& setCryptKey(uint16_t key) {
        this->_config.format.CRYPT_H = (key >> 8) & 0xFF;
        this->_config.format.CRYPT_L = key & 0xFF;
        return *this;
    }
    /**
     * @brief UARTシリアルポートの通信速度を設定する
     * @param rate 設定する通信速度
     * @return *this
     */
    E220& setUARTSerialPortRate(E220_UARTSerialPortRate rate) {
        this->_config.format.REG0 &= 0x1F;
        this->_config.format.REG0 |= static_cast<uint8_t>(rate);
        return *this;
    }
    /**
     * @brief 空中通信速度を設定する
     * @param rate 設定する空中通信速度
     * @return *this
     */
    E220& setAirDataRate(E220_AirDataRate rate) {
        this->_config.format.REG0 &= 0xE0; // 
        this->_config.format.REG0 |= static_cast<uint8_t>(rate);
        return *this;
    }
    /**
     * @brief ペイロード長を設定する
     * @param length 設定するペイロード長
     * @return *this
     */
    E220& setPayloadLength(E220_PayloadLength length) {
        this->_config.format.REG1 &= 0x3F;
        this->_config.format.REG1 |= static_cast<uint8_t>(length);
        return *this;
    }
    /**
     * @brief RSSIノイズ検出機能を有効にする
     * @param enable 有効にする場合はtrue、無効にする場合はfalse
     * @return *this
     */
    E220& setRSSINoiseEnable(bool enable) {
        if(enable)
            this->_config.format.REG1 |= 0x20;
        else
            this->_config.format.REG1 &= 0xDF;
        return *this;
    }
    /**
     * @brief 送信出力を設定する
     * @param power 設定する送信出力
     * @return *this
     */
    E220& setTxPower(E220_TxPower_22S power) {
        this->_config.format.REG1 &= 0xF0;
        this->_config.format.REG1 |= static_cast<uint8_t>(power);
        return *this;
    }
    /**
     * @brief 周波数チャンネルを設定する
     * @param channel 設定する周波数チャンネル
     * @return *this
     * @note データレートによって最大チャンネル数が変わるため、最大チャンネル数を超える値を設定した場合は最大チャンネル数に丸められる
     */ 
    E220& setFrequencyChannel(uint8_t channel) {
        uint8_t maxChannel = this->_getMaxChannel();
        if(channel > maxChannel) channel = maxChannel;

        this->_config.format.REG2 = channel;
        return *this;
    }
    /**
     * @brief RSSIバイトを有効にする
     * @param enable 有効にする場合はtrue、無効にする場合はfalse
     * @return *this
     */
    E220& setRSSIByteEnable(bool enable) {
        if(enable)
            this->_config.format.REG3 |= 0x80;
        else
            this->_config.format.REG3 &= 0x7F;
        return *this;
    }
    /**
     * @brief 送信モードを設定する
     * @param mode 設定する送信モード
     * @return *this
     */
    E220& setSendMode(E220_SendMode mode)
    {
        this->_config.format.REG3 &= 0xBF; // bit6をクリア
        this->_config.format.REG3 |= static_cast<uint8_t>(mode);

        return *this;
    }
    /**
     * @brief WORサイクルを設定する
     * @param cycle 設定するWORサイクル
     * @return *this
     */
    E220& setWORCycle(E220_WORCycle cycle) {
        this->_config.format.REG3 &= 0xF8;
        this->_config.format.REG3 |= static_cast<uint8_t>(cycle);
        return *this;
    }
    //!SECTION

    //SECTION Getters
    /**
     * @brief コマンドを取得する
     * @return 設定されているコマンド
     */
    uint16_t getDeviceAddress() const {
        return (static_cast<uint16_t>(_config.format.ADDH) << 8) | _config.format.ADDL;
    }
    /**
     * @brief 暗号化キーを取得する
     * @return 設定されている暗号化キー
     */
    uint16_t getCryptKey() const {
        return (static_cast<uint16_t>(_config.format.CRYPT_H) << 8) | _config.format.CRYPT_L;
    }
    /**
     * @brief UARTシリアルポートの通信速度を取得する
     * @return 設定されているUARTシリアルポートの通信速度
     */
    E220_UARTSerialPortRate getUARTSerialPortRate() const {
        return static_cast<E220_UARTSerialPortRate>(_config.format.REG0 & 0xE0);
    }
    /**
     * @brief 空中通信速度を取得する
     * @return 設定されている空中通信速度
     */
    E220_AirDataRate getAirDataRate() const {
        return static_cast<E220_AirDataRate>(_config.format.REG0 & 0x1F);
    }
    /**
     * @brief ペイロード長を取得する
     * @return 設定されているペイロード長
     */
    E220_PayloadLength getPayloadLength() const {
        return static_cast<E220_PayloadLength>(_config.format.REG1 & 0xC0);
    }
    /**
     * @brief RSSIノイズ検出機能が有効かどうかを取得する
     * @return 有効な場合はtrue、無効な場合はfalse
     */
    bool getRSSINoiseEnable() const {
        return (_config.format.REG1 & 0x20) != 0;
    }
    /**
     * @brief 送信出力を取得する
     * @return 設定されている送信出力
     */
    E220_TxPower_22S getTxPower() const {
        return static_cast<E220_TxPower_22S>(_config.format.REG1 & 0x0F);
    }
    /**
     * @brief 周波数チャンネルを取得する
     * @return 設定されている周波数チャンネル
     */
    uint8_t getFrequencyChannel() const {
        return _config.format.REG2;
    }
    /**
     * @brief RSSIバイトが有効かどうかを取得する
     * @return 有効な場合はtrue、無効な場合はfalse
     */
    bool getRSSIByteEnable() const {
        return (_config.format.REG3 & 0x80) != 0;
    }
    /**
     * @brief 送信モードを取得する
     * @return 設定されている送信モード
     */
    E220_SendMode getSendMode() const
    {
        return static_cast<E220_SendMode>(this->_config.format.REG3 & 0x40);
    }
    /**
     * @brief WORサイクルを取得する
     * @return 設定されているWORサイクル
     */
    E220_WORCycle getWORCycle() const {
        return static_cast<E220_WORCycle>(_config.format.REG3 & 0x07);
    }
    /**
     * @brief 設定パケットの生データを取得する
     * @return 設定パケットの生データ
     */
    const uint8_t* rawBytes() const {
        return _config.bytes;
    }
    //!SECTION

    void printConfig() const {
        Serial.println("E220 Configuration:");
        Serial.print("  Device Address: 0x"); Serial.println(getDeviceAddress(), HEX);
        Serial.print("  Crypt Key: 0x"); Serial.println(getCryptKey(), HEX);
        Serial.print("  UART Serial Port Rate: "); Serial.println(static_cast<uint8_t>(getUARTSerialPortRate()));
        Serial.print("  Air Data Rate: "); Serial.println(static_cast<uint8_t>(getAirDataRate()));
        Serial.print("  Payload Length: "); Serial.println(static_cast<uint8_t>(getPayloadLength()));
        Serial.print("  RSSI Noise Enable: "); Serial.println(getRSSINoiseEnable() ? "Enabled" : "Disabled");
        Serial.print("  Tx Power: "); Serial.println(static_cast<uint8_t>(getTxPower()));
        Serial.print("  Frequency Channel: "); Serial.println(getFrequencyChannel());
        Serial.print("  RSSI Byte Enable: "); Serial.println(getRSSIByteEnable() ? "Enabled" : "Disabled");
        Serial.print("  Send Mode: "); Serial.println(static_cast<uint8_t>(getSendMode()));
        Serial.print("  WOR Cycle: "); Serial.println(static_cast<uint8_t>(getWORCycle()));
    }
};

#endif // E220_HPP