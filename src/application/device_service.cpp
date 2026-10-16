#include "application/device_service.h"
#include <ESP.h>
DeviceService::DeviceService(WifiManager& wifi, FingerprintSensor& fingerprint)
    : wifi_(wifi), fingerprint_(fingerprint), fingerprintService_(fingerprint), access_(fingerprintService_, indicator_) {}
void DeviceService::tick() {
  wifi_.tick();
  fingerprintService_.tick();
  access_.tick();
  if (rebootPending_ && millis() - rebootAt_ >= 1000) ESP.restart();
}
void DeviceService::scheduleReboot() { rebootAt_ = millis(); rebootPending_ = true; }
WifiManager& DeviceService::wifi() { return wifi_; }
FingerprintSensor& DeviceService::fingerprint() { return fingerprint_; }
FingerprintService& DeviceService::fingerprintService() { return fingerprintService_; }
AccessService& DeviceService::access() { return access_; }
void DeviceService::beginServices() {
  fingerprintService_.begin();
  access_.begin();
}
