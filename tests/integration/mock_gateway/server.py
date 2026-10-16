import asyncio
import json
import os
import ssl
from websockets.asyncio.server import serve

HOST = os.getenv("HOST", "0.0.0.0")
PORT = int(os.getenv("PORT", "8765"))
CERT = os.getenv("TLS_CERT", "/certs/server.crt")
KEY = os.getenv("TLS_KEY", "/certs/server.key")

async def handler(socket):
    async for raw in socket:
        try:
            message = json.loads(raw)
            if message.get("type") != "access.request" or not isinstance(message.get("request_id"), int):
                await socket.send(json.dumps({"type": "error", "reason": "invalid_request"}))
                continue
            fingerprint_id = message.get("fingerprint_id")
            if not isinstance(fingerprint_id, int):
                await socket.send(json.dumps({"type": "error", "reason": "invalid_fingerprint_id"}))
                continue
            # Deterministic test rules: 0 accepted, other IDs rejected;
            # 998 delayed, 999 no response (client timeout).
            if fingerprint_id == 999:
                continue
            if fingerprint_id == 998:
                await asyncio.sleep(2)
            decision = "ACCEPTED" if fingerprint_id == 0 else "REJECTED"
            await socket.send(json.dumps({
                "type": "access.response",
                "request_id": message["request_id"],
                "decision": decision,
            }))
        except (ValueError, TypeError):
            await socket.send(json.dumps({"type": "error", "reason": "invalid_json"}))

async def main():
    tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls.load_cert_chain(CERT, KEY)
    async with serve(handler, HOST, PORT, ssl=tls):
        print(f"Mock WSS gateway listening on {HOST}:{PORT}", flush=True)
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())
