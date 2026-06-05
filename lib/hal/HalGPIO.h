#pragma once

#include <Arduino.h>
#include <TDeckMaxBoard.h>

struct HalChargerStatusSnapshot {
  bool chargerReady = false;
  bool readOk = false;
  bool vbusConnected = false;
  bool charging = false;
  bool chargeDone = false;
  uint8_t busType = 0xFF;
  uint8_t chargeState = 0xFF;
  uint8_t faultStatusRaw = 0;
  uint16_t vbusVoltageMv = 0;
  uint16_t systemVoltageMv = 0;
  uint16_t batteryVoltageMv = 0;
  uint16_t chargeCurrentAdcMa = 0;
  uint16_t inputLimitMa = 0;
  uint16_t targetVoltageMv = 0;
  uint16_t targetCurrentMa = 0;
  uint16_t prechargeCurrentMa = 0;
};

class HalGPIO {
 public:
  HalGPIO() = default;

  void begin();
  void setScreenBacklightLevel(uint8_t level);

  void update();
  bool isPressed(uint8_t buttonIndex) const;
  bool wasPressed(uint8_t buttonIndex) const;
  bool wasAnyPressed() const;
  bool wasReleased(uint8_t buttonIndex) const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  enum KeypadKey : uint8_t {
    KEYPAD_ALT = 0,
    KEYPAD_Q,
    KEYPAD_W,
    KEYPAD_E,
    KEYPAD_R,
    KEYPAD_T,
    KEYPAD_Y,
    KEYPAD_U,
    KEYPAD_I,
    KEYPAD_O,
    KEYPAD_P,
    KEYPAD_A,
    KEYPAD_S,
    KEYPAD_D,
    KEYPAD_F,
    KEYPAD_G,
    KEYPAD_H,
    KEYPAD_J,
    KEYPAD_K,
    KEYPAD_L,
    KEYPAD_DEL,
    KEYPAD_Z,
    KEYPAD_X,
    KEYPAD_C,
    KEYPAD_V,
    KEYPAD_B,
    KEYPAD_N,
    KEYPAD_M,
    KEYPAD_DOLLAR,
    KEYPAD_ENT,
    KEYPAD_SHIFT_LEFT,
    KEYPAD_MIC,
    KEYPAD_SPACE,
    KEYPAD_SYM,
    KEYPAD_SHIFT_RIGHT,
    KEYPAD_KEY_COUNT
  };
  bool isRawKeyPressed(uint8_t keyCode) const;
  bool wasRawKeyPressed(uint8_t keyCode) const;
  bool wasRawKeyReleased(uint8_t keyCode) const;
  unsigned long getRawKeyHeldTime(uint8_t keyCode) const;
  bool isKeypadKeyPressed(KeypadKey key) const;
  bool wasKeypadKeyPressed(KeypadKey key) const;
  bool wasKeypadKeyReleased(KeypadKey key) const;
  unsigned long getKeypadKeyHeldTime(KeypadKey key) const;
  int getLastPressedBindableKey() const;

  void startDeepSleep();
  bool shutdown();
  void verifyPowerButtonWakeup(uint16_t requiredDurationMs, bool shortPressAllowed);

  bool isUsbConnected() const;
  bool wasUsbStateChanged() const;
  bool readChargerStatus(HalChargerStatusSnapshot& snapshot) const;
  bool initFuelGaugeModel();

  enum class WakeupReason { PowerButton, PowerButtonColdBoot, AfterFlash, AfterUSBPower, Other };
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
  bool keypadKeyState[KEYPAD_KEY_COUNT] = {false};
  bool keypadKeyPressedEdge[KEYPAD_KEY_COUNT] = {false};
  bool keypadKeyReleasedEdge[KEYPAD_KEY_COUNT] = {false};
  unsigned long keypadKeyPressedSince[KEYPAD_KEY_COUNT] = {0};
  int lastPressedBindableKey = -1;

  bool lastUsbConnected = false;
  bool usbStateChanged = false;
  bool anyPressed = false;
  bool anyReleased = false;
  unsigned long heldTime = 0;
};

extern HalGPIO gpio;
