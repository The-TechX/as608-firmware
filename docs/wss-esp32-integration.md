# ESP32 → Docker WSS mock (hardware test)

This is a **development-only** WSS integration. AccessService uses the abstract AccessIntegration contract; only WssAccessIntegration knows WebSocket/TLS.

## Windows PowerShell setup

From the repository root, after starting the mock with `tests/integration/start.ps1 -HostIp 192.168.1.66`:

```powershell
.\scripts\configure-wss.ps1 -HostIp 192.168.1.66
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

Or build/upload from PlatformIO VS Code. The configuration script copies the **public** development certificate into an ignored C++ header at `include/transport/wss_local_config.h`. Never commit this generated file; do not include the private key. If the certificate is regenerated or host IP changes, rerun the configuration script. The certificate SAN must contain the IP used.

Without this local header, firmware still compiles but **denies all access** (no authorization integration). Never disable TLS verification.

## Expected hardware behavior

- Known fingerprint ID **0**: gateway returns ACCEPTED; LED success **after** WSS response.
- Known fingerprint ID **1**: gateway returns REJECTED; LED failure.
- Unknown fingerprint: immediate local failure; no WSS request.
- Gateway offline / Wi-Fi down: failure; never success.
- Missing/late/mismatched response: fail closed (3-second application timeout).

The fingerprint sensor's IDs 0 and 1 must actually be enrolled to test both outcomes. IDs 998/999 are protocol-only test fixtures because AS608 capacity is 300.

## Caveats

The WebSockets library's TLS connect/reconnect handshake may block briefly during network failure; `tick()` does not guarantee hard real-time responsiveness. We should measure worst-case delays on hardware. The single pending request and one-slot response buffer are suitable for initial sequential fingerprint tests, not concurrent high-throughput authorization. The mock gateway has no device authentication; it must remain on a trusted LAN.

The Python Docker tests validate the gateway protocol, **not** the firmware, physical TLS handshake, or LED behavior. Capture serial logs and test on hardware before merging.
