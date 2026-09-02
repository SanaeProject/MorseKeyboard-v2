#ifndef MORSE_KEYBOARD_HPP
#define MORSE_KEYBOARD_HPP

#include <Arduino.h>
#include "timer.hpp"

// <定数宣言>
#define MORSE_KEY_NONE 0            // 返却値なし
#define MORSE_MAX_LENGTH 5          // モールス信号の最大長
#define MORSE_DAH_DURATION_MS 200   // モールス信号ダッシュ長
#define MORSE_END_DURATION_MS 1000  // モールス信号終了判定時間
#define MORSE_INTERVAL_MS     200   // ボタン押下、判定後待機時間

// モールス信号構造体
typedef struct {
    uint8_t code[MORSE_MAX_LENGTH]; // 1: dot signal, 2: dah signal
    char ch;
}MorseCode;
// 対応表
const MorseCode morseDictionary[] = {
    {{1, 2}         , 'a'}, // a: ・-
    {{2, 1, 1, 1}   , 'b'}, // b: -・・・
    {{2, 1, 2, 1}   , 'c'}, // c: -・-・
    {{2, 1, 1}      , 'd'}, // d: -・・
    {{1}            , 'e'}, // e: ・
    {{1, 1, 2, 1}   , 'f'}, // f: ・・-・
    {{2, 2, 1}      , 'g'}, // g: --・
    {{1, 1, 1, 1}   , 'h'}, // h: ・・・・
    {{1, 1}         , 'i'}, // i: ・・
    {{1, 2, 2, 2}   , 'j'}, // j: ・---
    {{2, 1, 2}      , 'k'}, // k: -・-
    {{1, 2, 1, 1}   , 'l'}, // l: ・-・・
    {{2, 2}         , 'm'}, // m: --
    {{2, 1}         , 'n'}, // n: -・
    {{2, 2, 2}      , 'o'}, // o: ---
    {{1, 2, 2, 1}   , 'p'}, // p: ・--・
    {{2, 2, 1, 2}   , 'q'}, // q: --・-
    {{1, 2, 1}      , 'r'}, // r: ・-・
    {{1, 1, 1}      , 's'}, // s: ・・・
    {{2}            , 't'}, // t: -
    {{1, 1, 2}      , 'u'}, // u: ・・-
    {{1, 1, 1, 2}   , 'v'}, // v: ・・・-
    {{1, 2, 2}      , 'w'}, // w: ・--
    {{2, 1, 1, 2}   , 'x'}, // x: -・・-
    {{2, 1, 2, 2}   , 'y'}, // y: -・--
    {{2, 2, 1 ,1}   , 'z'}, // z: --・・
    {{2, 2, 2, 2, 2}, '0'}, // 0: -----
    {{1, 2, 2, 2, 2}, '1'}, // 1: ・----
    {{1, 1, 2, 2, 2}, '2'}, // 2: ・・---
    {{1, 1, 1, 2, 2}, '3'}, // 3: ・・・--
    {{1, 1, 1, 1, 2}, '4'}, // 4: ・・・・-
    {{1, 1, 1, 1, 1}, '5'}, // 5: ・・・・・
    {{2, 1, 1, 1, 1}, '6'}, // 6: -・・・・
    {{2, 2, 1, 1 ,1}, '7'}, // 7: --・・・
    {{2, 2, 2 ,1 ,1}, '8'}, // 8: ---・・
    {{2 ,2 ,2 ,2 ,1}, '9'}, // 9: ----・
};
const uint64_t dictionaryLength = sizeof(morseDictionary) / sizeof(MorseCode); // dictionary length

class MorseSignalReader {
private:
    uint32_t _signalPin;
    uint32_t _dahSignalPin;
    uint8_t  _signals[MORSE_MAX_LENGTH];

    bool     _isStarted           = false;
    bool     _isPreviousPushed    = false;

    uint8_t  _signalIdx           = 0;
    Timer    _buttonPressDuration;
    Timer    _intervalDuration;

protected:
    inline char _searchDictionary(uint8_t* signals){
        Serial.print("signals:");
        for(uint8_t i = 0; i < MORSE_MAX_LENGTH; i++){
            Serial.print(signals[i]);
        }
        Serial.println();

        for(uint64_t i = 0; i < dictionaryLength; i++){
            const uint8_t* target = morseDictionary[i].code;
            bool isIncorrect = false;

            for(uint64_t j = 0; j < MORSE_MAX_LENGTH; j++){
                if(target[j] != signals[j]) {
                    isIncorrect = true;
                    break;
                }
            }
            if(!isIncorrect) return morseDictionary[i].ch;
        }

        return '?';
    }
    inline bool _getDahSignal(){
        if(this->_dahSignalPin == UINT32_MAX) return false;
        return digitalRead(this->_dahSignalPin) == LOW;
    }

public:
    /**
     * @brief モールス信号入力の初期化
     * @param signalPin モールス信号入力ピン
     * @param dahSignalPin 長音信号入力ピン(省略可)
     * @note dahSignalPinを省略した場合、signalPinの押下時間で短音・長音を判定する
     */
    void begin(uint32_t signalPin, uint32_t dahSignalPin = UINT32_MAX){
        this->_signalPin = signalPin;
        this->_dahSignalPin = dahSignalPin;
        pinMode(this->_signalPin, INPUT_PULLUP);

        if(dahSignalPin != UINT32_MAX)
            pinMode(this->_dahSignalPin, INPUT_PULLUP);
    }

    /**
     * @brief モールス信号の判定
     * @return 判定結果の文字列
     * @note 非同期で呼び出すことを想定しているため、判定結果がない場合はMORSE_KEY_NONEを返却する
     */
    char getKey(){
        const bool signal = digitalRead(this->_signalPin) == LOW;
        const bool dahSignal = this->_getDahSignal();

        // 待機時間中は返却なし
        if(this->_intervalDuration.isRunning() && 
            this->_intervalDuration.elapsed() < MORSE_INTERVAL_MS) return MORSE_KEY_NONE;

        // キー判定開始
        if(!this->_isStarted && (signal || dahSignal)){
            this->_isStarted = true;
            this->_buttonPressDuration.start();
        }
        if(!this->_isStarted) return MORSE_KEY_NONE;

        // キー判定
        if((!signal && !this->_isPreviousPushed && MORSE_END_DURATION_MS < this->_buttonPressDuration.elapsed()) || // ボタンが押下されずに一定時間たった場合
            this->_signalIdx == MORSE_MAX_LENGTH) // シグナルが最大長に達した場合
        {
            this->_isStarted = false;
            this->_signalIdx = 0;

            char result = _searchDictionary(this->_signals);
            memset(this->_signals, 0, MORSE_MAX_LENGTH);
            
            this->_intervalDuration.start();
            return result;
        }

        // 長音単音判定
        if(!signal && this->_isPreviousPushed){ // ボタンが押されていない且つ前回は押されていた場合短音と長音を判定
            this->_signals[this->_signalIdx] = this->_buttonPressDuration.elapsed() < MORSE_DAH_DURATION_MS ? 1 : 2;
            this->_signalIdx++;
            this->_intervalDuration.start();
        }
        // 押下状態に過去と変更があった場合タイマーをリセット
        if(signal != this->_isPreviousPushed) this->_buttonPressDuration.start();

        // 長音キーがある場合、長音キーの状態を確認して長音信号を追加する
        if(dahSignal){
            this->_signals[this->_signalIdx] = 2;
            this->_signalIdx++;
            this->_intervalDuration.start();
        }

        this->_isPreviousPushed = signal; // 過去の押下状態を保存

        return MORSE_KEY_NONE;
    }
};

#endif // MORSE_KEYBOARD_HPP