#pragma once
#include <WebServer.h>
#include "application/device_service.h"
class HttpApi {
 public:
  explicit HttpApi(DeviceService& device);
  void begin();
  void tick();
 private:
  void sendJson(int status, const String& body);
  DeviceService& device_;
  WebServer server_{80};
};
