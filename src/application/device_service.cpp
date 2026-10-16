#include "application/device_service.h"
#include <ESP.h>
DeviceService::DeviceService(WifiManager& wifi) : wifi_(wifi) {}
void DeviceService::tick() {
  wifi_.tick();
  if (rebootPending_ && millis() - rebootAt_ >= 1000) ESP.restart();
}
void DeviceService::scheduleReboot() { rebootAt_ = millis(); rebootPending_ = true; }
WifiManager& DeviceService::wifi() { return wifi_; }
