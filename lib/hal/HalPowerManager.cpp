#include "HalPowerManager.h"

#include <Logging.h>
#include <TDeckMaxBoard.h>
#include <WiFi.h>

#include <cassert>

#include "HalGPIO.h"

namespace {
constexpr uint8_t BQ27220_SOC_REG = 0x2C;
constexpr unsigned long BATTERY_POLL_MS = 1500;

bool readBq27220Reg16(uint8_t reg, uint16_t& value) {
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

uint16_t HalPowerManager::getBatteryPercentage() const {
  const unsigned long now = millis();
  if (_batteryLastPollMs != 0 && (now - _batteryLastPollMs) < BATTERY_POLL_MS) {
    return _batteryCachedPercent;
  }

  uint16_t soc = 0;
  if (readBq27220Reg16(BQ27220_SOC_REG, soc)) {
    _batteryCachedPercent = soc > 100 ? 100 : soc;
  }
  _batteryLastPollMs = now;
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
