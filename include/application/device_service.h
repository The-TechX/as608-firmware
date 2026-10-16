#pragma once
#include <Arduino.h>
#include "infrastructure/wifi_manager.h"
#include "infrastructure/fingerprint_sensor.h"
#include "application/fingerprint_service.h"
#include "infrastructure/status_indicator.h"
#include "application/access_service.h"
class DeviceService {
 public:
  explicit DeviceService(WifiManager& wifi, FingerprintSensor& fingerprint);
  void tick();
  void scheduleReboot();
  WifiManager& wifi();
  FingerprintSensor& fingerprint();
  FingerprintService& fingerprintService();
  void beginServices();
  AccessService& access();
 private:
  WifiManager& wifi_;
  FingerprintSensor& fingerprint_;
  FingerprintService fingerprintService_;
  StatusIndicator indicator_;
  AccessService access_;
  bool rebootPending_ = false;
  uint32_t rebootAt_ = 0;
};
