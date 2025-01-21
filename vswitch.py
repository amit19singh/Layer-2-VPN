import asyncio
import logging
from collections import OrderedDict
import struct
import argparse
import signal


# Logging Configuration
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(message)s")
logger = logging.getLogger("VSwitch")
handler = logging.FileHandler("vswitch.log")
formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(logging.INFO)

# MAC Table with Fixed Size
class MACCache:
    def __init__(self, max_size):
        self.cache = OrderedDict()
        self.max_size = max_size

    def get(self, key):
        if key in self.cache:
            self.cache.move_to_end(key)  # Mark as recently used
            return self.cache[key]
        return None

    def set(self, key, value):
        if key in self.cache:
            self.cache.move_to_end(key)
        self.cache[key] = value
        if len(self.cache) > self.max_size:
            self.cache.popitem(last=False)  # Evict least recently used


# Parse Ethernet Frame
def parse_frame(data):
    eth_dst, eth_src = struct.unpack("!6s6s", data[:12])
    eth_dst = ":".join(f"{byte:02x}" for byte in eth_dst)
    eth_src = ":".join(f"{byte:02x}" for byte in eth_src)
    return eth_src, eth_dst


# Handle a Batch of Frames
async def process_batch(loop, server_socket, mac_table, batch_size=10):
    frames = []
    for _ in range(batch_size):
        try:
            data, vport_addr = await loop.sock_recvfrom(server_socket, 1518)
            frames.append((data, vport_addr))
        except BlockingIOError:
            break  # No more frames to read

    for data, vport_addr in frames:
        eth_src, eth_dst = parse_frame(data)
        logger.debug(f"Received frame src<{eth_src}> dst<{eth_dst}> from {vport_addr}")

        # Update MAC Table
        mac_table.set(eth_src, vport_addr)

        # Frame Forwarding Logic
        if mac_table.get(eth_dst):
            await loop.sock_sendto(server_socket, data, mac_table.get(eth_dst))
            logger.debug(f"Forwarded to: {eth_dst}")
        elif eth_dst == "ff:ff:ff:ff:ff:ff":
            for dest_addr in mac_table.cache.values():
                if dest_addr != vport_addr:
                    await loop.sock_sendto(server_socket, data, dest_addr)
            logger.debug(f"Broadcasted to all except source: {vport_addr}")
        else:
            logger.debug("Frame discarded")

# Graceful Shutdown
async def shutdown(loop):
    logger.info("Shutting down VSwitch...")
    tasks = [t for t in asyncio.all_tasks() if t is not asyncio.current_task()]
    for task in tasks:
        task.cancel()
    await asyncio.gather(*tasks, return_exceptions=True)
    loop.stop()


def setup_signal_handlers(loop):
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, lambda: asyncio.create_task(shutdown(loop)))


# Main Server Loop
async def main(port, mac_table_size, batch_size):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind(("0.0.0.0", port))
    server_socket.setblocking(False)

    mac_table = MACCache(max_size=mac_table_size)

    logger.info(f"[VSwitch] Started at 0.0.0.0:{port}")
    loop = asyncio.get_event_loop()
    setup_signal_handlers(loop)

    while True:
        await process_batch(loop, server_socket, mac_table, batch_size)


if __name__ == "__main__":
    import socket

    parser = argparse.ArgumentParser(description="Virtual Switch (VSwitch)")
    parser.add_argument("port", type=int, help="Port to listen on")
    parser.add_argument("--mac-table-size", type=int, default=1000, help="Size of the MAC table")
    parser.add_argument("--batch-size", type=int, default=10, help="Batch size for frame processing")
    args = parser.parse_args()

    asyncio.run(main(args.port, args.mac_table_size, args.batch_size))


