#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/semphr.h>

#include <cassert>

class HalGPIO;

class HalPowerManager;
extern HalPowerManager powerManager;

class HalPowerManager {
  int normalFreq = 0;
  bool isLowPower = false;
  mutable int _batteryCachedPercent = 0;
  mutable unsigned long _batteryLastPollMs = 0;

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
