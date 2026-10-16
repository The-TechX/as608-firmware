#include "application/fingerprint_runtime.h"

namespace {
constexpr uint8_t kWakePin = 4;
constexpr uint8_t kLedPin = 2;
constexpr uint32_t kReleaseDebounceMs = 150;
constexpr uint32_t kSuccessMs = 1500;
constexpr uint32_t kFailurePulseMs = 500;
constexpr uint32_t kFailureGapMs = 250;
constexpr uint32_t kFailureTotalMs = 1500;
}

FingerprintRuntime::FingerprintRuntime(FingerprintSensor& sensor) : sensor_(sensor) {}

void FingerprintRuntime::begin() {
  pinMode(kWakePin, INPUT);
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);
  fingerWasPresent_ = digitalRead(kWakePin) == HIGH;
  armed_ = !fingerWasPresent_;
  Serial.println("[runtime] GPIO4 wake identification enabled");
}

void FingerprintRuntime::identify() {
  busy_ = true;
  const uint32_t start = millis();
  last_ = FingerprintMatch{};
  last_.available = true;
  if (!sensor_.online()) {
    last_.error = FINGERPRINT_PACKETRECIEVEERR;
  } else {
    uint16_t id = 0;
    uint16_t confidence = 0;
    last_.error = sensor_.identify(id, confidence);
    if (last_.error == FINGERPRINT_OK) {
      last_.matched = true;
      last_.id = id;
      last_.confidence = confidence;
    }
  }
  last_.elapsedMs = millis() - start;
  last_.timestampMs = millis();
  led_ = last_.matched ? LedState::Success : LedState::Failure;
  ledStarted_ = millis();
  Serial.printf("[runtime] match=%s id=%u confidence=%u error=%u elapsed=%lu ms\n",
                last_.matched ? "yes" : "no", last_.id, last_.confidence,
                last_.error, static_cast<unsigned long>(last_.elapsedMs));
  busy_ = false;
}

void FingerprintRuntime::updateLed(uint32_t now) {
  if (led_ == LedState::Idle) {
    digitalWrite(kLedPin, LOW);
    return;
  }
  const uint32_t elapsed = now - ledStarted_;
  if (led_ == LedState::Success) {
    digitalWrite(kLedPin, elapsed < kSuccessMs ? HIGH : LOW);
    if (elapsed >= kSuccessMs) led_ = LedState::Idle;
    return;
  }
  if (elapsed >= kFailureTotalMs) {
    digitalWrite(kLedPin, LOW);
    led_ = LedState::Idle;
    return;
  }
  digitalWrite(kLedPin, (elapsed < kFailurePulseMs ||
               (elapsed >= kFailurePulseMs + kFailureGapMs &&
                elapsed < 2 * kFailurePulseMs + kFailureGapMs)) ? HIGH : LOW);
}

void FingerprintRuntime::tick() {
  const uint32_t now = millis();
  updateLed(now);
  const bool present = digitalRead(kWakePin) == HIGH;
  if (!present) {
    if (fingerWasPresent_) releasedSince_ = now;
    if (!armed_ && now - releasedSince_ >= kReleaseDebounceMs) armed_ = true;
  } else if (armed_ && !fingerWasPresent_) {
    armed_ = false;
    identify();
  }
  fingerWasPresent_ = present;
}

const FingerprintMatch& FingerprintRuntime::lastResult() const { return last_; }
bool FingerprintRuntime::busy() const { return busy_; }
