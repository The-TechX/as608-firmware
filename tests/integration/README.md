# Mock WSS gateway (development only)

This Docker service exposes **WSS** on TCP 8765 to the LAN, using a development TLS certificate. It simulates authorization; it is NOT Odoo and does not yet include the ESP32 WSS client.

## Start

From `tests/integration`:

```bash
mkdir -p certs
# Replace 192.168.1.10 with the LAN IP of the Docker host.
HOST_IP=192.168.1.10
openssl req -x509 -newkey rsa:2048 -nodes -days 30 \
  -keyout certs/server.key -out certs/server.crt \
  -subj "/CN=mock-gateway" \
  -addext "subjectAltName=IP:${HOST_IP},DNS:localhost"
chmod 644 certs/server.crt
chmod 644 certs/server.key
docker compose up --build -d
```

The mock listens on `wss://<docker-host-LAN-IP>:8765`. The ESP32 must be able to reach that host over the LAN. Do not use a Docker container IP. Firewall rules must permit TCP 8765.

For Python tests, use `python -m pip install -r mock_gateway/requirements.txt` and `python -m unittest -v test_wss.py` in this directory. The certificate is trusted explicitly by the test; ESP32 certificate trust/pinning will be implemented in the firmware client. Set `WSS_URL` and `WSS_CA_CERT` for remote tests.

## Deterministic responses

- Fingerprint ID 0: ACCEPTED
- Other IDs: REJECTED
- ID 998: delayed REJECTED (2 seconds)
- ID 999: no response (timeout)

Responses echo `request_id`. The mock is intended for an isolated trusted development LAN only; it has no authentication and should not be exposed to the internet. Certificates are local test artifacts and should not be committed.

## Next phase

Implement ESP32 WSS client with TLS validation, request correlation, nonblocking transport, fail-closed timeout, and a hardware-in-the-loop test. No existing LED semantics are changed in this PR.
