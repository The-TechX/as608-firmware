#pragma once
#include <Arduino.h>
#include "application/fingerprint_runtime.h"
#include "infrastructure/status_indicator.h"

// An integration may enqueue an access request without waiting for a server response.
// The runtime never depends on the integration or its availability.
struct AccessRequest {
  uint32_t sequence = 0;
  uint16_t fingerprintId = 0;
  uint16_t confidence = 0;
  uint32_t timestampMs = 0;
};

class AccessIntegration {
 public:
  virtual ~AccessIntegration() = default;
  virtual void submit(const AccessRequest& request) = 0;
};

class AccessService {
 public:
  AccessService(FingerprintRuntime& runtime, StatusIndicator& indicator);
  void begin();
  void tick();
  void setIntegration(AccessIntegration* integration);
 private:
  FingerprintRuntime& runtime_;
  StatusIndicator& indicator_;
  AccessIntegration* integration_ = nullptr;
  uint32_t lastSequence_ = 0;
};
