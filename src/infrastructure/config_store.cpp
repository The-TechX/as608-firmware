#include "infrastructure/config_store.h"
void ConfigStore::begin() { prefs_.begin("device", false); }
String ConfigStore::ssid() { return prefs_.getString("ssid", ""); }
String ConfigStore::password() { return prefs_.getString("password", ""); }
void ConfigStore::save(const String& ssid, const String& password) {
  prefs_.putString("ssid", ssid);
  prefs_.putString("password", password);
}
void ConfigStore::clear() { prefs_.remove("ssid"); prefs_.remove("password"); }
