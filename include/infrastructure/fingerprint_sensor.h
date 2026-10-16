#pragma once
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

class FingerprintSensor {
 public:
  void begin();
  bool online() const;
  uint8_t lastError() const;
  uint16_t capacity() const;
  uint16_t templateCount() const;
  uint16_t securityLevel() const;
  uint32_t deviceAddress() const;
  uint16_t packetLength() const;
  uint16_t baudRate() const;
  bool refresh();
  uint8_t identify(uint16_t& id, uint16_t& confidence);
 private:
  HardwareSerial serial_{2};
  Adafruit_Fingerprint sensor_{&serial_};
  bool online_ = false;
  uint8_t lastError_ = 0;
  uint16_t capacity_ = 0;
  uint16_t templateCount_ = 0;
  uint16_t securityLevel_ = 0;
  uint32_t deviceAddress_ = 0;
  uint16_t packetLength_ = 0;
  uint16_t baudRate_ = 0;
};
