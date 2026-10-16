# Mock WSS gateway (development only)

This Docker service exposes **WSS** on TCP 8765 to the LAN, using a development TLS certificate. It simulates authorization; it is NOT Odoo and does not yet include the ESP32 WSS client.

## Windows PowerShell + Docker Desktop

Open PowerShell in `tests/integration`. Ensure Docker Desktop is running, and OpenSSL and Python are installed.

```powershell
# Find your LAN IPv4 (Wi-Fi or Ethernet):
Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like '192.168.*' -or $_.IPAddress -like '10.*' -or $_.IPAddress -like '172.*' } | Format-Table InterfaceAlias,IPAddress

# Replace with the Windows host IPv4 reachable by your ESP32:
.\start.ps1 -HostIp 192.168.1.50

# Install Python test dependency and execute tests:
python -m pip install -r mock_gateway/requirements.txt
python -m unittest -v test_wss.py
```

The script generates a self-signed certificate containing the selected IP as an IP subject alternative name, if certificates do not already exist, then runs Docker Compose. It does not require changing the host IP in source files. If you change the IP, remove both files in `certs` before running the script again.

If PowerShell blocks script execution, run `powershell -ExecutionPolicy Bypass -File .\start.ps1 -HostIp 192.168.1.50` after reviewing the script. OpenSSL must be on PATH; Git for Windows often includes it under `C:\Program Files\Git\usr\bin\openssl.exe`, but availability varies.

If multiple interfaces are detected, the script requires an explicit `-HostIp`. It does not alter Windows Firewall; allow inbound TCP 8765 only on the private LAN if necessary.

## Linux/macOS

From `tests/integration`:

```bash
mkdir -p certs
HOST_IP=192.168.1.10 # replace with Docker host's reachable LAN IP
openssl req -x509 -newkey rsa:2048 -nodes -days 30 \
  -keyout certs/server.key -out certs/server.crt \
  -subj "/CN=mock-gateway" \
  -addext "subjectAltName=IP:${HOST_IP},DNS:localhost"
chmod 644 certs/server.crt
chmod 644 certs/server.key
docker compose up --build -d
```

## Connection and testing

The mock listens on `wss://<docker-host-LAN-IP>:8765`. The ESP32 must reach the Windows/Linux Docker host, not the container's internal IP. The server certificate must match the IP/DNS name used, and clients must explicitly trust the development certificate. The Python test trusts `certs/server.crt`; ESP32 certificate validation is not yet implemented.

To test from a different machine, set `WSS_URL` and `WSS_CA_CERT` appropriately. On Windows, for example: `$env:WSS_URL = "wss://192.168.1.50:8765"`.

## Deterministic responses

- Fingerprint ID 0: ACCEPTED
- Other IDs: REJECTED
- ID 998: delayed REJECTED (2 seconds)
- ID 999: no response (timeout)

Responses echo `request_id`. This mock is for an isolated development LAN only; it has no authentication. Do not expose it to the internet or commit the certificates.

## Next phase

Implement ESP32 WSS client with TLS validation, request correlation, nonblocking transport, fail-closed timeout, and hardware-in-the-loop tests. No existing LED semantics are changed in this PR.
