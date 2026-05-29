#include "MappedInputManager.h"

#include <algorithm>

#include "CrossPointSettings.h"

namespace {
using ButtonIndex = uint8_t;

static_assert(HalGPIO::RAW_BINDABLE_KEY_COUNT == CrossPointSettings::KEY_BINDING_COUNT, "Key binding count mismatch");

uint8_t getBindingForButton(const MappedInputManager::Button button) {
  const auto sideLayout = static_cast<CrossPointSettings::SIDE_BUTTON_LAYOUT>(SETTINGS.sideButtonLayout);

  switch (button) {
    case MappedInputManager::Button::Back:
      return SETTINGS.keyBindingBack;
    case MappedInputManager::Button::Confirm:
      return SETTINGS.keyBindingConfirm;
    case MappedInputManager::Button::Left:
      return SETTINGS.keyBindingLeft;
    case MappedInputManager::Button::Right:
      return SETTINGS.keyBindingRight;
    case MappedInputManager::Button::Up:
      return SETTINGS.keyBindingUp;
    case MappedInputManager::Button::Down:
      return SETTINGS.keyBindingDown;
    case MappedInputManager::Button::Power:
      return SETTINGS.keyBindingPower;
    case MappedInputManager::Button::PageBack:
      return sideLayout == CrossPointSettings::SIDE_BUTTON_LAYOUT::PREV_NEXT ? SETTINGS.keyBindingUp
                                                                             : SETTINGS.keyBindingDown;
    case MappedInputManager::Button::PageForward:
    default:
      return sideLayout == CrossPointSettings::SIDE_BUTTON_LAYOUT::PREV_NEXT ? SETTINGS.keyBindingDown
                                                                             : SETTINGS.keyBindingUp;
  }
}
}  // namespace

bool MappedInputManager::mapButton(const Button button, bool (HalGPIO::*fn)(uint8_t) const) const {
  return (gpio.*fn)(getBindingForButton(button));
}

bool MappedInputManager::wasPressed(const Button button) const { return mapButton(button, &HalGPIO::wasRawKeyPressed); }

bool MappedInputManager::wasReleased(const Button button) const {
  return mapButton(button, &HalGPIO::wasRawKeyReleased);
}

bool MappedInputManager::isPressed(const Button button) const { return mapButton(button, &HalGPIO::isRawKeyPressed); }

bool MappedInputManager::wasAnyPressed() const { return gpio.wasAnyPressed(); }

bool MappedInputManager::wasAnyReleased() const { return gpio.wasAnyReleased(); }

unsigned long MappedInputManager::getHeldTime() const {
  unsigned long heldTime = 0;
  constexpr Button buttons[] = {Button::Back,  Button::Confirm, Button::Left,     Button::Right, Button::Up,
                                Button::Down, Button::Power,   Button::PageBack, Button::PageForward};
  for (const Button button : buttons) {
    const uint8_t binding = getBindingForButton(button);
    heldTime = std::max(heldTime, gpio.getRawKeyHeldTime(binding));
  }
  return heldTime;
}

MappedInputManager::Labels MappedInputManager::mapLabels(const char* back, const char* confirm, const char* previous,
                                                         const char* next) const {
  // Build the label order based on the configured hardware mapping.
  auto labelForHardware = [&](uint8_t hw) -> const char* {
    // Compare against configured logical roles and return the matching label.
    if (hw == SETTINGS.frontButtonBack) {
      return back;
    }
    if (hw == SETTINGS.frontButtonConfirm) {
      return confirm;
    }
    if (hw == SETTINGS.frontButtonLeft) {
      return previous;
    }
    if (hw == SETTINGS.frontButtonRight) {
      return next;
    }
    return "";
  };

  return {labelForHardware(HalGPIO::BTN_BACK), labelForHardware(HalGPIO::BTN_CONFIRM),
          labelForHardware(HalGPIO::BTN_LEFT), labelForHardware(HalGPIO::BTN_RIGHT)};
}

int MappedInputManager::getPressedFrontButton() const {
  // Scan the raw front buttons in hardware order.
  // This bypasses remapping so the remap activity can capture physical presses.
  if (gpio.wasPressed(HalGPIO::BTN_BACK)) {
    return HalGPIO::BTN_BACK;
  }
  if (gpio.wasPressed(HalGPIO::BTN_CONFIRM)) {
    return HalGPIO::BTN_CONFIRM;
  }
  if (gpio.wasPressed(HalGPIO::BTN_LEFT)) {
    return HalGPIO::BTN_LEFT;
  }
  if (gpio.wasPressed(HalGPIO::BTN_RIGHT)) {
    return HalGPIO::BTN_RIGHT;
  }
  return -1;
}
