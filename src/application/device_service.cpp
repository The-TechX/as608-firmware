#include "application/device_service.h"
#include <ESP.h>
DeviceService::DeviceService(WifiManager& wifi, FingerprintSensor& fingerprint)
    : wifi_(wifi), fingerprint_(fingerprint), runtime_(fingerprint), access_(runtime_, indicator_) {}
void DeviceService::tick() {
  wifi_.tick();
  runtime_.tick();
  access_.tick();
  if (rebootPending_ && millis() - rebootAt_ >= 1000) ESP.restart();
}
void DeviceService::scheduleReboot() { rebootAt_ = millis(); rebootPending_ = true; }
WifiManager& DeviceService::wifi() { return wifi_; }
FingerprintSensor& DeviceService::fingerprint() { return fingerprint_; }
FingerprintRuntime& DeviceService::runtime() { return runtime_; }
AccessService& DeviceService::access() { return access_; }
void DeviceService::beginRuntime() {
  runtime_.begin();
  access_.begin();
}
