#pragma once

#include <Arduino.h>
#include <TDeckMaxBoard.h>

class HalGPIO {
 public:
  HalGPIO() = default;

  void begin();

  void update();
  bool isPressed(uint8_t buttonIndex) const;
  bool wasPressed(uint8_t buttonIndex) const;
  bool wasAnyPressed() const;
  bool wasReleased(uint8_t buttonIndex) const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  bool isRawKeyPressed(uint8_t keyCode) const;
  bool wasRawKeyPressed(uint8_t keyCode) const;
  bool wasRawKeyReleased(uint8_t keyCode) const;
  unsigned long getRawKeyHeldTime(uint8_t keyCode) const;
  int getLastPressedBindableKey() const;

  void startDeepSleep();
  void verifyPowerButtonWakeup(uint16_t requiredDurationMs, bool shortPressAllowed);

  bool isUsbConnected() const;
  bool wasUsbStateChanged() const;

  enum class WakeupReason { PowerButton, AfterFlash, AfterUSBPower, Other };
  WakeupReason getWakeupReason() const;

  static constexpr uint8_t BTN_BACK = 0;
  static constexpr uint8_t BTN_CONFIRM = 1;
  static constexpr uint8_t BTN_LEFT = 2;
  static constexpr uint8_t BTN_RIGHT = 3;
  static constexpr uint8_t BTN_UP = 4;
  static constexpr uint8_t BTN_DOWN = 5;
  static constexpr uint8_t BTN_POWER = 6;
  static constexpr uint8_t BUTTON_COUNT = 7;
  static constexpr uint8_t RAW_BINDABLE_KEY_COUNT = 32;
  static constexpr uint32_t I2C_FREQUENCY = 400000;

 private:
  bool buttonState[BUTTON_COUNT] = {false};
  bool buttonPressedEdge[BUTTON_COUNT] = {false};
  bool buttonReleasedEdge[BUTTON_COUNT] = {false};
  unsigned long buttonPressedSince[BUTTON_COUNT] = {0};
  bool rawKeyState[RAW_BINDABLE_KEY_COUNT] = {false};
  bool rawKeyPressedEdge[RAW_BINDABLE_KEY_COUNT] = {false};
  bool rawKeyReleasedEdge[RAW_BINDABLE_KEY_COUNT] = {false};
  unsigned long rawKeyPressedSince[RAW_BINDABLE_KEY_COUNT] = {0};
  int lastPressedBindableKey = -1;

  bool lastUsbConnected = false;
  bool usbStateChanged = false;
  bool anyPressed = false;
  bool anyReleased = false;
  unsigned long heldTime = 0;
};

extern HalGPIO gpio;
