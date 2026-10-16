# AS608 Firmware

Minimal PlatformIO firmware for ESP32 DevKit V1. First milestone: Wi-Fi provisioning and device management, with a tiny HTML page backed by REST.

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

AS608 driver, identification, LED signaling, and WSS transport are intentionally deferred.
