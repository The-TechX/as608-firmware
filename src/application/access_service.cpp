#include "application/access_service.h"

AccessService::AccessService(FingerprintService& fingerprintService, StatusIndicator& indicator)
    : fingerprintService_(fingerprintService), indicator_(indicator) {}

void AccessService::begin() { indicator_.begin(); }

void AccessService::setIntegration(AccessIntegration* integration) {
  integration_ = integration;
}

void AccessService::tick() {
  if (integration_) integration_->tick();

  AccessDecision decision;
  if (integration_) {
    while (integration_->poll(decision)) {
      if (!pending_ || decision.requestId != pendingId_) {
        Serial.printf("[access] Ignoring stale response id=%lu\n",
                      static_cast<unsigned long>(decision.requestId));
        continue;
      }
      if (millis() - pendingSince_ >= kAuthorizationTimeoutMs) {
        // Timeout always wins, even if a response arrives in the same tick.
        break;
      }
      pending_ = false;
      Serial.printf("[access] Authorization id=%lu result=%s\n",
                    static_cast<unsigned long>(decision.requestId),
                    decision.authorized ? "ACCEPTED" : "REJECTED");
      if (decision.authorized) indicator_.identificationSucceeded();
      else indicator_.identificationFailed();
    }
  }

  if (pending_ && millis() - pendingSince_ >= kAuthorizationTimeoutMs) {
    Serial.printf("[access] Authorization timeout id=%lu\n",
                  static_cast<unsigned long>(pendingId_));
    pending_ = false;
    indicator_.identificationFailed();
  }

  const uint32_t sequence = fingerprintService_.eventSequence();
  if (sequence != lastSequence_) {
    lastSequence_ = sequence;
    const auto& match = fingerprintService_.lastResult();
    if (!match.matched || pending_ || !integration_) {
      // Unknown fingerprint, concurrent request or missing backend: deny.
      indicator_.identificationFailed();
    } else {
      AccessRequest request;
      request.sequence = sequence;
      request.fingerprintId = match.id;
      request.confidence = match.confidence;
      request.timestampMs = match.timestampMs;
      if (integration_->submit(request)) {
        pending_ = true;
        pendingId_ = sequence;
        pendingSince_ = millis();
        Serial.printf("[access] Authorization requested id=%lu fingerprint=%u\n",
                      static_cast<unsigned long>(pendingId_), request.fingerprintId);
      } else {
        Serial.println("[access] Transport unavailable: denied");
        indicator_.identificationFailed();
      }
    }
  }
  indicator_.tick();
}
