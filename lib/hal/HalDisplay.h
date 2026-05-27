#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <TDeckMaxBoard.h>

class HalDisplay {
 public:
  HalDisplay();
  ~HalDisplay();

  enum RefreshMode {
    FULL_REFRESH,
    HALF_REFRESH,
    FAST_REFRESH,
  };

  void begin();

  static constexpr uint16_t DISPLAY_WIDTH = LCD_HOR_SIZE;
  static constexpr uint16_t DISPLAY_HEIGHT = LCD_VER_SIZE;
  static constexpr uint16_t DISPLAY_WIDTH_BYTES = (DISPLAY_WIDTH + 7) / 8;
  static constexpr uint32_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT;

  void clearScreen(uint8_t color = 0xFF) const;
  void drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 bool fromProgmem = false) const;
  void drawImageTransparent(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            bool fromProgmem = false) const;

  void displayBuffer(RefreshMode mode = RefreshMode::FAST_REFRESH, bool turnOffScreen = false);
  void refreshDisplay(RefreshMode mode = RefreshMode::FAST_REFRESH, bool turnOffScreen = false);
  void requestFullRefreshNext();

  void deepSleep();
  uint8_t* getFrameBuffer() const;

  void copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer);
  void copyGrayscaleLsbBuffers(const uint8_t* lsbBuffer);
  void copyGrayscaleMsbBuffers(const uint8_t* msbBuffer);
  void cleanupGrayscaleBuffers(const uint8_t* bwBuffer);
  void displayGrayBuffer(bool turnOffScreen = false);

  uint16_t getDisplayWidth() const;
  uint16_t getDisplayHeight() const;
  uint16_t getDisplayWidthBytes() const;
  uint32_t getBufferSize() const;

 private:
  using Panel = GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>;

  Panel epd;
  uint8_t* frameBuffer = nullptr;
  uint8_t* lastDisplayedBuffer = nullptr;
  uint8_t* grayscaleLsbBuffer = nullptr;
  uint8_t* grayscaleMsbBuffer = nullptr;
  uint8_t* ditherBuffer = nullptr;
  bool initialized = false;
  bool requireFullRefresh = true;

  void ensureBuffers();
  void flushBuffer(const uint8_t* buffer, RefreshMode mode, bool turnOffScreen);
  static bool getBufferBit(const uint8_t* buffer, uint16_t x, uint16_t y);
  static void setBufferPixel(uint8_t* buffer, uint16_t x, uint16_t y, bool black);
};

extern HalDisplay display;
