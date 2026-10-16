#pragma once
#include <Arduino.h>
#include "infrastructure/wifi_manager.h"
#include "infrastructure/fingerprint_sensor.h"
class DeviceService {
 public:
  explicit DeviceService(WifiManager& wifi, FingerprintSensor& fingerprint);
  void tick();
  void scheduleReboot();
  WifiManager& wifi();
  FingerprintSensor& fingerprint();
 private:
  WifiManager& wifi_;
  FingerprintSensor& fingerprint_;
  bool rebootPending_ = false;
  uint32_t rebootAt_ = 0;
};
