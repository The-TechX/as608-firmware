#include <Arduino.h>
#include "application/device_service.h"
#include "infrastructure/config_store.h"
#include "infrastructure/wifi_manager.h"
#include "infrastructure/fingerprint_sensor.h"
#include "transport/http_api.h"

ConfigStore config;
WifiManager wifi(config);
FingerprintSensor fingerprint;
DeviceService device(wifi, fingerprint);
HttpApi http(device);

void setup() {
  Serial.begin(115200);
  config.begin();
  wifi.begin();
  fingerprint.begin();
  http.begin();
}
void loop() {
  http.tick();
  device.tick();
}
