#!/usr/bin/env python3
import socket
import os
import logging
from pathlib import Path
from typing import Dict

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s')
logger = logging.getLogger(__name__)

UDP_IP = "0.0.0.0"
UDP_PORT = 2342
ZONEINFO_PATH = Path("/usr/share/zoneinfo")

# Cache for TZ strings
tz_cache: Dict[str, str] = {}

def load_tz_cache() -> None:
    """Load all POSIX TZ strings from zoneinfo files."""
    zones = (ZONEINFO_PATH / "").rglob("*.tzf")
    for zone_path in zones:
        tz_name = str(zone_path.relative_to(ZONEINFO_PATH)).replace(os.sep, "/")
        try:
            with open(zone_path, "rb") as f:
                lines = f.readlines()
                if lines:
                    posix_tz = lines[-1].decode("ascii").strip("\n\r\0")
                    if posix_tz and not posix_tz.startswith("#"):
                        tz_cache[tz_name] = posix_tz
                        logger.info(f"Loaded {tz_name}: {posix_tz[:50]}...")
        except Exception as e:
            logger.warning(f"Failed to load {zone_path}: {e}")
    logger.info(f"Loaded {len(tz_cache)} timezones")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
logger.info(f"Timezone server listening on {UDP_IP}:{UDP_PORT}")

load_tz_cache()  # Load once at startup

while True:
    try:
        data, addr = sock.recvfrom(1024)
        tz_query = data.decode("utf-8").strip()
        logger.info(f"Request from {addr}: {tz_query}")
        
        if tz_query in tz_cache:
            response = tz_cache[tz_query].encode("ascii")
        else:
            response = b"ERROR"
            logger.warning(f"Unknown TZ: {tz_query}")
        
        sock.sendto(response, addr)
    except Exception as e:
        logger.error(f"Server error: {e}")
