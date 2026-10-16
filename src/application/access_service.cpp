#include "application/access_service.h"

AccessService::AccessService(FingerprintRuntime& runtime, StatusIndicator& indicator)
    : runtime_(runtime), indicator_(indicator) {}

void AccessService::begin() { indicator_.begin(); }

void AccessService::setIntegration(AccessIntegration* integration) {
  integration_ = integration;
}

void AccessService::tick() {
  const uint32_t sequence = runtime_.eventSequence();
  if (sequence != lastSequence_) {
    lastSequence_ = sequence;
    const auto& match = runtime_.lastResult();
    if (match.matched) {
      // This confirms biometric recognition only, NOT access authorization.
      indicator_.identificationSucceeded();
      if (integration_) {
        AccessRequest request;
        request.sequence = sequence;
        request.fingerprintId = match.id;
        request.confidence = match.confidence;
        request.timestampMs = match.timestampMs;
        integration_->submit(request);
      }
    } else {
      indicator_.identificationFailed();
    }
  }
  indicator_.tick();
}
