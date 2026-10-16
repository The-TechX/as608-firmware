#include "infrastructure/status_indicator.h"

namespace {
constexpr uint8_t kLedPin = 2;
constexpr uint32_t kSuccessMs = 1500;
constexpr uint32_t kPulseMs = 500;
constexpr uint32_t kGapMs = 250;
constexpr uint32_t kFailureTotalMs = 2 * kPulseMs + 2 * kGapMs;
}

void StatusIndicator::begin() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);
}

void StatusIndicator::identificationSucceeded() {
  pattern_ = Pattern::Success;
  startedAt_ = millis();
  digitalWrite(kLedPin, HIGH);
}

void StatusIndicator::identificationFailed() {
  pattern_ = Pattern::Failure;
  startedAt_ = millis();
  digitalWrite(kLedPin, HIGH);
}

void StatusIndicator::tick() {
  const uint32_t elapsed = millis() - startedAt_;
  switch (pattern_) {
    case Pattern::Idle:
      digitalWrite(kLedPin, LOW);
      break;
    case Pattern::Success:
      if (elapsed >= kSuccessMs) {
        pattern_ = Pattern::Idle;
        digitalWrite(kLedPin, LOW);
      }
      break;
    case Pattern::Failure:
      if (elapsed >= kFailureTotalMs) {
        pattern_ = Pattern::Idle;
        digitalWrite(kLedPin, LOW);
      } else {
        digitalWrite(kLedPin, (elapsed < kPulseMs ||
            (elapsed >= kPulseMs + kGapMs &&
             elapsed < 2 * kPulseMs + kGapMs)) ? HIGH : LOW);
      }
      break;
  }
}
