# WSS authorization integration test

This is a reproducible, LAN-only integration environment for the ESP32/AS608 firmware. A Docker-hosted mock gateway makes deterministic authorization decisions over **WSS with certificate verification**. It is not Odoo or a production authorization service.

## Requirements

- ESP32 DevKit V1 with AS608 wired and working; at least two enrolled templates (IDs 0 and 1 for the manual test)
- PlatformIO (environment `esp32dev`), USB serial access
- Docker Desktop or Docker Engine with Compose
- OpenSSL (Windows: Git for Windows / Scoop Git is supported)
- ESP32 and Docker host on a LAN where TCP 8765 is reachable
- Internet/NTP reachability for ESP32 clock synchronization (the firmware waits for a plausible clock before TLS)

**Security:** This mock intentionally accepts ID 0 and rejects ID 1; it has no authentication beyond server TLS. Never expose TCP 8765 to the public internet. Certificate and private key are generated locally, ignored by Git, and intended only for development.

## Windows: clean start

Open PowerShell at the repository root. Substitute your Docker host's reachable IPv4 address throughout. This example uses `192.168.1.66`.

```powershell
git switch feat/wss-integration-test
git pull origin feat/wss-integration-test

# If certificates already exist, remove BOTH files before regenerating for a new IP.
# Only do this when deliberately rotating the test certificate.
Remove-Item .\tests\integration\certs\server.crt, .\tests\integration\certs\server.key -ErrorAction SilentlyContinue

.\tests\integration\start.ps1 -HostIp 192.168.1.66
.\scripts\configure-wss.ps1 -HostIp 192.168.1.66

pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

`start.ps1` temporarily enters `tests/integration`, generates the test certificate if absent, starts Docker Compose, and restores the original working directory when it succeeds. `configure-wss.ps1` generates the **ignored local** header `include/transport/wss_local_config.h` containing the gateway IP, port and **public certificate only**. Re-run the configuration script and reflash after any certificate rotation. Do not commit generated credentials, certificates or device-specific headers.

The development certificate contains `IP:<host-ip>`, `DNS:<host-ip>`, and `DNS:localhost` subject alternative names. The DNS-form IP entry is required by the mbedTLS hostname-validation behavior observed with this ESP32 toolchain; no LAN DNS or mDNS configuration is required. The server key never goes into the firmware.

To find your Windows LAN address:

```powershell
Get-NetIPAddress -AddressFamily IPv4 | Format-Table InterfaceAlias,IPAddress
```

If script execution is blocked, review the script and invoke `powershell -ExecutionPolicy Bypass -File .\tests\integration\start.ps1 -HostIp 192.168.1.66`. The script can also accept `-OpenSslPath` when OpenSSL is not discoverable.

## Automated gateway tests

In a separate terminal:

```powershell
cd tests/integration
docker compose run --build --rm integration-tests
```

The test runner executes inside Docker; no host Python installation is needed. It uses `wss://localhost:8765` and verifies the mock protocol responses. **These tests do not replace hardware-in-the-loop validation.**

## Hardware-in-the-loop acceptance test

Keep the Docker gateway running and the ESP32 serial monitor open.

1. Boot the ESP32. Expect Wi-Fi connected, clock synchronization, and `[wss] Connected`. No TLS bypass is used.
2. Present enrolled fingerprint **ID 0**. Expect `[access] Authorization requested`, matching `[wss] Response ... decision=ACCEPTED`, `[access] ... result=ACCEPTED`, and the corresponding success indication.
3. Present enrolled fingerprint **ID 1**. Expect `decision=REJECTED`, `result=REJECTED`, and no success indication.
4. Present an unenrolled finger. Expect `[fingerprint] match=no` and **no authorization request** for that read.
5. Repeat IDs 0 and 1; ensure each response's `request_id` matches its outstanding request.

Representative successful log:

```text
[wss] Clock synchronized: ...; starting verified TLS
[wss] Connected
[fingerprint] match=yes id=0 ...
[access] Authorization requested id=1 fingerprint=0
[wss] Response id=1 decision=ACCEPTED
[access] Authorization id=1 result=ACCEPTED
[fingerprint] match=yes id=1 ...
[access] Authorization requested id=2 fingerprint=1
[wss] Response id=2 decision=REJECTED
[access] Authorization id=2 result=REJECTED
```

## Failure-mode tests (manual hardware validation required)

The access layer is designed to **fail closed** with a 3-second response timeout, one outstanding request and matching response IDs. Confirm on hardware:

| Scenario | Procedure | Expected |
| --- | --- | --- |
| Gateway unavailable | From `tests/integration`, run `docker compose stop mock-gateway`, then present ID 0 | No access granted; disconnected/send failure or timeout |
| Gateway restored | Run `docker compose start mock-gateway`; wait for `[wss] Connected`, then present ID 0 | New request accepted normally |
| No response | Configure the mock to suppress the response for a fingerprint ID, or use the existing mock-only ID 999 test path | Timeout at ~3 s; no success indication |
| Delayed response | Use the mock-only delayed ID 998 test path; ensure an old response cannot authorize a newer request | Old/nonmatching response ignored |
| Unenrolled finger | Present a finger without a stored template | No remote authorization request |

**Important:** IDs 998 and 999 are synthetic protocol test IDs, not normally enrolled AS608 fingerprints. Their behavior is exercised by the Docker integration tests; do not claim the hardware delay/timeout scenarios passed unless an actual hardware injection or test hook is run. A 2-second delay alone is below the 3-second timeout and is not sufficient to test a late response.

## Linux/macOS gateway setup

From `tests/integration`:

```bash
mkdir -p certs
HOST_IP=192.168.1.66 # replace with Docker host's reachable LAN IP
openssl req -x509 -newkey rsa:2048 -nodes -days 30 \
  -keyout certs/server.key -out certs/server.crt \
  -subj "/CN=mock-gateway" \
  -addext "subjectAltName=IP:${HOST_IP},DNS:${HOST_IP},DNS:localhost" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign,digitalSignature,keyEncipherment" \
  -addext "extendedKeyUsage=serverAuth"
docker compose up --build -d
docker compose run --build --rm integration-tests
```

For ESP32 firmware, generate the equivalent ignored `wss_local_config.h` with the correct host, port and PEM certificate; `configure-wss.ps1` is the provided Windows helper.

## Maintenance and cleanup

- Certificates are valid for 30 days; regenerate **both** `server.crt` and `server.key`, regenerate the firmware header, then reflash.
- If the Docker host IP changes, repeat the same rotation procedure.
- Stop the environment with `docker compose down` from `tests/integration`.
- The normal firmware does not run certificate self-test diagnostics or embed the server's private key.
- A successful `[wss] Connected` and authorization logs demonstrate connectivity and the application flow; failure scenarios must be tested separately before production use.
