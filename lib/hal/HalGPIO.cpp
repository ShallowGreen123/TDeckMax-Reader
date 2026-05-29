#include <HalGPIO.h>

#include <Adafruit_TCA8418.h>
#include <IoExpanderXL9555.hpp>
#include <HynTouch.h>
#include <Logging.h>
#include <SPI.h>
#include <XPowersLib.h>
#include <Wire.h>
#include <esp_sleep.h>

#include <algorithm>

HalGPIO gpio;

namespace {
constexpr char KEYPAD_KEY_NONE = '\0';
constexpr char KEYPAD_KEY_DEL = '\b';
constexpr char KEYPAD_KEY_ENT = 'E';
constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 10;
constexpr uint8_t KEYBOARD_RESET_PULSE_MS = 20;
constexpr uint8_t TOUCH_RESET_PULSE_MS = 20;
constexpr uint8_t TOUCH_RESET_SETTLE_MS = 60;

Adafruit_TCA8418 keypad;
IoExpanderXL9555 xl9555;
XPowersPPM charger;

bool xl9555Ready = false;
bool keypadReady = false;
bool touchReady = false;
bool chargerReady = false;

enum RawBindableKey : uint8_t {
  RAW_KEY_BOOT = 0,
  RAW_KEY_A,
  RAW_KEY_B,
  RAW_KEY_C,
  RAW_KEY_D,
  RAW_KEY_E,
  RAW_KEY_F,
  RAW_KEY_G,
  RAW_KEY_H,
  RAW_KEY_I,
  RAW_KEY_J,
  RAW_KEY_K,
  RAW_KEY_L,
  RAW_KEY_M,
  RAW_KEY_N,
  RAW_KEY_O,
  RAW_KEY_P,
  RAW_KEY_Q,
  RAW_KEY_R,
  RAW_KEY_S,
  RAW_KEY_T,
  RAW_KEY_U,
  RAW_KEY_V,
  RAW_KEY_W,
  RAW_KEY_X,
  RAW_KEY_Y,
  RAW_KEY_Z,
  RAW_KEY_0,
  RAW_KEY_2,
  RAW_KEY_SPACE,
  RAW_KEY_DEL,
  RAW_KEY_ENT,
  RAW_KEY_COUNT,
};
static_assert(RAW_KEY_COUNT == HalGPIO::RAW_BINDABLE_KEY_COUNT, "Raw bindable key count mismatch");

const char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', KEYPAD_KEY_DEL},
    {'2', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '$', KEYPAD_KEY_ENT},
    {KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, 'U', '0', ' ', 'S', 'U'},
};

void deselectSharedSpiDevices() {
  pinMode(BOARD_LORA_CS, OUTPUT);
  digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_LORA_RST, OUTPUT);
  digitalWrite(BOARD_LORA_RST, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT);
  digitalWrite(BOARD_SD_CS, HIGH);
  pinMode(BOARD_EPD_CS, OUTPUT);
  digitalWrite(BOARD_EPD_CS, HIGH);
}

void initExpanderOutputs() {
  if (!xl9555.begin(Wire, XL9555_SLAVE_ADDRESS0, BOARD_I2C_SDA, BOARD_I2C_SCL)) {
    LOG_ERR("GPIO", "XL9555 init failed");
    xl9555Ready = false;
    return;
  }

  xl9555Ready = true;

  const uint8_t lowOutputs[] = {
      BOARD_XL9555_00_6609_EN, BOARD_XL9555_01_LORA_EN, BOARD_XL9555_02_GPS_EN,
      BOARD_XL9555_03_1V8_EN,  BOARD_XL9555_05_MOTOR_EN, BOARD_XL9555_06_AMPLIFIER,
      BOARD_XL9555_10_PWRKEY_EN,
  };
  const uint8_t highOutputs[] = {
      BOARD_XL9555_04_LORA_SEL,
      BOARD_XL9555_07_TOUCH_RST,
      BOARD_XL9555_11_KEY_RST,
      BOARD_XL9555_12_AUDIO_SEL,
  };

  for (uint8_t pin : lowOutputs) {
    xl9555.pinMode(pin, OUTPUT);
    xl9555.digitalWrite(pin, LOW);
  }
  for (uint8_t pin : highOutputs) {
    xl9555.pinMode(pin, OUTPUT);
    xl9555.digitalWrite(pin, HIGH);
  }
}

void resetTouchIfAvailable() {
  if (!xl9555Ready) {
    return;
  }
  xl9555.pinMode(BOARD_XL9555_07_TOUCH_RST, OUTPUT);
  xl9555.digitalWrite(BOARD_XL9555_07_TOUCH_RST, LOW);
  delay(TOUCH_RESET_PULSE_MS);
  xl9555.digitalWrite(BOARD_XL9555_07_TOUCH_RST, HIGH);
  delay(TOUCH_RESET_SETTLE_MS);
}

void resetKeyboardIfAvailable() {
  if (!xl9555Ready) {
    return;
  }
  xl9555.pinMode(BOARD_XL9555_11_KEY_RST, OUTPUT);
  xl9555.digitalWrite(BOARD_XL9555_11_KEY_RST, LOW);
  delay(KEYBOARD_RESET_PULSE_MS);
  xl9555.digitalWrite(BOARD_XL9555_11_KEY_RST, HIGH);
  delay(TOUCH_RESET_SETTLE_MS);
}

void initTouch() {
  resetTouchIfAvailable();
  hyn_touch_attach_xl9555(&xl9555);
  touchReady = hyn_touch_init();
  if (!touchReady) {
    LOG_INF("GPIO", "Touch init failed");
  }
}

void initKeyboard() {
  resetKeyboardIfAvailable();
  pinMode(BOARD_KEYBOARD_INT, INPUT_PULLUP);
  keypadReady = keypad.begin(BOARD_I2C_ADDR_KEYBOARD, &Wire);
  if (!keypadReady) {
    LOG_ERR("GPIO", "Keyboard init failed");
    return;
  }
  keypad.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  keypad.flush();
}

void initCharger() {
  chargerReady = charger.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, SY6970_SLAVE_ADDRESS);
  if (!chargerReady) {
    LOG_INF("GPIO", "SY6970 init failed");
    return;
  }
  charger.disableADCMeasure();
}

uint8_t mapKeyToButton(const char key) {
  switch (key) {
    case KEYPAD_KEY_DEL:
      return HalGPIO::BTN_BACK;
    case KEYPAD_KEY_ENT:
      return HalGPIO::BTN_CONFIRM;
    case 'a':
    case 'A':
      return HalGPIO::BTN_LEFT;
    case 'd':
    case 'D':
      return HalGPIO::BTN_RIGHT;
    case 'w':
    case 'W':
      return HalGPIO::BTN_UP;
    case 's':
    case 'S':
      return HalGPIO::BTN_DOWN;
    default:
      return HalGPIO::BUTTON_COUNT;
  }
}

uint8_t mapKeyToRawBindableKey(const char key) {
  if (key == KEYPAD_KEY_DEL) {
    return RAW_KEY_DEL;
  }
  if (key == KEYPAD_KEY_ENT) {
    return RAW_KEY_ENT;
  }
  if (key == ' ') {
    return RAW_KEY_SPACE;
  }
  if (key == '0') {
    return RAW_KEY_0;
  }
  if (key == '2') {
    return RAW_KEY_2;
  }
  if (key >= 'a' && key <= 'z') {
    return static_cast<uint8_t>(RAW_KEY_A + (key - 'a'));
  }
  if (key >= 'A' && key <= 'Z') {
    return static_cast<uint8_t>(RAW_KEY_A + (key - 'A'));
  }
  return HalGPIO::RAW_BINDABLE_KEY_COUNT;
}

uint8_t mapRawBindableKeyToButton(const uint8_t rawKey) {
  switch (rawKey) {
    case RAW_KEY_DEL:
      return HalGPIO::BTN_BACK;
    case RAW_KEY_ENT:
      return HalGPIO::BTN_CONFIRM;
    case RAW_KEY_A:
      return HalGPIO::BTN_LEFT;
    case RAW_KEY_D:
      return HalGPIO::BTN_RIGHT;
    case RAW_KEY_W:
      return HalGPIO::BTN_UP;
    case RAW_KEY_S:
      return HalGPIO::BTN_DOWN;
    default:
      return HalGPIO::BUTTON_COUNT;
  }
}

bool decodeKeypadEvent(const int event, uint8_t& outRawKey, uint8_t& outButton, bool& outPressed) {
  if ((event & 0x7F) == 0) {
    return false;
  }

  const int keyIndex = (event & 0x7F) - 1;
  const int row = keyIndex / KEYPAD_COLS;
  const int col = (KEYPAD_COLS - 1) - (keyIndex % KEYPAD_COLS);
  if (row < 0 || row >= KEYPAD_ROWS || col < 0 || col >= KEYPAD_COLS) {
    return false;
  }

  outPressed = (event & 0x80) != 0;
  outRawKey = mapKeyToRawBindableKey(keymap[row][col]);
  outButton = mapRawBindableKeyToButton(outRawKey);
  return outRawKey < HalGPIO::RAW_BINDABLE_KEY_COUNT;
}
}  // namespace

void HalGPIO::begin() {
  pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
  pinMode(BOARD_EPD_BL, OUTPUT);
  analogWrite(BOARD_EPD_BL, 0);
  pinMode(BOARD_KEYBOARD_LED, OUTPUT);
  analogWrite(BOARD_KEYBOARD_LED, 0);

  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
  deselectSharedSpiDevices();

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL, I2C_FREQUENCY);
  Wire.setTimeOut(6);

  initExpanderOutputs();
  initTouch();
  initKeyboard();
  initCharger();

  lastUsbConnected = isUsbConnected();
}

void HalGPIO::update() {
  bool nextState[BUTTON_COUNT];
  bool nextRawState[RAW_BINDABLE_KEY_COUNT];
  std::copy(std::begin(buttonState), std::end(buttonState), std::begin(nextState));
  std::copy(std::begin(rawKeyState), std::end(rawKeyState), std::begin(nextRawState));

  std::fill(std::begin(buttonPressedEdge), std::end(buttonPressedEdge), false);
  std::fill(std::begin(buttonReleasedEdge), std::end(buttonReleasedEdge), false);
  std::fill(std::begin(rawKeyPressedEdge), std::end(rawKeyPressedEdge), false);
  std::fill(std::begin(rawKeyReleasedEdge), std::end(rawKeyReleasedEdge), false);
  anyPressed = false;
  anyReleased = false;
  lastPressedBindableKey = -1;

  if (keypadReady) {
    while (keypad.available() > 0) {
      uint8_t rawKey = RAW_KEY_COUNT;
      uint8_t button = BUTTON_COUNT;
      bool pressed = false;
      if (!decodeKeypadEvent(keypad.getEvent(), rawKey, button, pressed)) {
        continue;
      }
      nextRawState[rawKey] = pressed;
      nextState[button] = pressed;
    }
  }

  nextState[BTN_POWER] = digitalRead(BOARD_BOOT_PIN) == LOW;
  nextRawState[RAW_KEY_BOOT] = nextState[BTN_POWER];

  const unsigned long now = millis();
  heldTime = 0;
  for (uint8_t i = 0; i < RAW_BINDABLE_KEY_COUNT; i++) {
    if (nextRawState[i] != rawKeyState[i]) {
      if (nextRawState[i]) {
        rawKeyPressedEdge[i] = true;
        rawKeyPressedSince[i] = now;
        anyPressed = true;
        lastPressedBindableKey = i;
      } else {
        rawKeyReleasedEdge[i] = true;
        anyReleased = true;
      }
      rawKeyState[i] = nextRawState[i];
    }
  }

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (nextState[i] != buttonState[i]) {
      if (nextState[i]) {
        buttonPressedEdge[i] = true;
        buttonPressedSince[i] = now;
        anyPressed = true;
      } else {
        buttonReleasedEdge[i] = true;
        anyReleased = true;
      }
      buttonState[i] = nextState[i];
    }

    if (buttonState[i]) {
      heldTime = std::max(heldTime, now - buttonPressedSince[i]);
    }
  }

  const bool connected = isUsbConnected();
  usbStateChanged = connected != lastUsbConnected;
  lastUsbConnected = connected;
}

bool HalGPIO::isPressed(const uint8_t buttonIndex) const {
  return buttonIndex < BUTTON_COUNT ? buttonState[buttonIndex] : false;
}

bool HalGPIO::wasPressed(const uint8_t buttonIndex) const {
  return buttonIndex < BUTTON_COUNT ? buttonPressedEdge[buttonIndex] : false;
}

bool HalGPIO::wasAnyPressed() const { return anyPressed; }

bool HalGPIO::wasReleased(const uint8_t buttonIndex) const {
  return buttonIndex < BUTTON_COUNT ? buttonReleasedEdge[buttonIndex] : false;
}

bool HalGPIO::wasAnyReleased() const { return anyReleased; }

unsigned long HalGPIO::getHeldTime() const { return heldTime; }

bool HalGPIO::isRawKeyPressed(const uint8_t keyCode) const {
  return keyCode < RAW_BINDABLE_KEY_COUNT ? rawKeyState[keyCode] : false;
}

bool HalGPIO::wasRawKeyPressed(const uint8_t keyCode) const {
  return keyCode < RAW_BINDABLE_KEY_COUNT ? rawKeyPressedEdge[keyCode] : false;
}

bool HalGPIO::wasRawKeyReleased(const uint8_t keyCode) const {
  return keyCode < RAW_BINDABLE_KEY_COUNT ? rawKeyReleasedEdge[keyCode] : false;
}

unsigned long HalGPIO::getRawKeyHeldTime(const uint8_t keyCode) const {
  if (keyCode >= RAW_BINDABLE_KEY_COUNT || !rawKeyState[keyCode]) {
    return 0;
  }
  return millis() - rawKeyPressedSince[keyCode];
}

int HalGPIO::getLastPressedBindableKey() const { return lastPressedBindableKey; }

void HalGPIO::startDeepSleep() {
  while (isPressed(BTN_POWER)) {
    delay(20);
    update();
  }

  analogWrite(BOARD_KEYBOARD_LED, 0);
  analogWrite(BOARD_EPD_BL, 0);

  if (xl9555Ready) {
    xl9555.digitalWrite(BOARD_XL9555_06_AMPLIFIER, LOW);
    xl9555.digitalWrite(BOARD_XL9555_05_MOTOR_EN, LOW);
    xl9555.digitalWrite(BOARD_XL9555_01_LORA_EN, LOW);
    xl9555.digitalWrite(BOARD_XL9555_02_GPS_EN, LOW);
    xl9555.digitalWrite(BOARD_XL9555_03_1V8_EN, LOW);
  }

  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BOARD_BOOT_PIN), 0);
  esp_deep_sleep_start();
}

void HalGPIO::verifyPowerButtonWakeup(const uint16_t requiredDurationMs, const bool shortPressAllowed) {
  if (shortPressAllowed) {
    return;
  }

  const uint16_t elapsedBeforeCheck = millis();
  const uint16_t calibratedDuration =
      elapsedBeforeCheck < requiredDurationMs ? requiredDurationMs - elapsedBeforeCheck : 1;

  const unsigned long start = millis();
  update();
  while (!isPressed(BTN_POWER) && millis() - start < 1000) {
    delay(10);
    update();
  }

  if (!isPressed(BTN_POWER)) {
    startDeepSleep();
  }

  do {
    delay(10);
    update();
  } while (isPressed(BTN_POWER) && getHeldTime() < calibratedDuration);

  if (getHeldTime() < calibratedDuration) {
    startDeepSleep();
  }
}

bool HalGPIO::isUsbConnected() const { return chargerReady ? charger.isVbusIn() : false; }

bool HalGPIO::wasUsbStateChanged() const { return usbStateChanged; }

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  const auto wakeupCause = esp_sleep_get_wakeup_cause();
  const auto resetReason = esp_reset_reason();

  if (wakeupCause == ESP_SLEEP_WAKEUP_EXT0 && resetReason == ESP_RST_DEEPSLEEP) {
    return WakeupReason::PowerButton;
  }

  if (resetReason == ESP_RST_POWERON) {
    return isUsbConnected() ? WakeupReason::AfterUSBPower : WakeupReason::PowerButton;
  }

  if (resetReason == ESP_RST_SW || resetReason == ESP_RST_UNKNOWN) {
    return WakeupReason::AfterFlash;
  }

  return WakeupReason::Other;
}
