#include <Arduino.h>
#include "application/device_service.h"
#include "infrastructure/config_store.h"
#include "infrastructure/wifi_manager.h"
#include "infrastructure/fingerprint_sensor.h"
#include "transport/http_api.h"
#include "transport/wss_access_integration.h"

#if __has_include("transport/wss_local_config.h")
#include "transport/wss_local_config.h"
#define WSS_CONFIGURED 1
#else
#define WSS_CONFIGURED 0
#endif

ConfigStore config;
WifiManager wifi(config);
FingerprintSensor fingerprint;
DeviceService device(wifi, fingerprint);
HttpApi http(device);
#if WSS_CONFIGURED
WssAccessIntegration wss(kWssHost, kWssPort, kWssCaCert);
#endif

void setup() {
  Serial.begin(115200);
  config.begin();
  wifi.begin();
  fingerprint.begin();
  device.beginServices();
#if WSS_CONFIGURED
  wss.begin();
  device.access().setIntegration(&wss);
#else
  Serial.println("[wss] Local configuration missing; access fails closed");
#endif
  http.begin();
}
void loop() {
  http.tick();
  device.tick();
}
