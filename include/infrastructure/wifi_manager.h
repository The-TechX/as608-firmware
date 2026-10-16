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
  enum class State { Unconfigured, Connecting, Connected, Recovery };
  void startAp();
  void stopAp();
  void connect();
  void setConnected();
  ConfigStore& store_;
  DNSServer dns_;
  String ssid_;
  String password_;
  State state_ = State::Unconfigured;
  bool apActive_ = false;
  uint32_t attemptStarted_ = 0;
  uint32_t outageStarted_ = 0;
  uint32_t lastAttempt_ = 0;
};
