#include <Arduino.h>
#include "application/device_service.h"
#include "infrastructure/config_store.h"
#include "infrastructure/wifi_manager.h"
#include "transport/http_api.h"

ConfigStore config;
WifiManager wifi(config);
DeviceService device(wifi);
HttpApi http(device);

void setup() {
  Serial.begin(115200);
  config.begin();
  wifi.begin();
  http.begin();
}
void loop() {
  http.tick();
  device.tick();
}
