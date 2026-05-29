#include "KeyboardKeyMappingActivity.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <I18n.h>

#include <cstdio>
#include <utility>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr unsigned long kTransientMessageMs = 1500;
constexpr unsigned long kCaptureCancelHoldMs = 700;

std::string bindingName(const uint8_t binding) {
  using KeyBinding = CrossPointSettings::KEY_BINDING;

  if (binding >= KeyBinding::KEY_BIND_A && binding <= KeyBinding::KEY_BIND_Z) {
    std::string label(1, static_cast<char>('A' + (binding - KeyBinding::KEY_BIND_A)));
    return label;
  }

  switch (binding) {
    case KeyBinding::KEY_BIND_BOOT:
      return "BOOT";
    case KeyBinding::KEY_BIND_0:
      return "0";
    case KeyBinding::KEY_BIND_2:
      return "2";
    case KeyBinding::KEY_BIND_SPACE:
      return "SPACE";
    case KeyBinding::KEY_BIND_DEL:
      return "DEL";
    case KeyBinding::KEY_BIND_ENT:
      return "ENT";
    default:
      return "?";
  }
}
}  // namespace

void KeyboardKeyMappingActivity::onEnter() {
  Activity::onEnter();

  selectedIndex = 0;
  captureMode = false;
  captureRoleIndex = 0;
  transientMessage.clear();
  messageUntil = 0;
  loadBindingsFromSettings();
  requestUpdate();
}

void KeyboardKeyMappingActivity::onExit() { Activity::onExit(); }

void KeyboardKeyMappingActivity::loop() {
  if (messageUntil > 0 && millis() > messageUntil) {
    transientMessage.clear();
    messageUntil = 0;
    requestUpdate();
  }

  if (captureMode) {
    const uint8_t cancelBinding = tempBindings[0];
    if (gpio.isRawKeyPressed(cancelBinding) && gpio.getRawKeyHeldTime(cancelBinding) >= kCaptureCancelHoldMs) {
      captureMode = false;
      transientMessage.clear();
      messageUntil = 0;
      requestUpdate();
      return;
    }

    const int pressedKey = gpio.getLastPressedBindableKey();
    if (pressedKey < 0) {
      return;
    }

    const uint8_t binding = static_cast<uint8_t>(pressedKey);
    if (binding == cancelBinding) {
      return;
    }

    if (binding == CrossPointSettings::KEY_BIND_BOOT && captureRoleIndex != 6) {
      transientMessage = tr(STR_BOOT_ONLY_FOR_POWER);
      messageUntil = millis() + kTransientMessageMs;
      requestUpdate();
      return;
    }

    if (isDuplicateBinding(captureRoleIndex, binding)) {
      transientMessage = tr(STR_DUPLICATE_KEY);
      messageUntil = millis() + kTransientMessageMs;
      requestUpdate();
      return;
    }

    tempBindings[captureRoleIndex] = binding;
    captureMode = false;
    transientMessage.clear();
    messageUntil = 0;
    requestUpdate();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    if (selectedIndex < kBindingItemCount) {
      captureMode = true;
      captureRoleIndex = selectedIndex;
      transientMessage.clear();
      messageUntil = 0;
      requestUpdate();
      return;
    }

    if (selectedIndex == kRestoreDefaultsIndex) {
      resetTempBindingsToDefaults();
      transientMessage.clear();
      messageUntil = 0;
      requestUpdate();
      return;
    }

    if (saveBindings()) {
      finish();
      return;
    }

    requestUpdate();
    return;
  }

  buttonNavigator.onNextRelease([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, kMenuItemCount);
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, kMenuItemCount);
    requestUpdate();
  });

  buttonNavigator.onNextContinuous([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, kMenuItemCount);
    requestUpdate();
  });

  buttonNavigator.onPreviousContinuous([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, kMenuItemCount);
    requestUpdate();
  });
}

void KeyboardKeyMappingActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int helpLineHeight = renderer.getLineHeight(SMALL_FONT_ID);
  const int helpReservedHeight = helpLineHeight * 2 + metrics.verticalSpacing * 2;
  const int headerY = metrics.topPadding;
  const int subHeaderY = headerY + metrics.headerHeight;
  const int listTop = subHeaderY + metrics.tabBarHeight + metrics.verticalSpacing;
  const int listHeight =
      pageHeight - listTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2 - helpReservedHeight;

  GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, metrics.headerHeight}, tr(STR_KEYBOARD_MAPPING));
  GUI.drawSubHeader(renderer, Rect{0, subHeaderY, pageWidth, metrics.tabBarHeight}, getPrimaryMessage().c_str());

  GUI.drawList(
      renderer, Rect{0, listTop, pageWidth, listHeight}, kMenuItemCount, selectedIndex,
      [this](int index) { return std::string(getItemLabel(static_cast<uint8_t>(index))); }, nullptr, nullptr,
      [this](int index) { return getItemValue(static_cast<uint8_t>(index)); }, true);

  const int helpY = listTop + listHeight + metrics.verticalSpacing;
  if (captureMode) {
    GUI.drawHelpText(renderer, Rect{0, helpY + helpLineHeight + metrics.verticalSpacing, pageWidth, helpLineHeight},
                     tr(STR_HOLD_BACK_TO_CANCEL));
  }

  const auto labels = mappedInput.mapLabels(tr(STR_EXIT), captureMode ? "" : tr(STR_SELECT), tr(STR_DIR_UP),
                                            tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

void KeyboardKeyMappingActivity::loadBindingsFromSettings() {
  tempBindings[0] = SETTINGS.keyBindingBack;
  tempBindings[1] = SETTINGS.keyBindingConfirm;
  tempBindings[2] = SETTINGS.keyBindingLeft;
  tempBindings[3] = SETTINGS.keyBindingRight;
  tempBindings[4] = SETTINGS.keyBindingUp;
  tempBindings[5] = SETTINGS.keyBindingDown;
  tempBindings[6] = SETTINGS.keyBindingPower;
}

void KeyboardKeyMappingActivity::resetTempBindingsToDefaults() {
  for (uint8_t i = 0; i < kBindingItemCount; ++i) {
    tempBindings[i] = defaultBindingForRole(i);
  }
}

bool KeyboardKeyMappingActivity::saveBindings() {
  const auto previousBindings = std::array<uint8_t, kBindingItemCount>{
      SETTINGS.keyBindingBack, SETTINGS.keyBindingConfirm, SETTINGS.keyBindingLeft,  SETTINGS.keyBindingRight,
      SETTINGS.keyBindingUp,   SETTINGS.keyBindingDown,    SETTINGS.keyBindingPower,
  };

  SETTINGS.keyBindingBack = tempBindings[0];
  SETTINGS.keyBindingConfirm = tempBindings[1];
  SETTINGS.keyBindingLeft = tempBindings[2];
  SETTINGS.keyBindingRight = tempBindings[3];
  SETTINGS.keyBindingUp = tempBindings[4];
  SETTINGS.keyBindingDown = tempBindings[5];
  SETTINGS.keyBindingPower = tempBindings[6];
  CrossPointSettings::validateKeyBindings(SETTINGS);

  if (SETTINGS.saveToFile()) {
    return true;
  }

  SETTINGS.keyBindingBack = previousBindings[0];
  SETTINGS.keyBindingConfirm = previousBindings[1];
  SETTINGS.keyBindingLeft = previousBindings[2];
  SETTINGS.keyBindingRight = previousBindings[3];
  SETTINGS.keyBindingUp = previousBindings[4];
  SETTINGS.keyBindingDown = previousBindings[5];
  SETTINGS.keyBindingPower = previousBindings[6];
  transientMessage = tr(STR_ERROR_GENERAL_FAILURE);
  messageUntil = millis() + kTransientMessageMs;
  return false;
}

bool KeyboardKeyMappingActivity::isDuplicateBinding(const uint8_t roleIndex, const uint8_t binding) const {
  for (uint8_t i = 0; i < kBindingItemCount; ++i) {
    if (i != roleIndex && tempBindings[i] == binding) {
      return true;
    }
  }
  return false;
}

const char* KeyboardKeyMappingActivity::getItemLabel(const uint8_t index) const {
  switch (index) {
    case 0:
      return tr(STR_BACK_BUTTON_LABEL);
    case 1:
      return tr(STR_CONFIRM);
    case 2:
      return tr(STR_DIR_LEFT);
    case 3:
      return tr(STR_DIR_RIGHT);
    case 4:
      return tr(STR_DIR_UP);
    case 5:
      return tr(STR_DIR_DOWN);
    case 6:
      return tr(STR_POWER_BUTTON_LABEL);
    case kRestoreDefaultsIndex:
      return tr(STR_RESTORE_DEFAULTS);
    case kSaveAndExitIndex:
    default:
      return tr(STR_SAVE_AND_EXIT);
  }
}

std::string KeyboardKeyMappingActivity::getItemValue(const uint8_t index) const {
  if (index >= kBindingItemCount) {
    return "";
  }

  return bindingName(tempBindings[index]);
}

std::string KeyboardKeyMappingActivity::getPrimaryMessage() const {
  if (!transientMessage.empty()) {
    return transientMessage;
  }

  if (!captureMode) {
    return "";
  }

  char buffer[80];
  snprintf(buffer, sizeof(buffer), tr(STR_PRESS_KEY_FOR), getItemLabel(captureRoleIndex));
  return buffer;
}

uint8_t KeyboardKeyMappingActivity::defaultBindingForRole(const uint8_t roleIndex) {
  switch (roleIndex) {
    case 0:
      return CrossPointSettings::KEY_BIND_DEL;
    case 1:
      return CrossPointSettings::KEY_BIND_ENT;
    case 2:
      return CrossPointSettings::KEY_BIND_A;
    case 3:
      return CrossPointSettings::KEY_BIND_D;
    case 4:
      return CrossPointSettings::KEY_BIND_W;
    case 5:
      return CrossPointSettings::KEY_BIND_S;
    case 6:
    default:
      return CrossPointSettings::KEY_BIND_BOOT;
  }
}
