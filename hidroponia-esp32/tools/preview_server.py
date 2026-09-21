"""
Servidor local para previsualizar el dashboard (data/) en un navegador
sin necesitar el hardware ESP32. Sirve los archivos estaticos y simula
lecturas de sensores por WebSocket, igual que lo haria el firmware real.

Uso:
    pip install websockets
    python3 tools/preview_server.py
    # abrir http://localhost:8080
"""

import asyncio
import http
import json
import mimetypes
import os
import random
import sys
import time

import websockets

DATA_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data"))
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

state = {"ph": 6.0, "temperature": 22.0, "tds": 700.0, "waterLevel": 80.0}
start_time = time.time()


def simulate():
    state["ph"] = max(4.0, min(8.5, state["ph"] + random.uniform(-0.05, 0.05)))
    state["temperature"] = max(15.0, min(30.0, state["temperature"] + random.uniform(-0.2, 0.2)))
    state["tds"] = max(300.0, min(1000.0, state["tds"] + random.uniform(-15, 15)))
    state["waterLevel"] = max(0.0, min(100.0, state["waterLevel"] + random.uniform(-2, 1)))
    return {
        "ph": round(state["ph"], 2),
        "temperature": round(state["temperature"], 1),
        "tds": round(state["tds"]),
        "waterLevel": round(state["waterLevel"]),
        "uptime": int(time.time() - start_time),
    }


async def ws_handler(websocket, path=None):
    try:
        while True:
            await websocket.send(json.dumps(simulate()))
            await asyncio.sleep(2)
    except websockets.ConnectionClosed:
        pass


async def process_request(path, request_headers):
    if path == "/ws":
        return None
    file_path = path.lstrip("/") or "index.html"
    full_path = os.path.join(DATA_DIR, file_path)
    if not os.path.isfile(full_path):
        return (http.HTTPStatus.NOT_FOUND, [], b"Not found\n")
    content_type, _ = mimetypes.guess_type(full_path)
    content_type = content_type or "application/octet-stream"
    with open(full_path, "rb") as f:
        body = f.read()
    headers = [("Content-Type", content_type), ("Content-Length", str(len(body)))]
    return (http.HTTPStatus.OK, headers, body)


async def main():
    async with websockets.serve(ws_handler, "0.0.0.0", PORT, process_request=process_request):
        print(f"Preview listo en http://localhost:{PORT}")
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
