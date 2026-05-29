#pragma once

#include <HalPowerManager.h>

#include <array>
#include <vector>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class BatteryStatusActivity final : public Activity {
 public:
  explicit BatteryStatusActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BatteryStatus", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  ButtonNavigator verticalNavigator;
  ButtonNavigator horizontalNavigator;
  HalBatteryStatusSnapshot snapshot = {};
  bool hasSnapshot = false;
  uint8_t selectedPanel = 0;
  std::array<uint8_t, 2> panelScrollOffsets = {0, 0};

  void refreshBattery(bool force = true);
  void clampScrollOffsets();
  size_t getVisibleLineCapacity() const;
  std::vector<std::string> getPanelLines(uint8_t panelIndex) const;
};
