#include "infrastructure/wifi_manager.h"
#include <WiFi.h>

namespace {
constexpr char kApSsid[] = "AS608-Setup";
constexpr char kApPassword[] = "AS608setup!26";
constexpr uint32_t kAttemptTimeoutMs = 15000;
constexpr uint32_t kRetryIntervalMs = 5000;
constexpr uint32_t kRecoveryAfterMs = 45000;
}

WifiManager::WifiManager(ConfigStore& store) : store_(store) {}

void WifiManager::begin() {
  ssid_ = store_.ssid();
  password_ = store_.password();
  if (ssid_.isEmpty()) {
    state_ = State::Unconfigured;
    startAp();
  } else {
    outageStarted_ = millis();
    connect();
  }
}

void WifiManager::startAp() {
  if (apActive_) return;
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(kApSsid, kApPassword)) {
    Serial.println("[wifi] Failed to start recovery AP");
    return;
  }
  dns_.start(53, "*", WiFi.softAPIP());
  apActive_ = true;
  Serial.printf("[wifi] Setup AP active at %s\n", WiFi.softAPIP().toString().c_str());
}

void WifiManager::stopAp() {
  if (!apActive_) return;
  dns_.stop();
  WiFi.softAPdisconnect(true);
  apActive_ = false;
  // Keep STA active after disabling the AP.
  WiFi.mode(WIFI_STA);
  Serial.println("[wifi] Setup AP disabled; STA only");
}

void WifiManager::connect() {
  if (ssid_.isEmpty()) {
    state_ = State::Unconfigured;
    startAp();
    return;
  }
  WiFi.mode(apActive_ ? WIFI_AP_STA : WIFI_STA);
  Serial.printf("[wifi] Connecting to SSID='%s'\n", ssid_.c_str());
  WiFi.begin(ssid_.c_str(), password_.c_str());
  attemptStarted_ = millis();
  lastAttempt_ = attemptStarted_;
  state_ = apActive_ ? State::Recovery : State::Connecting;
}

void WifiManager::setConnected() {
  state_ = State::Connected;
  Serial.printf("[wifi] Connected: %s\n", WiFi.localIP().toString().c_str());
  stopAp();
}

bool WifiManager::configure(const String& ssid, const String& password) {
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() < 8 || password.length() > 63) return false;
  ssid_ = ssid;
  password_ = password;
  store_.save(ssid_, password_);
  // Keep the portal available until the new STA connection succeeds.
  startAp();
  outageStarted_ = millis();
  connect();
  return true;
}

void WifiManager::forget() {
  store_.clear();
  ssid_ = "";
  password_ = "";
  WiFi.disconnect();
  state_ = State::Unconfigured;
  startAp();
  Serial.println("[wifi] Credentials cleared; setup mode");
}

void WifiManager::tick() {
  if (apActive_) dns_.processNextRequest();
  const uint32_t now = millis();

  if (state_ == State::Unconfigured) return;

  // Check every tick, including after a timed-out attempt: late success counts.
  if (WiFi.status() == WL_CONNECTED) {
    if (state_ != State::Connected) setConnected();
    return;
  }

  if (state_ == State::Connected) {
    Serial.println("[wifi] Connection lost; reconnecting");
    outageStarted_ = now;
    connect();
    return;
  }

  if (!apActive_ && now - outageStarted_ >= kRecoveryAfterMs) {
    Serial.println("[wifi] Prolonged outage; enabling recovery AP");
    startAp();
    state_ = State::Recovery;
  }

  // Retry without blocking; keep AP and saved credentials during recovery.
  if (now - attemptStarted_ >= kAttemptTimeoutMs &&
      now - lastAttempt_ >= kAttemptTimeoutMs + kRetryIntervalMs) {
    Serial.println("[wifi] Connection pending; retrying");
    connect();
  }
}

String WifiManager::ssid() const { return ssid_; }
String WifiManager::mode() const { return apActive_ ? (connected() ? "AP+STA" : "AP") : "STA"; }
String WifiManager::ip() const {
  return connected() ? WiFi.localIP().toString()
                     : (apActive_ ? WiFi.softAPIP().toString() : "");
}
bool WifiManager::connected() const { return WiFi.status() == WL_CONNECTED; }
bool WifiManager::configured() const { return !ssid_.isEmpty(); }
bool WifiManager::apActive() const { return apActive_; }
