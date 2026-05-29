#pragma once

#include <array>
#include <string>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class KeyboardKeyMappingActivity final : public Activity {
 public:
  explicit KeyboardKeyMappingActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("KeyboardKeyMapping", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr uint8_t kBindingItemCount = 7;
  static constexpr uint8_t kRestoreDefaultsIndex = kBindingItemCount;
  static constexpr uint8_t kSaveAndExitIndex = kBindingItemCount + 1;
  static constexpr uint8_t kMenuItemCount = kBindingItemCount + 2;

  ButtonNavigator buttonNavigator;
  std::array<uint8_t, kBindingItemCount> tempBindings = {};
  uint8_t selectedIndex = 0;
  bool captureMode = false;
  uint8_t captureRoleIndex = 0;
  unsigned long messageUntil = 0;
  std::string transientMessage;

  void loadBindingsFromSettings();
  void resetTempBindingsToDefaults();
  bool saveBindings();
  bool isDuplicateBinding(uint8_t roleIndex, uint8_t binding) const;
  const char* getItemLabel(uint8_t index) const;
  std::string getItemValue(uint8_t index) const;
  std::string getPrimaryMessage() const;
  static uint8_t defaultBindingForRole(uint8_t roleIndex);
};
