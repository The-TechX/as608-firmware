#include "transport/wss_access_integration.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
WssAccessIntegration::WssAccessIntegration(const char* host, uint16_t port, const char* caCert)
    : host_(host), port_(port), caCert_(caCert) {}

void WssAccessIntegration::begin() {
  if (!host_ || !*host_ || !caCert_ || !*caCert_) {
    Serial.println("[wss] Disabled: missing host or CA certificate");
    return;
  }
  Serial.println("[wss] TLS configured; waiting for valid system clock");
}

void WssAccessIntegration::tick() {
  if (!host_ || !*host_ || !caCert_ || !*caCert_) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (!clockRequested_) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    clockRequested_ = true;
    Serial.println("[wss] Requesting SNTP time");
  }
  time_t now = time(nullptr);
  // Refuse TLS until the clock is plausible (2024-01-01 UTC).
  if (now < 1704067200) {
    if (millis() - lastClockLogMs_ >= 10000) {
      lastClockLogMs_ = millis();
      Serial.println("[wss] Waiting for clock synchronization");
    }
    return;
  }
  if (!socketStarted_) {
    Serial.printf("[wss] Clock synchronized: %ld; starting verified TLS\n",
                  static_cast<long>(now));
    socket_.beginSslWithCA(host_, port_, "/", caCert_);
    socket_.setReconnectInterval(5000);
    socket_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
      onEvent(type, payload, length);
    });
    socketStarted_ = true;
  }
  socket_.loop();
}

bool WssAccessIntegration::submit(const AccessRequest& request) {
  if (!connected_ || WiFi.status() != WL_CONNECTED) return false;
  JsonDocument doc;
  doc["type"] = "access.request";
  doc["request_id"] = request.sequence;
  doc["fingerprint_id"] = request.fingerprintId;
  doc["confidence"] = request.confidence;
  String message;
  serializeJson(doc, message);
  return socket_.sendTXT(message);
}

bool WssAccessIntegration::poll(AccessDecision& decision) {
  if (!hasReceived_) return false;
  decision = received_;
  hasReceived_ = false;
  return true;
}

void WssAccessIntegration::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    connected_ = true;
    Serial.println("[wss] Connected");
  } else if (type == WStype_DISCONNECTED) {
    connected_ = false;
    Serial.println("[wss] Disconnected");
  } else if (type == WStype_TEXT) {
    JsonDocument doc;
    if (deserializeJson(doc, payload, length)) return;
    if (doc["type"] != "access.response" || !doc["request_id"].is<uint32_t>() ||
        !doc["decision"].is<const char*>()) return;
    const char* value = doc["decision"];
    if (strcmp(value, "ACCEPTED") != 0 && strcmp(value, "REJECTED") != 0) return;
    received_.requestId = doc["request_id"].as<uint32_t>();
    received_.authorized = strcmp(value, "ACCEPTED") == 0;
    hasReceived_ = true;
    Serial.printf("[wss] Response id=%lu decision=%s\n",
                  static_cast<unsigned long>(received_.requestId), value);
  }
}
