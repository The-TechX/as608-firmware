#include "transport/http_api.h"
#include "web/setup_page.h"
#include <ArduinoJson.h>
#include <ESP.h>
HttpApi::HttpApi(DeviceService& device) : device_(device) {}
void HttpApi::sendJson(int status, const String& body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status, "application/json", body);
}
void HttpApi::begin() {
  server_.on("/", HTTP_GET, [this]() { server_.send_P(200, "text/html; charset=utf-8", kSetupPage); });
  server_.on("/api/v1/health", HTTP_GET, [this]() { sendJson(200, "{\"ok\":true}"); });
  server_.on("/api/v1/device", HTTP_GET, [this]() {
    JsonDocument doc;
    doc["uptime_ms"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    String body;
    serializeJson(doc, body);
    sendJson(200, body);
  });
  server_.on("/api/v1/fingerprint/last", HTTP_GET, [this]() {
    const auto& result = device_.runtime().lastResult();
    JsonDocument doc;
    doc["available"] = result.available;
    doc["busy"] = device_.runtime().busy();
    if (result.available) {
      doc["matched"] = result.matched;
      doc["fingerprint_id"] = result.matched ? result.id : 0;
      doc["confidence"] = result.matched ? result.confidence : 0;
      doc["error"] = result.error;
      doc["elapsed_ms"] = result.elapsedMs;
      doc["timestamp_ms"] = result.timestampMs;
    }
    String body;
    serializeJson(doc, body);
    sendJson(200, body);
  });
  server_.on("/api/v1/sensor", HTTP_GET, [this]() {
    auto& sensor = device_.fingerprint();
    JsonDocument doc;
    doc["online"] = sensor.online();
    doc["last_error"] = sensor.lastError();
    if (sensor.online()) {
      doc["capacity"] = sensor.capacity();
      doc["template_count"] = sensor.templateCount();
      doc["security_level"] = sensor.securityLevel();
      doc["device_address"] = sensor.deviceAddress();
      doc["packet_length"] = sensor.packetLength();
      doc["baud_rate"] = sensor.baudRate();
    }
    String body;
    serializeJson(doc, body);
    sendJson(sensor.online() ? 200 : 503, body);
  });
  server_.on("/api/v1/sensor/refresh", HTTP_POST, [this]() {
    auto& sensor = device_.fingerprint();
    bool ok = sensor.refresh();
    JsonDocument doc;
    doc["ok"] = ok;
    doc["last_error"] = sensor.lastError();
    doc["template_count"] = sensor.templateCount();
    String body;
    serializeJson(doc, body);
    sendJson(ok ? 200 : 503, body);
  });
  server_.on("/api/v1/wifi", HTTP_GET, [this]() {
    auto& wifi = device_.wifi();
    JsonDocument doc;
    doc["ssid"] = wifi.ssid();
    doc["configured"] = wifi.configured();
    doc["connected"] = wifi.connected();
    doc["mode"] = wifi.mode();
    doc["ip"] = wifi.ip();
    String body;
    serializeJson(doc, body);
    sendJson(200, body);
  });
  server_.on("/api/v1/wifi", HTTP_PUT, [this]() {
    JsonDocument doc;
    if (deserializeJson(doc, server_.arg("plain"))) {
      sendJson(400, "{\"error\":\"Invalid JSON\"}");
      return;
    }
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    // Blank password is not accepted for updates: clients must provide it.
    if (!device_.wifi().configure(ssid, password)) {
      sendJson(400, "{\"error\":\"SSID must be 1-32 chars; password must be 8-63 chars\"}");
      return;
    }
    sendJson(202, "{\"ok\":true,\"message\":\"Connecting; refresh status shortly\"}");
  });
  server_.on("/api/v1/wifi", HTTP_DELETE, [this]() {
    device_.wifi().forget();
    sendJson(200, "{\"ok\":true,\"message\":\"Wi-Fi credentials cleared\"}");
  });
  server_.on("/api/v1/device/reboot", HTTP_POST, [this]() {
    sendJson(202, "{\"ok\":true,\"message\":\"Reboot scheduled\"}");
    device_.scheduleReboot();
  });
  server_.onNotFound([this]() {
    if (device_.wifi().apActive() && server_.method() == HTTP_GET) {
      server_.sendHeader("Location", "http://192.168.4.1/", true);
      server_.send(302, "text/plain", "");
    } else sendJson(404, "{\"error\":\"Not found\"}");
  });
  server_.begin();
}
void HttpApi::tick() { server_.handleClient(); }
