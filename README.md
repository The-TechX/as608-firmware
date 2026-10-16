# AS608 Firmware

PlatformIO firmware for ESP32 DevKit V1 and AS608 fingerprint sensor. Includes Wi-Fi provisioning, device management REST endpoints, GPIO4-triggered fingerprint identification, LED signaling, and a WSS authorization adapter. The remote gateway makes the access decision; a recognized fingerprint alone does not grant access.

## Build

Install PlatformIO in VS Code, open this repository, select `esp32dev`, then Build and Upload.

## First boot

Without saved Wi-Fi credentials, the ESP32 starts a recovery AP:

- SSID: `AS608-Setup`
- Password: `AS608setup!26`
- Open: `http://192.168.4.1`

Use the page to configure Wi-Fi, check status, reboot, or forget the network. When a saved network cannot be reached, the recovery AP starts after 15 seconds. During provisioning, AP remains available for recovery.

## REST endpoints

- `GET /api/v1/health`
- `GET /api/v1/device`
- `POST /api/v1/device/reboot`
- `GET /api/v1/wifi`
- `PUT /api/v1/wifi` with JSON `{"ssid":"Network","password":"secret123"}`
- `DELETE /api/v1/wifi`

**Security:** This is a LAN-only prototype. REST operations are currently unauthenticated and served over HTTP; the recovery AP password is a shared default. Do not expose the device to the internet or an untrusted network. Authentication and secure provisioning are required before production use.

## Fingerprint authorization over WSS

The AS608 identifies an enrolled fingerprint. The firmware sends an `access.request` over verified WSS and only grants access on a matching `access.response` with `ACCEPTED`. Rejection, transport failure and timeout must not grant access. TLS startup waits for SNTP time synchronization. The development gateway is a mock, not an Odoo integration.

**Reproduce the gateway, certificate provisioning, firmware upload and hardware acceptance tests:** [WSS integration test guide](tests/integration/README.md). Development certificates and device-specific WSS configuration are generated locally and must not be committed.

