#ifndef OLED_DISPLAY_HELPER_HPP
#define OLED_DISPLAY_HELPER_HPP

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BASE_CHAR_WIDTH  6  // textSize=1のときの文字幅
#define BASE_CHAR_HEIGHT 8  // textSize=1のときの文字高さ

enum class Align {
  Left,
  Center,
  Right
};

enum class SSD1306Color {
  Black   = SSD1306_BLACK,
  White   = SSD1306_WHITE,
  Inverse = SSD1306_INVERSE
};

struct Padding {
  uint16_t top;
  uint16_t right;
  uint16_t left;

  Padding(uint16_t top = BASE_CHAR_HEIGHT / 2, uint16_t right = BASE_CHAR_WIDTH / 2, uint16_t left = BASE_CHAR_WIDTH / 2)
  : top(top), right(right), left(left) {}
};

class OledDisplay {
private:
  Adafruit_SSD1306& _display;
  SSD1306Color      _color;
  uint8_t           _textSize;
  Align             _align;
  Padding           _padding;

  /**
   * @brief 文字列の表示位置を取得する
   * @param length 文字列の長さ
   * @return X座標
   */
  int16_t _getXPos(size_t length) const {
    switch (this->_align) {
      case Align::Left:
        return 0;
      case Align::Center:
        return (this->_display.width() - (length * BASE_CHAR_WIDTH * this->_textSize)) / 2;
      case Align::Right:
        return this->_display.width() - (length * BASE_CHAR_WIDTH * this->_textSize);
    }
    return 0;
  }

  int16_t _getXPadding() const {
    switch (this->_align) {
      case Align::Left:
        return this->_padding.left;
      case Align::Center:
        return 0;
      case Align::Right:
        return -1 * this->_padding.right;
    }
    return 0;
  }

public:
  /**
   * @brief コンストラクタ
   * @param display Adafruit_SSD1306のインスタンス
   * @param textSize 文字サイズ(1~8)
   * @param color 文字色(SSD1306Color::Black, SSD1306Color::White, SSD1306Color::Inverse)
   * @param align 表示位置(Align::Left, Align::Center, Align::Right)
   * @note display.begin()は呼び出されている必要があります。
   */
  OledDisplay(Adafruit_SSD1306& display, uint8_t textSize = 1, SSD1306Color color = SSD1306Color::White, Align align = Align::Left, Padding padding = Padding())
  : _display(display), _textSize(textSize), _color(color), _align(align), _padding(padding) {}

  /**
   * @brief パディングを設定する
   * @param padding パディング
   * @return OledDisplayの参照
   */
  OledDisplay& setPadding(Padding padding) {
    this->_padding = padding;
    return *this;
  }

  /**
   * @brief OLEDの初期化を行う
   * @return OledDisplayの参照
   */
  OledDisplay& init() {
    this->setTextSize(this->_textSize);
    this->setColor(this->_color);
    this->clear();
    return *this;
  }

  /**
   * @brief OLEDの内容を表示する
   * @return OledDisplayの参照
   */
  OledDisplay& display() {
    this->_display.display();
    return *this;
  }

  /**
   * @brief OLEDをクリアする
   * @return OledDisplayの参照
   */
  OledDisplay& clear() {
    this->_display.clearDisplay();
    this->_display.display();
    return *this;
  }

  /**
   * @brief 文字サイズを設定する
   * @param size 文字サイズ
   * @return OledDisplayの参照
   */
  OledDisplay& setTextSize(uint8_t size) {
    this->_textSize = size;
    this->_display.setTextSize(this->_textSize);
    return *this;
  }

  /**
   * @brief 文字色を設定する
   * @param color 文字色(SSD1306Color::Black, SSD1306Color::White, SSD1306Color::Inverse)
   * @return OledDisplayの参照
   */
  OledDisplay& setColor(SSD1306Color color) {
    this->_color = color;
    this->_display.setTextColor((uint16_t)this->_color);
    return *this;
  }

  /**
   * @brief 文字列の表示位置を設定する
   * @param align 表示位置(Align::Left, Align::Center, Align::Right)
   * @return OledDisplayの参照
   */
  OledDisplay& setAlign(Align align) {
    this->_align = align;
    return *this;
  }

  /**
   * @brief カーソル位置を設定する
   * @param x X座標
   * @param y Y座標
   * @return OledDisplayの参照
   */
  OledDisplay& setCursor(int16_t x, int16_t y) {
    this->_display.setCursor(x, y);
    return *this;
  }

  /**
   * @brief 文字列をOLEDに出力する
   * @param str 出力する文字列
   * @param line 出力する行番号(0始まり)。省略した場合はカーソル位置に出力される
   * @return OledDisplayの参照
   */
  OledDisplay& write(const String& str, int16_t line = INT16_MAX) {
    if(line != INT16_MAX) {
      int16_t xPos = this->_getXPos(str.length()) + this->_getXPadding();
      int16_t yPos = line * BASE_CHAR_HEIGHT * this->_textSize + this->_padding.top;
      this->_display.setCursor(xPos, yPos);
    }

    this->_display.print(str);
    return *this;
  }

  /**
   * @brief 文字列をOLEDに出力する
   * @param str 出力する文字列
   * @param line 出力する行番号(0始まり)。省略した場合はカーソル位置に出力される
   * @return OledDisplayの参照
   */
  OledDisplay& print(const String& str, int16_t line = INT16_MAX) {
    this->write(str, line);
    this->_display.display();
    return *this;
  }

  /**
   * @brief 文字列をOLEDに出力する
   * @param str 出力する文字列
   * @param line 出力する行番号(0始まり)。省略した場合はカーソル位置に出力される
   * @return OledDisplayの参照
   */
  OledDisplay& write(const char* str, int16_t line = INT16_MAX) {
    if(line != INT16_MAX) {
      int16_t xPos = this->_getXPos(strlen(str)) + this->_getXPadding();
      int16_t yPos = line * BASE_CHAR_HEIGHT * this->_textSize + this->_padding.top;
      this->_display.setCursor(xPos, yPos);
    }

    this->_display.print(str);
    return *this;
  }

  /**
   * @brief 文字列をOLEDに出力する
   * @param str 出力する文字列
   * @param line 出力する行番号(0始まり)。省略した場合はカーソル位置に出力される
   * @return OledDisplayの参照
   */
  OledDisplay& print(const char* str, int16_t line = INT16_MAX) {
    this->write(str, line);
    this->_display.display();
    return *this;
  }
};

#endif