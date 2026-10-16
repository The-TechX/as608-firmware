#include "infrastructure/wifi_manager.h"
#include <WiFi.h>
namespace {
constexpr char kApSsid[] = "AS608-Setup";
constexpr char kApPassword[] = "AS608setup!26";
constexpr uint32_t kTimeoutMs = 15000;
}
WifiManager::WifiManager(ConfigStore& store) : store_(store) {}
void WifiManager::begin() {
  ssid_ = store_.ssid();
  password_ = store_.password();
  if (ssid_.isEmpty()) startAp(); else connect();
}
void WifiManager::startAp() {
  if (apActive_) return;
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(kApSsid, kApPassword)) {
    Serial.println("Failed to start setup AP");
    return;
  }
  dns_.start(53, "*", WiFi.softAPIP());
  apActive_ = true;
  Serial.printf("Setup AP: %s at %s\n", kApSsid, WiFi.softAPIP().toString().c_str());
}
void WifiManager::connect() {
  if (ssid_.isEmpty()) { startAp(); return; }
  WiFi.mode(apActive_ ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  started_ = millis();
  connecting_ = true;
}
bool WifiManager::configure(const String& ssid, const String& password) {
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() < 8 || password.length() > 63) return false;
  ssid_ = ssid;
  password_ = password;
  store_.save(ssid_, password_);
  startAp();
  connect();
  return true;
}
void WifiManager::forget() {
  store_.clear();
  ssid_ = "";
  password_ = "";
  connecting_ = false;
  WiFi.disconnect();
  startAp();
}
void WifiManager::tick() {
  if (apActive_) dns_.processNextRequest();
  if (!connecting_) return;
  if (WiFi.status() == WL_CONNECTED) {
    connecting_ = false;
    Serial.printf("Connected: %s\n", WiFi.localIP().toString().c_str());
  } else if (millis() - started_ >= kTimeoutMs) {
    connecting_ = false;
    startAp();
    Serial.println("STA timeout; recovery AP active");
  }
}
String WifiManager::ssid() const { return ssid_; }
String WifiManager::mode() const { return apActive_ ? (connected() ? "AP+STA" : "AP") : "STA"; }
String WifiManager::ip() const { return connected() ? WiFi.localIP().toString() : (apActive_ ? WiFi.softAPIP().toString() : ""); }
bool WifiManager::connected() const { return WiFi.status() == WL_CONNECTED; }
bool WifiManager::configured() const { return !ssid_.isEmpty(); }
bool WifiManager::apActive() const { return apActive_; }
