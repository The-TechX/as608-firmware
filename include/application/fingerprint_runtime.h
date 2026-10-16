#pragma once
#include <Arduino.h>
#include "infrastructure/fingerprint_sensor.h"

struct FingerprintMatch {
  bool available = false;
  bool matched = false;
  uint16_t id = 0;
  uint16_t confidence = 0;
  uint8_t error = 0;
  uint32_t elapsedMs = 0;
  uint32_t timestampMs = 0;
};

class FingerprintRuntime {
 public:
  explicit FingerprintRuntime(FingerprintSensor& sensor);
  void begin();
  void tick();
  const FingerprintMatch& lastResult() const;
  bool busy() const;
 private:
  enum class LedState { Idle, Success, Failure };
  void identify();
  void updateLed(uint32_t now);
  FingerprintSensor& sensor_;
  FingerprintMatch last_;
  LedState led_ = LedState::Idle;
  uint32_t ledStarted_ = 0;
  bool armed_ = true;
  bool fingerWasPresent_ = false;
  uint32_t releasedSince_ = 0;
  bool busy_ = false;
};
