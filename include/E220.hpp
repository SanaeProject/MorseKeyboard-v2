/**
 * @note 参考文献: https://support.dragon-torch.tech/docs/lora/E220_ver.2.0/E220_ver.2.0_9
 */
#ifndef E220_HPP
#define E220_HPP

#include <Arduino.h>

//SECTION - 列挙型
//ANCHOR - コマンド
enum class E220_Command : uint8_t {
    WRITE_PERM = 0xC0, // 永続書き込み (EEPROM保存)
    READ       = 0xC1, // レジスタ読み出し
    ACK        = 0xC1, // E220からの応答ヘッダー
    WRITE_TEMP = 0xC2  // 一時書き込み (RAM保存)
};
//ANCHOR - UARTシリアル通信速度
enum class E220_UARTSerialPortRate : uint8_t {
    RATE_1200   = 0x00, // 0000 1200bps
    RATE_2400   = 0x20, // 0010 2400bps
    RATE_4800   = 0x40, // 0100 4800bps
    RATE_9600   = 0x60, // 0110 9600bps
    RATE_19200  = 0x08, // 1000 19200bps
    RATE_38400  = 0x0a, // 1010 38400bps
    RATE_57600  = 0x0c, // 1100 57600bps
    RATE_115200 = 0x0e, // 1110 115200bps
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
//ANCHOR - ペイロード長
enum class E220_PayloadLength : uint8_t {
    LENGTH_200 = 0x00, // 0000 200byte
    LENGTH_128 = 0x04, // 0100 128byte
    LENGTH_64  = 0x08, // 1000 64byte
    LENGTH_32  = 0x0c, // 1100 32byte
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
//ANCHOR - 送信モード
enum class E220_SendMode : uint8_t {
    MODE_DEFAULT     = 0x00,
    MODE_TRANSPARENT = 0x20
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
     * @brief AUXピンがLOWになるまで待機する
     * @note AUXピンが接続されていない場合は20ms待機する
     * @note AUXピンが接続されている場合は、AUXピンがHIGHになるまで待機する
     */
    void _waitAUX() const {
        if (_auxPin >= 0) {
            while (digitalRead(_auxPin) == LOW) delay(1);
        } else {
            delay(20);
        }
    }

    /**
     * @brief モジュールの動作モードを設定する
     * @param mode 設定するモード (0:ノーマルモード, 3:コンフィグモード)
     * @note M0ピンとM1ピンが接続されていない場合は何もしない
     */
    void _setMode(uint8_t mode) const {
        _waitAUX();
        delay(10);
        if (_m0Pin >= 0 && _m1Pin >= 0) {
            switch (mode) {
                case 0: // Normal Mode
                    digitalWrite(_m0Pin, LOW);
                    digitalWrite(_m1Pin, LOW);
                    break;
                case 3: // Configuration Mode
                    digitalWrite(_m0Pin, HIGH);
                    digitalWrite(_m1Pin, HIGH);
                    break;
            }
        }
        delay(10);
        _waitAUX();
    }

public:
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
    void begin(HardwareSerial& serial, int8_t m0Pin = -1, int8_t m1Pin = -1, int8_t auxPin = -1) {
        _serial = &serial;
        _m0Pin = m0Pin;
        _m1Pin = m1Pin;
        _auxPin = auxPin;

        if (_m0Pin >= 0) pinMode(_m0Pin, OUTPUT);
        if (_m1Pin >= 0) pinMode(_m1Pin, OUTPUT);
        if (_auxPin >= 0) pinMode(_auxPin, INPUT);

        _setMode(0); // デフォルトはノーマルモード
    }

    /**
     * @brief モジュールに設定を書き込む
     * @param saveType 書き込みタイプ (WRITE_PERM: 永続書き込み, WRITE_TEMP: 一時書き込み)
     * @return 書き込みが成功した場合はtrue、失敗した場合はfalse
     * @note AUXピンが接続されていない場合は、書き込みが成功したかどうかの判定が正確でなくなる可能性がある
     */
    bool writeConfig(E220_Command saveType = E220_Command::WRITE_PERM) {
        if (!_serial) return false;

        _setMode(3); // コンフィグモードへ移行
        
        _config.format.command = saveType;
        _config.format.registerAddress = 0x00;
        _config.format.length = 0x08;

        // シリアルバッファクリア
        while (_serial->available()) _serial->read();

        // 11バイト送信
        _serial->write(_config.bytes, sizeof(_config.bytes));
        _serial->flush();

        // 返答待ち (ACKチェック)
        bool success = false;
        unsigned long start = millis();
        while (millis() - start < 1000) {
            if (_serial->available() >= sizeof(_config.bytes)) {
                uint8_t resBuf[11];
                _serial->readBytes(resBuf, sizeof(resBuf));
                if (resBuf[0] == static_cast<uint8_t>(E220_Command::ACK)) {
                    success = true;
                }
                break;
            }
        }

        _setMode(0); // ノーマルモードに戻す
        return success;
    }

    /**
     * @brief モジュールから設定を読み込む
     * @return 読み込みが成功した場合はtrue、失敗した場合はfalse
     * @note AUXピンが接続されていない場合は、読み込みが成功したかどうかの判定が正確でなくなる可能性がある
     */
    bool readConfig() {
        if (!_serial) return false;

        _setMode(3); // コンフィグモードへ移行

        uint8_t readCmd[3] = {
            static_cast<uint8_t>(E220_Command::READ),
            0x00, // 開始アドレス
            0x08  // 読み出し長
        };

        // シリアルバッファクリア
        while (_serial->available()) _serial->read();

        _serial->write(readCmd, sizeof(readCmd));
        _serial->flush();

        bool success = false;
        unsigned long start = millis();
        while (millis() - start < 1000) {
            if (_serial->available() >= sizeof(_config.bytes)) {
                _serial->readBytes(_config.bytes, sizeof(_config.bytes));
                if (_config.format.command == E220_Command::ACK) {
                    success = true;
                }
                break;
            }
        }

        _setMode(0); // ノーマルモードに戻す
        return success;
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
        this->_config.format.REG0 &= 0xE0;
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
            this->_config.format.REG1 |= 0x10;
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
    E220& setSendMode(E220_SendMode mode) {
        this->_config.format.REG3 &= 0x9F;
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
        return (_config.format.REG1 & 0x10) != 0;
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
    E220_SendMode getSendMode() const {
        return static_cast<E220_SendMode>(_config.format.REG3 & 0x60);
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
};

#endif // E220_HPP