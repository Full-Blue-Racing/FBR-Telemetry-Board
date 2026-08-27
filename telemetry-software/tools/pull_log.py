#!/usr/bin/env python3
"""Pulls a log file off the board's flash storage via the `get` console command.

Usage:
    python tools/pull_log.py --port /dev/tty.usbmodemXXXX --file datalog_003.txt --out ./datalog_003.txt
"""
import argparse
import sys
import time

import serial

BEGIN_MARKER = b"-----BEGIN-FILE-----"
END_MARKER = b"-----END-FILE-----"

# Opening the port can reset the board (DTR gets asserted on open by default on
# many OS/driver combos, which some ESP32 boards wire to trigger a reset). Give
# it time to finish rebooting and drain the boot log before sending anything,
# so `get` isn't sent into a device that isn't listening yet.
BOOT_SETTLE_S = 2.0


def pull_file(port: str, baud: int, remote_name: str, out_path: str, timeout: float) -> None:
    with serial.Serial(port, baud, timeout=timeout) as ser:
        time.sleep(BOOT_SETTLE_S)
        ser.reset_input_buffer()
        ser.write(f"get {remote_name}\r".encode())  # device console expects CR as line end

        # Wait for the begin marker, ignoring the echoed command / any log noise before it.
        line = ser.read_until(b"\n")
        while line and BEGIN_MARKER not in line:
            line = ser.read_until(b"\n")
        if not line:
            sys.exit(f"error: never saw {BEGIN_MARKER.decode()} - is '{remote_name}' a real file? (try `ls`)")

        data = bytearray()
        while True:
            chunk = ser.read_until(END_MARKER)
            if END_MARKER in chunk:
                data += chunk[: chunk.index(END_MARKER)]
                break
            if not chunk:
                sys.exit("error: timed out waiting for end marker")
            data += chunk

        # Drop the single trailing newline cmd_get adds before the end marker.
        if data.endswith(b"\n"):
            data = data[:-1]

        with open(out_path, "wb") as f:
            f.write(data)
        print(f"wrote {len(data)} bytes to {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Serial port, e.g. /dev/tty.usbmodemXXXX")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--file", required=True, help="Remote filename on the board, e.g. datalog_003.txt")
    parser.add_argument("--out", required=True, help="Local path to write the file to")
    parser.add_argument("--timeout", type=float, default=5.0, help="Per-read timeout in seconds")
    args = parser.parse_args()

    pull_file(args.port, args.baud, args.file, args.out, args.timeout)


if __name__ == "__main__":
    main()
