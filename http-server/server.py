import asyncio
from datetime import datetime
from aiohttp import web
import aiofiles
from aiohttp.client_exceptions import ClientConnectionResetError

LOG_FILE = "logs.txt"
UDP_PORT = 9999
HTTP_PORT = 8080
HEARTBEAT_INTERVAL = 5  # seconds

# -------- UDP Server --------
class UDPServerProtocol:
    def __init__(self, clients):
        self.clients = clients

    def connection_made(self, transport):
        self.transport = transport
        print(f"UDP server listening on port {UDP_PORT}")

    def datagram_received(self, data, addr):
        message = data.decode().strip()
        print(f"Received from {addr}: {message}")
        asyncio.create_task(self.write_and_broadcast(message))

    async def write_and_broadcast(self, message):
        timestamp = datetime.now().isoformat()
        log_message = f"{timestamp} - {message}"
        # Write to file
        async with aiofiles.open(LOG_FILE, "a") as f:
            await f.write(log_message + "\n")

        # Broadcast to all connected HTTP clients
        for queue in self.clients:
            queue.put_nowait(log_message + "\n")

# -------- HTTP Server --------
async def tail_handler(request):
    # Create a queue for this client
    queue = asyncio.Queue()
    request.app['clients'].append(queue)
    response = web.StreamResponse(
        status=200,
        reason='OK',
        headers={'Content-Type': 'text/plain'}
    )
    await response.prepare(request)

    # Send existing content first
    try:
        try:
            async with aiofiles.open(LOG_FILE, "r") as f:
                async for line in f:
                    await response.write(line.encode())
        except FileNotFoundError:
            pass # log file doesn't exist yet

        # Send new log lines as they arrive
        while True:
            try:
                # Wait for new log or timeout for heartbeat
                line = await asyncio.wait_for(queue.get(), timeout=HEARTBEAT_INTERVAL)
            except asyncio.TimeoutError:
                line = "\n"  # Send heartbeat newline to keep connection open

            await response.write(line.encode())
    except (asyncio.CancelledError, ClientConnectionResetError):
        print("Client disconnected")
    finally:
        # Remove client from the list
        request.app['clients'].remove(queue)
    return response

async def index_handler(request):
    return web.FileResponse('./index.html')

async def start_servers():
    # Create log file if it doesn't exist
    async with aiofiles.open(LOG_FILE, "a"):
        pass

    # Shared list of clients
    clients = []

    # Start UDP server
    loop = asyncio.get_running_loop()
    transport, protocol = await loop.create_datagram_endpoint(
        lambda: UDPServerProtocol(clients),
        local_addr=('0.0.0.0', UDP_PORT)
    )

    # Start HTTP server
    app = web.Application()
    app['clients'] = clients
    app.router.add_get('/', index_handler)
    app.router.add_get('/logs', tail_handler)
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', HTTP_PORT)
    await site.start()

    print(f"HTTP server listening on port {HTTP_PORT}")

    # Keep running
    try:
        while True:
            await asyncio.sleep(3600)
    except KeyboardInterrupt:
        transport.close()

if __name__ == "__main__":
    asyncio.run(start_servers())
