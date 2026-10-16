#pragma once
#include <Arduino.h>
#include <Preferences.h>
class ConfigStore {
 public:
  void begin();
  String ssid();
  String password();
  void save(const String& ssid, const String& password);
  void clear();
 private:
  Preferences prefs_;
};
