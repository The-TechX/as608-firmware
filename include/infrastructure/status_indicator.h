#pragma once
#include <Arduino.h>

// Hardware output only. It does not decide whether an access is authorized.
class StatusIndicator {
 public:
  void begin();
  void identificationSucceeded();
  void identificationFailed();
  void tick();
 private:
  enum class Pattern { Idle, Success, Failure };
  Pattern pattern_ = Pattern::Idle;
  uint32_t startedAt_ = 0;
};
