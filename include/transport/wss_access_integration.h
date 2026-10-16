#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include "application/access_service.h"

// Transport adapter. AccessService knows only AccessIntegration.
class WssAccessIntegration : public AccessIntegration {
 public:
  WssAccessIntegration(const char* host, uint16_t port, const char* caCert);
  void begin();
  void tick() override;
  bool submit(const AccessRequest& request) override;
  bool poll(AccessDecision& decision) override;
 private:
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  WebSocketsClient socket_;
  const char* host_;
  uint16_t port_;
  const char* caCert_;
  bool connected_ = false;
  bool queued_ = false;
  AccessDecision received_;
  bool hasReceived_ = false;
};
