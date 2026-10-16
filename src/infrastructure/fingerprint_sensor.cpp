#include "infrastructure/fingerprint_sensor.h"

namespace {
constexpr int kRxPin = 16;
constexpr int kTxPin = 17;
constexpr uint32_t kBaud = 57600;
}

void FingerprintSensor::begin() {
  serial_.begin(kBaud, SERIAL_8N1, kRxPin, kTxPin);
  sensor_.begin(kBaud);
  online_ = sensor_.verifyPassword();
  if (!online_) {
    lastError_ = FINGERPRINT_PACKETRECIEVEERR;
    Serial.println("[sensor] AS608 not responding on UART2");
    return;
  }
  Serial.println("[sensor] AS608 detected");
  refresh();
}

bool FingerprintSensor::refresh() {
  if (!online_) return false;
  lastError_ = sensor_.getParameters();
  if (lastError_ != FINGERPRINT_OK) {
    Serial.printf("[sensor] Parameter read failed: %u\n", lastError_);
    return false;
  }
  capacity_ = sensor_.capacity;
  securityLevel_ = sensor_.security_level;
  deviceAddress_ = sensor_.device_addr;
  packetLength_ = sensor_.packet_len;
  baudRate_ = sensor_.baud_rate;
  lastError_ = sensor_.getTemplateCount();
  if (lastError_ != FINGERPRINT_OK) {
    Serial.printf("[sensor] Template count failed: %u\n", lastError_);
    return false;
  }
  templateCount_ = sensor_.templateCount;
  Serial.printf("[sensor] Capacity=%u templates=%u security=%u\n",
                capacity_, templateCount_, securityLevel_);
  return true;
}

bool FingerprintSensor::online() const { return online_; }
uint8_t FingerprintSensor::lastError() const { return lastError_; }
uint16_t FingerprintSensor::capacity() const { return capacity_; }
uint16_t FingerprintSensor::templateCount() const { return templateCount_; }
uint16_t FingerprintSensor::securityLevel() const { return securityLevel_; }
uint32_t FingerprintSensor::deviceAddress() const { return deviceAddress_; }
uint16_t FingerprintSensor::packetLength() const { return packetLength_; }
uint16_t FingerprintSensor::baudRate() const { return baudRate_; }

uint8_t FingerprintSensor::identify(uint16_t& id, uint16_t& confidence) {
  id = 0;
  confidence = 0;
  if (!online_) return FINGERPRINT_PACKETRECIEVEERR;
  uint8_t result = sensor_.getImage();
  if (result != FINGERPRINT_OK) return result;
  result = sensor_.image2Tz(1);
  if (result != FINGERPRINT_OK) return result;
  result = sensor_.fingerFastSearch();
  if (result == FINGERPRINT_OK) {
    id = sensor_.fingerID;
    confidence = sensor_.confidence;
  }
  return result;
}
