#include "application/device_service.h"
#include <ESP.h>
DeviceService::DeviceService(WifiManager& wifi, FingerprintSensor& fingerprint)
    : wifi_(wifi), fingerprint_(fingerprint), runtime_(fingerprint) {}
void DeviceService::tick() {
  wifi_.tick();
  runtime_.tick();
  if (runtime_.eventSequence() != lastHandledEvent_) {
    lastHandledEvent_ = runtime_.eventSequence();
    if (runtime_.lastResult().matched) indicator_.identificationSucceeded();
    else indicator_.identificationFailed();
  }
  indicator_.tick();
  if (rebootPending_ && millis() - rebootAt_ >= 1000) ESP.restart();
}
void DeviceService::scheduleReboot() { rebootAt_ = millis(); rebootPending_ = true; }
WifiManager& DeviceService::wifi() { return wifi_; }
FingerprintSensor& DeviceService::fingerprint() { return fingerprint_; }
FingerprintRuntime& DeviceService::runtime() { return runtime_; }
void DeviceService::beginRuntime() {
  indicator_.begin();
  runtime_.begin();
}
