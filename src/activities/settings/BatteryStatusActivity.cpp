#include "BatteryStatusActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kCardRadius = 8;
constexpr int kCardPadding = 6;

const char* primaryModeText(const HalBatteryPrimaryMode mode) {
  switch (mode) {
    case HalBatteryPrimaryMode::Full:
      return "Full";
    case HalBatteryPrimaryMode::Charging:
      return "Charging";
    case HalBatteryPrimaryMode::StandbyUsb:
      return "Standby / USB IN";
    case HalBatteryPrimaryMode::Discharge:
      return "Discharge";
    case HalBatteryPrimaryMode::Unavailable:
    default:
      return "Unavailable";
  }
}

const char* gaugeStateText(const HalGaugeState state) {
  switch (state) {
    case HalGaugeState::Sleep:
      return "Sleep";
    case HalGaugeState::Full:
      return "Full";
    case HalGaugeState::Charge:
      return "Charge";
    case HalGaugeState::Discharge:
      return "Discharge";
    case HalGaugeState::Relax:
      return "Relax";
    case HalGaugeState::Unknown:
    default:
      return "Unknown";
  }
}

const char* chargerBusTypeText(const uint8_t busType) {
  switch (busType) {
    case 0:
      return "No input";
    case 1:
      return "USB SDP";
    case 2:
      return "USB CDP";
    case 3:
      return "USB DCP";
    case 4:
      return "HVDCP";
    case 5:
      return "Adapter";
    case 6:
      return "Non-std";
    case 7:
      return "OTG";
    default:
      return "Unknown";
  }
}

const char* chargerStateText(const uint8_t chargeState) {
  switch (chargeState) {
    case 0:
      return "Idle";
    case 1:
      return "Pre-charge";
    case 2:
      return "Fast charge";
    case 3:
      return "Done";
    default:
      return "Unknown";
  }
}

std::string formatTemperature(const uint16_t deciKelvin) {
  if (deciKelvin == 0) {
    return "--";
  }

  const int deciCelsius = static_cast<int>(deciKelvin) - 2731;
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "%d.%d C", deciCelsius / 10, std::abs(deciCelsius % 10));
  return buffer;
}

std::string lineText(const char* label, const std::string& value) {
  std::string line(label);
  line += ": ";
  line += value;
  return line;
}

std::string lineText(const char* label, const char* value) { return lineText(label, std::string(value)); }

std::string lineText(const char* label, const int value, const char* unit) {
  char buffer[40];
  std::snprintf(buffer, sizeof(buffer), "%d %s", value, unit);
  return lineText(label, buffer);
}

std::string lineText(const char* label, const unsigned value, const char* unit) {
  char buffer[40];
  std::snprintf(buffer, sizeof(buffer), "%u %s", value, unit);
  return lineText(label, buffer);
}

std::string lineText(const char* label, const uint16_t value, const char* unit) {
  return lineText(label, static_cast<unsigned>(value), unit);
}

std::string summaryText(const HalBatteryStatusSnapshot& snapshot) {
  if (!snapshot.hasData) {
    return "Battery management unavailable";
  }

  const bool usbConnected = snapshot.charger.readOk ? snapshot.charger.vbusConnected : gpio.isUsbConnected();
  const char* powerPath = usbConnected ? "USB IN" : "Battery";

  char buffer[128];
  if (snapshot.gauge.readOk) {
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%s | %u%% | %umV | Avg %dmA",
                  powerPath,
                  snapshot.gauge.socPercent,
                  snapshot.gauge.voltageMv,
                  static_cast<int>(snapshot.gauge.averageCurrentMa));
    return buffer;
  }

  if (snapshot.charger.readOk) {
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%s | VBAT %umV | VBUS %umV | %s",
                  powerPath,
                  snapshot.charger.batteryVoltageMv,
                  snapshot.charger.vbusVoltageMv,
                  chargerStateText(snapshot.charger.chargeState));
    return buffer;
  }

  return "Gauge read error / Charger read error";
}

std::vector<std::string> gaugeLines(const HalBatteryStatusSnapshot& snapshot) {
  const auto& gauge = snapshot.gauge;
  if (!gauge.gaugeReady) {
    return {lineText("State", "Not found"),
            lineText("SOC", "--"),
            lineText("SOH", "--"),
            lineText("Volt", "--"),
            lineText("Current", "--"),
            lineText("Avg I", "--"),
            lineText("Temp", "--"),
            lineText("Remain", "--"),
            lineText("Full", "--"),
            lineText("Design", "--"),
            lineText("Status B", "--"),
            lineText("Status G", "--")};
  }

  if (!gauge.readOk) {
    return {lineText("State", "Read error"),
            lineText("SOC", "--"),
            lineText("SOH", "--"),
            lineText("Volt", "--"),
            lineText("Current", "--"),
            lineText("Avg I", "--"),
            lineText("Temp", "--"),
            lineText("Remain", "--"),
            lineText("Full", "--"),
            lineText("Design", "--"),
            lineText("Status B", "--"),
            lineText("Status G", "--")};
  }

  char batteryStatus[16];
  char gaugingStatus[16];
  std::snprintf(batteryStatus, sizeof(batteryStatus), "%04X", gauge.batteryStatusRaw);
  std::snprintf(gaugingStatus, sizeof(gaugingStatus), "%04X", gauge.gaugingStatusRaw);

  return {lineText("State", gaugeStateText(gauge.state)),
          lineText("SOC", std::to_string(gauge.socPercent) + "%"),
          lineText("SOH", std::to_string(gauge.sohPercent) + "%"),
          lineText("Volt", gauge.voltageMv, "mV"),
          lineText("Current", gauge.currentMa, "mA"),
          lineText("Avg I", gauge.averageCurrentMa, "mA"),
          lineText("Temp", formatTemperature(gauge.temperatureDk)),
          lineText("Remain", gauge.remainingCapacityMah, "mAh"),
          lineText("Full", gauge.fullCapacityMah, "mAh"),
          lineText("Design", gauge.designCapacityMah, "mAh"),
          lineText("Status B", batteryStatus),
          lineText("Status G", gaugingStatus)};
}

std::vector<std::string> chargerLines(const HalBatteryStatusSnapshot& snapshot) {
  const auto& charger = snapshot.charger;
  if (!charger.chargerReady) {
    return {lineText("VBUS", "Not found"),
            lineText("Bus", "--"),
            lineText("Charge", "--"),
            lineText("VBUS mV", "--"),
            lineText("VSYS", "--"),
            lineText("VBAT", "--"),
            lineText("Charge ADC", "--"),
            lineText("Input Lim", "--"),
            lineText("Target V", "--"),
            lineText("Target I", "--")};
  }

  if (!charger.readOk) {
    return {lineText("VBUS", "Read error"),
            lineText("Bus", "--"),
            lineText("Charge", "--"),
            lineText("VBUS mV", "--"),
            lineText("VSYS", "--"),
            lineText("VBAT", "--"),
            lineText("Charge ADC", "--"),
            lineText("Input Lim", "--"),
            lineText("Target V", "--"),
            lineText("Target I", "--")};
  }

  return {lineText("VBUS", charger.vbusConnected ? "IN" : "OUT"),
          lineText("Bus", chargerBusTypeText(charger.busType)),
          lineText("Charge", chargerStateText(charger.chargeState)),
          lineText("VBUS mV", charger.vbusVoltageMv, "mV"),
          lineText("VSYS", charger.systemVoltageMv, "mV"),
          lineText("VBAT", charger.batteryVoltageMv, "mV"),
          lineText("Charge ADC", charger.chargeCurrentAdcMa, "mA"),
          lineText("Input Lim", charger.inputLimitMa, "mA"),
          lineText("Target V", charger.targetVoltageMv, "mV"),
          lineText("Target I", charger.targetCurrentMa, "mA")};
}

void drawPanel(const GfxRenderer& renderer, const Rect rect, const char* title, const std::vector<std::string>& lines,
               const size_t scrollOffset, const size_t visibleLineCount) {
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCardRadius, true);
  renderer.drawText(SMALL_FONT_ID, rect.x + kCardPadding, rect.y + kCardPadding, title, true, EpdFontFamily::BOLD);

  const int dividerY = rect.y + kCardPadding + renderer.getLineHeight(SMALL_FONT_ID) + 2;
  renderer.drawLine(rect.x + kCardPadding, dividerY, rect.x + rect.width - kCardPadding, dividerY);

  const int lineHeight = renderer.getLineHeight(SMALL_FONT_ID) + 1;
  const int maxTextWidth = rect.width - kCardPadding * 2;
  int y = dividerY + 4;
  const size_t endIndex = std::min(lines.size(), scrollOffset + visibleLineCount);
  for (size_t i = scrollOffset; i < endIndex; ++i) {
    if (y + lineHeight > rect.y + rect.height - kCardPadding) {
      break;
    }
    const auto text = renderer.truncatedText(SMALL_FONT_ID, lines[i].c_str(), maxTextWidth);
    renderer.drawText(SMALL_FONT_ID, rect.x + kCardPadding, y, text.c_str());
    y += lineHeight;
  }

  if (scrollOffset > 0) {
    renderer.drawText(SMALL_FONT_ID, rect.x + rect.width - 10, rect.y + kCardPadding, "^");
  }
  if (endIndex < lines.size()) {
    renderer.drawText(SMALL_FONT_ID, rect.x + rect.width - 10, rect.y + rect.height - renderer.getLineHeight(SMALL_FONT_ID) - 2,
                      "v");
  }
}
}  // namespace

void BatteryStatusActivity::onEnter() {
  Activity::onEnter();
  selectedPanel = 0;
  panelScrollOffsets = {0, 0};
  refreshBattery(true);
  requestUpdate();
}

void BatteryStatusActivity::onExit() { Activity::onExit(); }

void BatteryStatusActivity::refreshBattery(const bool force) {
  hasSnapshot = powerManager.readBatteryStatus(snapshot, force);
  clampScrollOffsets();
}

void BatteryStatusActivity::clampScrollOffsets() {
  const size_t visibleLineCount = getVisibleLineCapacity();
  for (uint8_t panel = 0; panel < panelScrollOffsets.size(); ++panel) {
    const auto lines = getPanelLines(panel);
    const size_t maxOffset = lines.size() > visibleLineCount ? lines.size() - visibleLineCount : 0;
    if (panelScrollOffsets[panel] > maxOffset) {
      panelScrollOffsets[panel] = static_cast<uint8_t>(maxOffset);
    }
  }
}

size_t BatteryStatusActivity::getVisibleLineCapacity() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageHeight = renderer.getScreenHeight();
  const int helpLineHeight = renderer.getLineHeight(SMALL_FONT_ID);
  const int helpReservedHeight = helpLineHeight + metrics.verticalSpacing;
  const int summaryTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int panelTop = summaryTop + renderer.getLineHeight(SMALL_FONT_ID) + metrics.verticalSpacing + 2;
  const int panelHeight = pageHeight - panelTop - helpReservedHeight - metrics.verticalSpacing;
  const int dividerY = kCardPadding + renderer.getLineHeight(SMALL_FONT_ID) + 2;
  const int contentHeight = panelHeight - dividerY - kCardPadding - 6;
  const int lineHeight = renderer.getLineHeight(SMALL_FONT_ID) + 1;
  return std::max(1, contentHeight / std::max(1, lineHeight));
}

std::vector<std::string> BatteryStatusActivity::getPanelLines(const uint8_t panelIndex) const {
  return panelIndex == 0 ? gaugeLines(snapshot) : chargerLines(snapshot);
}

void BatteryStatusActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    refreshBattery(true);
    requestUpdate();
    return;
  }

  horizontalNavigator.onPress({MappedInputManager::Button::Right}, [this] {
    selectedPanel = static_cast<uint8_t>(ButtonNavigator::nextIndex(selectedPanel, 2));
    requestUpdate();
  });

  horizontalNavigator.onPress({MappedInputManager::Button::Left}, [this] {
    selectedPanel = static_cast<uint8_t>(ButtonNavigator::previousIndex(selectedPanel, 2));
    requestUpdate();
  });

  verticalNavigator.onPressAndContinuous({MappedInputManager::Button::Down}, [this] {
    const auto lines = getPanelLines(selectedPanel);
    const size_t visibleLineCount = getVisibleLineCapacity();
    const size_t maxOffset = lines.size() > visibleLineCount ? lines.size() - visibleLineCount : 0;
    if (panelScrollOffsets[selectedPanel] < maxOffset) {
      panelScrollOffsets[selectedPanel]++;
      requestUpdate();
    }
  });

  verticalNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this] {
    if (panelScrollOffsets[selectedPanel] > 0) {
      panelScrollOffsets[selectedPanel]--;
      requestUpdate();
    }
  });
}

void BatteryStatusActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int sidePadding = std::max(4, metrics.contentSidePadding - 4);
  const int helpLineHeight = renderer.getLineHeight(SMALL_FONT_ID);
  const int helpReservedHeight = helpLineHeight + metrics.verticalSpacing;

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BATTERY_STATUS));

  const int summaryTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const std::string summaryLine =
      std::string(primaryModeText(hasSnapshot ? snapshot.primaryMode : HalBatteryPrimaryMode::Unavailable)) + " | " +
      summaryText(snapshot);
  const auto summary = renderer.truncatedText(SMALL_FONT_ID, summaryLine.c_str(), pageWidth - sidePadding * 2);
  renderer.drawText(SMALL_FONT_ID, sidePadding, summaryTop, summary.c_str(), true, EpdFontFamily::BOLD);

  const int panelTop = summaryTop + renderer.getLineHeight(SMALL_FONT_ID) + metrics.verticalSpacing + 2;
  const int panelHeight = pageHeight - panelTop - helpReservedHeight - metrics.verticalSpacing;
  const Rect panelRect{sidePadding, panelTop, pageWidth - sidePadding * 2, panelHeight};
  const auto panelLines = getPanelLines(selectedPanel);
  const size_t visibleLineCount = getVisibleLineCapacity();
  const size_t endIndex =
      std::min(panelLines.size(), static_cast<size_t>(panelScrollOffsets[selectedPanel]) + visibleLineCount);
  char panelTitle[48];
  std::snprintf(panelTitle,
                sizeof(panelTitle),
                "%s (%u/%u) %u-%u/%u",
                selectedPanel == 0 ? "BQ27220" : "SY6970",
                static_cast<unsigned>(selectedPanel + 1),
                2u,
                static_cast<unsigned>(panelScrollOffsets[selectedPanel] + 1),
                static_cast<unsigned>(endIndex),
                static_cast<unsigned>(panelLines.size()));

  drawPanel(renderer, panelRect, panelTitle, panelLines, panelScrollOffsets[selectedPanel], visibleLineCount);

  const int helpTop = panelTop + panelHeight + metrics.verticalSpacing;
  std::string helpText = std::string("U/D:Scroll  L/R:Panel  ") + tr(STR_CONFIRM) + ":" + tr(STR_UPDATE);
  GUI.drawHelpText(renderer, Rect{0, helpTop, pageWidth, helpLineHeight}, helpText.c_str());

  renderer.displayBuffer();
}
