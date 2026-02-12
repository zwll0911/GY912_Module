"""
UDP → WebSocket Relay for NAV Module
=====================================
Receives UDP sensor data from ESP32 and forwards to dashboard via local WebSocket.

Usage:
  1. Connect laptop to NAV_MODULE_OTA WiFi
  2. Run: python udp_relay.py
  3. Open dashboard, click WIFI connect (uses ws://localhost:8765)

Press Ctrl+C to stop.
"""

import asyncio
import socket

try:
    import websockets
except ImportError:
    print("Installing websockets...")
    import subprocess, sys
    subprocess.check_call([sys.executable, "-m", "pip", "install", "websockets"])
    import websockets

# Config
UDP_PORT = 4210
WS_PORT = 8765

# Connected dashboard clients
clients = set()


async def ws_handler(websocket):
    """Handle dashboard WebSocket connections."""
    clients.add(websocket)
    addr = websocket.remote_address
    print(f"[WS] Dashboard connected: {addr[0]}:{addr[1]}")
    try:
        await websocket.wait_closed()
    finally:
        clients.discard(websocket)
        print(f"[WS] Dashboard disconnected: {addr[0]}:{addr[1]}")


async def udp_listener():
    """Receive UDP packets from ESP32 and forward to all WebSocket clients."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", UDP_PORT))
    sock.setblocking(False)

    loop = asyncio.get_event_loop()
    print(f"[UDP] Listening on port {UDP_PORT}...")

    pkt_count = 0
    while True:
        try:
            data = await loop.run_in_executor(None, lambda: sock.recv(512))
            line = data.decode("utf-8", errors="ignore").strip()
            if line and clients:
                # Send to all connected dashboards
                await asyncio.gather(
                    *[c.send(line) for c in clients.copy()],
                    return_exceptions=True
                )
                pkt_count += 1
                if pkt_count % 250 == 0:
                    print(f"[UDP] {pkt_count} packets relayed ({len(clients)} client(s))")
        except Exception as e:
            await asyncio.sleep(0.001)


async def main():
    print("=" * 50)
    print("  NAV Module UDP → WebSocket Relay")
    print("=" * 50)
    print(f"[WS]  Dashboard server: ws://localhost:{WS_PORT}")
    print(f"[UDP] Listening for ESP32 on port {UDP_PORT}")
    print("-" * 50)

    async with websockets.serve(ws_handler, "0.0.0.0", WS_PORT):
        await udp_listener()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[RELAY] Stopped.")
