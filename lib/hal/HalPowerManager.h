#pragma once

#include <Arduino.h>
#include <HalGPIO.h>
#include <Wire.h>
#include <freertos/semphr.h>

#include <cassert>

enum class HalGaugeState : uint8_t { Unknown, Sleep, Full, Charge, Discharge, Relax };

enum class HalBatteryPrimaryMode : uint8_t { Unavailable, Full, Charging, StandbyUsb, Discharge };

struct HalGaugeStatusSnapshot {
  bool gaugeReady = false;
  bool readOk = false;
  bool charging = false;
  bool full = false;
  bool chargeDone = false;
  bool chargeInhibit = false;
  bool taperReached = false;
  bool discharge = false;
  HalGaugeState state = HalGaugeState::Unknown;
  uint16_t socPercent = 0;
  uint16_t sohPercent = 0;
  uint16_t voltageMv = 0;
  uint16_t temperatureDk = 0;
  uint16_t remainingCapacityMah = 0;
  uint16_t fullCapacityMah = 0;
  uint16_t chargeVoltageMv = 0;
  uint16_t batteryStatusRaw = 0;
  uint16_t gaugingStatusRaw = 0;
  int16_t currentMa = 0;
  int16_t averageCurrentMa = 0;
};

struct HalBatteryStatusSnapshot {
  HalGaugeStatusSnapshot gauge = {};
  HalChargerStatusSnapshot charger = {};
  HalBatteryPrimaryMode primaryMode = HalBatteryPrimaryMode::Unavailable;
  bool hasData = false;
};

class HalPowerManager;
extern HalPowerManager powerManager;

class HalPowerManager {
  int normalFreq = 0;
  bool isLowPower = false;
  mutable int _batteryCachedPercent = 0;
  mutable unsigned long _batteryLastPollMs = 0;
  mutable HalBatteryStatusSnapshot _batteryCachedSnapshot = {};

  enum LockMode { None, NormalSpeed };
  LockMode currentLockMode = None;
  SemaphoreHandle_t modeMutex = nullptr;

 public:
  static constexpr int LOW_POWER_FREQ = 10;
  static constexpr unsigned long IDLE_POWER_SAVING_MS = 3000;

  void begin();
  void setPowerSaving(bool enabled);
  void startDeepSleep(HalGPIO& gpio) const;
  uint16_t getBatteryPercentage() const;
  bool readBatteryStatus(HalBatteryStatusSnapshot& snapshot, bool forceRefresh = false) const;

  class Lock {
    friend class HalPowerManager;
    bool valid = false;

   public:
    explicit Lock();
    ~Lock();

    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;
  };
};
