#pragma once
#include <Arduino.h>
#include "application/fingerprint_service.h"
#include "infrastructure/status_indicator.h"

struct AccessRequest {
  uint32_t sequence = 0;
  uint16_t fingerprintId = 0;
  uint16_t confidence = 0;
  uint32_t timestampMs = 0;
};

struct AccessDecision {
  uint32_t requestId = 0;
  bool authorized = false;
};

// Transport-independent, asynchronous authorization contract.
class AccessIntegration {
 public:
  virtual ~AccessIntegration() = default;
  // true means submitted to the transport, NOT authorized.
  virtual bool submit(const AccessRequest& request) = 0;
  virtual bool poll(AccessDecision& decision) = 0;
  virtual void tick() = 0;
};

class AccessService {
 public:
  AccessService(FingerprintService& fingerprintService, StatusIndicator& indicator);
  void begin();
  void tick();
  void setIntegration(AccessIntegration* integration);
 private:
  static constexpr uint32_t kAuthorizationTimeoutMs = 3000;
  FingerprintService& fingerprintService_;
  StatusIndicator& indicator_;
  AccessIntegration* integration_ = nullptr;
  uint32_t lastSequence_ = 0;
  uint32_t pendingId_ = 0;
  uint32_t pendingSince_ = 0;
  bool pending_ = false;
};
