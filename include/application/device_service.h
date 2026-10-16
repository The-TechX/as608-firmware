#pragma once
#include <Arduino.h>
#include "infrastructure/wifi_manager.h"
class DeviceService {
 public:
  explicit DeviceService(WifiManager& wifi);
  void tick();
  void scheduleReboot();
  WifiManager& wifi();
 private:
  WifiManager& wifi_;
  bool rebootPending_ = false;
  uint32_t rebootAt_ = 0;
};
