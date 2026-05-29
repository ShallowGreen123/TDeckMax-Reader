#include "HalPowerManager.h"

#include <Logging.h>
#include <TDeckMaxBoard.h>
#include <WiFi.h>

#include <algorithm>
#include <cassert>

namespace {
constexpr uint8_t BQ27220_CONTROL_REG = 0x00;
constexpr uint8_t BQ27220_TEMPERATURE_REG = 0x06;
constexpr uint8_t BQ27220_VOLTAGE_REG = 0x08;
constexpr uint8_t BQ27220_BATTERY_STATUS_REG = 0x0A;
constexpr uint8_t BQ27220_CURRENT_REG = 0x0C;
constexpr uint8_t BQ27220_REMAINING_CAPACITY_REG = 0x10;
constexpr uint8_t BQ27220_FULL_CHARGE_CAPACITY_REG = 0x12;
constexpr uint8_t BQ27220_AVERAGE_CURRENT_REG = 0x14;
constexpr uint8_t BQ27220_SOC_REG = 0x2C;
constexpr uint8_t BQ27220_SOH_REG = 0x2E;
constexpr uint8_t BQ27220_CHARGE_VOLTAGE_REG = 0x30;
constexpr uint8_t BQ27220_MAC_DATA_REG = 0x40;

constexpr uint16_t BQ27220_DEVICE_ID = 0x0220;
constexpr uint16_t BQ27220_CONTROL_DEVICE_NUMBER = 0x0001;
constexpr uint16_t BQ27220_CONTROL_GAUGING_STATUS = 0x0056;

constexpr uint16_t BQ27220_STATUS_DSG = 1u << 0;
constexpr uint16_t BQ27220_STATUS_TCA = 1u << 6;
constexpr uint16_t BQ27220_STATUS_CHGINH = 1u << 8;
constexpr uint16_t BQ27220_STATUS_FC = 1u << 9;
constexpr uint16_t BQ27220_STATUS_SLEEP = 1u << 12;

constexpr uint16_t BQ27220_GAUGING_FC = 1u << 1;
constexpr uint16_t BQ27220_GAUGING_TC = 1u << 3;
constexpr uint16_t BQ27220_GAUGING_DSG = 1u << 6;

constexpr unsigned long BATTERY_POLL_MS = 1500;
constexpr unsigned int BQ27220_SELECT_DELAY_US = 1000;
constexpr int16_t BQ27220_CURRENT_THRESHOLD_MA = 10;

bool readBq27220Reg16(const uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(BOARD_I2C_ADDR_BQ27220);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(BOARD_I2C_ADDR_BQ27220, static_cast<uint8_t>(2), static_cast<uint8_t>(true)) < 2) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }
  const uint8_t lo = Wire.read();
  const uint8_t hi = Wire.read();
  value = (static_cast<uint16_t>(hi) << 8) | lo;
  return true;
}

bool writeBq27220ControlSubcommand(const uint16_t subcommand) {
  Wire.beginTransmission(BOARD_I2C_ADDR_BQ27220);
  Wire.write(BQ27220_CONTROL_REG);
  Wire.write(static_cast<uint8_t>(subcommand & 0xFF));
  Wire.write(static_cast<uint8_t>(subcommand >> 8));
  return Wire.endTransmission(true) == 0;
}

bool readBq27220MacWord(const uint16_t subcommand, uint16_t& value) {
  if (!writeBq27220ControlSubcommand(subcommand)) {
    return false;
  }
  delayMicroseconds(BQ27220_SELECT_DELAY_US);
  return readBq27220Reg16(BQ27220_MAC_DATA_REG, value);
}

bool readGaugeSnapshot(HalGaugeStatusSnapshot& gauge, const HalChargerStatusSnapshot& charger) {
  gauge = {};

  uint16_t deviceNumber = 0;
  if (!readBq27220MacWord(BQ27220_CONTROL_DEVICE_NUMBER, deviceNumber) || deviceNumber != BQ27220_DEVICE_ID) {
    return false;
  }

  gauge.gaugeReady = true;

  uint16_t batteryStatus = 0;
  uint16_t gaugingStatus = 0;
  uint16_t soc = 0;
  uint16_t soh = 0;
  uint16_t voltage = 0;
  uint16_t temperature = 0;
  uint16_t remainingCapacity = 0;
  uint16_t fullCapacity = 0;
  uint16_t chargeVoltage = 0;
  uint16_t currentRaw = 0;
  uint16_t averageCurrentRaw = 0;

  const bool readOk =
      readBq27220Reg16(BQ27220_BATTERY_STATUS_REG, batteryStatus) &&
      readBq27220MacWord(BQ27220_CONTROL_GAUGING_STATUS, gaugingStatus) && readBq27220Reg16(BQ27220_SOC_REG, soc) &&
      readBq27220Reg16(BQ27220_SOH_REG, soh) && readBq27220Reg16(BQ27220_VOLTAGE_REG, voltage) &&
      readBq27220Reg16(BQ27220_TEMPERATURE_REG, temperature) &&
      readBq27220Reg16(BQ27220_CURRENT_REG, currentRaw) &&
      readBq27220Reg16(BQ27220_AVERAGE_CURRENT_REG, averageCurrentRaw) &&
      readBq27220Reg16(BQ27220_REMAINING_CAPACITY_REG, remainingCapacity) &&
      readBq27220Reg16(BQ27220_FULL_CHARGE_CAPACITY_REG, fullCapacity) &&
      readBq27220Reg16(BQ27220_CHARGE_VOLTAGE_REG, chargeVoltage);

  if (!readOk) {
    return false;
  }

  gauge.readOk = true;
  gauge.batteryStatusRaw = batteryStatus;
  gauge.gaugingStatusRaw = gaugingStatus;
  gauge.socPercent = std::min<uint16_t>(soc, 100);
  gauge.sohPercent = std::min<uint16_t>(soh, 100);
  gauge.voltageMv = voltage;
  gauge.temperatureDk = temperature;
  gauge.remainingCapacityMah = remainingCapacity;
  gauge.fullCapacityMah = fullCapacity;
  gauge.chargeVoltageMv = chargeVoltage;
  gauge.currentMa = static_cast<int16_t>(currentRaw);
  gauge.averageCurrentMa = static_cast<int16_t>(averageCurrentRaw);
  gauge.chargeInhibit = (batteryStatus & BQ27220_STATUS_CHGINH) != 0;
  gauge.taperReached = (batteryStatus & BQ27220_STATUS_TCA) != 0 || (gaugingStatus & BQ27220_GAUGING_TC) != 0;
  gauge.full = (batteryStatus & BQ27220_STATUS_FC) != 0 || (gaugingStatus & BQ27220_GAUGING_FC) != 0;
  gauge.chargeDone = gauge.full;

  const bool hasPositiveCurrent =
      gauge.currentMa > BQ27220_CURRENT_THRESHOLD_MA || gauge.averageCurrentMa > BQ27220_CURRENT_THRESHOLD_MA;
  const bool hasNegativeCurrent =
      gauge.currentMa < -BQ27220_CURRENT_THRESHOLD_MA || gauge.averageCurrentMa < -BQ27220_CURRENT_THRESHOLD_MA;

  gauge.charging = !gauge.full && !gauge.chargeInhibit && (charger.vbusConnected || hasPositiveCurrent) && hasPositiveCurrent;
  gauge.discharge =
      (batteryStatus & BQ27220_STATUS_DSG) != 0 || (gaugingStatus & BQ27220_GAUGING_DSG) != 0 || hasNegativeCurrent;

  if ((batteryStatus & BQ27220_STATUS_SLEEP) != 0) {
    gauge.state = HalGaugeState::Sleep;
  } else if (gauge.full) {
    gauge.state = HalGaugeState::Full;
  } else if (gauge.charging) {
    gauge.state = HalGaugeState::Charge;
  } else if (gauge.discharge) {
    gauge.state = HalGaugeState::Discharge;
  } else {
    gauge.state = HalGaugeState::Relax;
  }

  return true;
}

HalBatteryPrimaryMode classifyPrimaryMode(const HalBatteryStatusSnapshot& snapshot) {
  if ((snapshot.gauge.readOk && snapshot.gauge.full) || (snapshot.charger.readOk && snapshot.charger.chargeDone)) {
    return HalBatteryPrimaryMode::Full;
  }

  if ((snapshot.charger.readOk && snapshot.charger.charging) ||
      (snapshot.gauge.readOk && snapshot.gauge.state == HalGaugeState::Charge)) {
    return HalBatteryPrimaryMode::Charging;
  }

  if (snapshot.charger.readOk && snapshot.charger.vbusConnected) {
    return HalBatteryPrimaryMode::StandbyUsb;
  }

  if (snapshot.gauge.readOk || snapshot.charger.readOk || snapshot.gauge.gaugeReady || snapshot.charger.chargerReady) {
    return HalBatteryPrimaryMode::Discharge;
  }

  return HalBatteryPrimaryMode::Unavailable;
}
}  // namespace

HalPowerManager powerManager;

void HalPowerManager::begin() {
  normalFreq = getCpuFrequencyMhz();
  modeMutex = xSemaphoreCreateMutex();
  assert(modeMutex != nullptr);
}

void HalPowerManager::setPowerSaving(bool enabled) {
  if (normalFreq <= 0) {
    return;
  }

  if (WiFi.getMode() != WIFI_MODE_NULL) {
    enabled = false;
  }

  const LockMode mode = currentLockMode;
  if (mode == None && enabled && !isLowPower) {
    if (setCpuFrequencyMhz(LOW_POWER_FREQ)) {
      isLowPower = true;
    }
    return;
  }

  if ((!enabled || mode != None) && isLowPower) {
    if (setCpuFrequencyMhz(normalFreq)) {
      isLowPower = false;
    }
  }
}

void HalPowerManager::startDeepSleep(HalGPIO& gpio) const { gpio.startDeepSleep(); }

bool HalPowerManager::readBatteryStatus(HalBatteryStatusSnapshot& snapshot, const bool forceRefresh) const {
  const unsigned long now = millis();
  if (!forceRefresh && _batteryLastPollMs != 0 && (now - _batteryLastPollMs) < BATTERY_POLL_MS) {
    snapshot = _batteryCachedSnapshot;
    return snapshot.hasData;
  }

  HalBatteryStatusSnapshot nextSnapshot = {};
  gpio.readChargerStatus(nextSnapshot.charger);
  readGaugeSnapshot(nextSnapshot.gauge, nextSnapshot.charger);
  nextSnapshot.primaryMode = classifyPrimaryMode(nextSnapshot);
  nextSnapshot.hasData = nextSnapshot.gauge.gaugeReady || nextSnapshot.charger.chargerReady;

  if (nextSnapshot.gauge.readOk) {
    _batteryCachedPercent = nextSnapshot.gauge.socPercent;
  }

  _batteryCachedSnapshot = nextSnapshot;
  _batteryLastPollMs = now;
  snapshot = _batteryCachedSnapshot;
  return snapshot.hasData;
}

uint16_t HalPowerManager::getBatteryPercentage() const {
  HalBatteryStatusSnapshot snapshot = {};
  if (readBatteryStatus(snapshot, false) && snapshot.gauge.readOk) {
    return snapshot.gauge.socPercent;
  }
  return _batteryCachedPercent;
}

HalPowerManager::Lock::Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (powerManager.currentLockMode != None) {
    LOG_ERR("PWR", "Lock already held, ignore");
    valid = false;
  } else {
    powerManager.currentLockMode = NormalSpeed;
    valid = true;
  }
  xSemaphoreGive(powerManager.modeMutex);
  if (valid) {
    powerManager.setPowerSaving(false);
  }
}

HalPowerManager::Lock::~Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (valid) {
    powerManager.currentLockMode = None;
  }
  xSemaphoreGive(powerManager.modeMutex);
}
