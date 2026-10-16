#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include "infrastructure/config_store.h"
class WifiManager {
 public:
  explicit WifiManager(ConfigStore& store);
  void begin();
  void tick();
  bool configure(const String& ssid, const String& password);
  void forget();
  String ssid() const;
  String mode() const;
  String ip() const;
  bool connected() const;
  bool configured() const;
  bool apActive() const;
 private:
  void startAp();
  void connect();
  ConfigStore& store_;
  DNSServer dns_;
  String ssid_;
  String password_;
  bool apActive_ = false;
  bool connecting_ = false;
  uint32_t started_ = 0;
};
