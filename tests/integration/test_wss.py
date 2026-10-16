import asyncio
import json
import os
import ssl
import unittest
import websockets

URL = os.getenv("WSS_URL", "wss://localhost:8765")
CA_CERT = os.getenv("WSS_CA_CERT", "certs/server.crt")

class GatewayIntegrationTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.connection = await websockets.connect(
            URL, ssl=ssl.create_default_context(cafile=CA_CERT)
        )

    async def asyncTearDown(self):
        await self.connection.close()

    async def request(self, fingerprint_id, request_id):
        await self.connection.send(json.dumps({
            "type": "access.request", "request_id": request_id,
            "fingerprint_id": fingerprint_id, "confidence": 320,
        }))
        return json.loads(await asyncio.wait_for(self.connection.recv(), timeout=5))

    async def test_accepted(self):
        response = await self.request(0, 1)
        self.assertEqual(response, {
            "type": "access.response", "request_id": 1, "decision": "ACCEPTED"
        })

    async def test_rejected(self):
        response = await self.request(1, 2)
        self.assertEqual(response["decision"], "REJECTED")
        self.assertEqual(response["request_id"], 2)

    async def test_delayed_response(self):
        response = await self.request(998, 3)
        self.assertEqual(response["request_id"], 3)
        self.assertEqual(response["decision"], "REJECTED")

    async def test_timeout(self):
        await self.connection.send(json.dumps({
            "type": "access.request", "request_id": 4, "fingerprint_id": 999
        }))
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(self.connection.recv(), timeout=0.5)

if __name__ == "__main__":
    unittest.main()
