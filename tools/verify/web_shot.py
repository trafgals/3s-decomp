#!/usr/bin/env python3
"""Functional check: the browser build has to render the game.

The browser build blocks the main thread while the game is running, so a
screenshot cannot be taken from the JavaScript side: the page is not able to
read the canvas back. The build therefore takes the screenshot itself and
sends it to the dev server, which writes it to a file. This script starts the
dev server, loads the page in a headless browser and checks that a frame of
the running game arrives - and that it is not an empty picture.

Usage:
    tools/verify/web_shot.py [--build-dir build/anniversary/web] [--timeout 120]
"""

from __future__ import annotations

import argparse
import http.server
import os
import struct
import subprocess
import sys
import threading
import time
import urllib.parse

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "web"))
from serve import RangeRequestHandler  # noqa: E402

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "verify"))
from browser import find_browser, kill_tree, remove_profile, unique_profile  # noqa: E402


def read_bmp(path: str) -> tuple[int, int, bytes]:
    """Returns the size of the bitmap and its pixels."""
    with open(path, "rb") as handle:
        data = handle.read()

    if data[:2] != b"BM":
        raise ValueError("not a bitmap")

    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]

    if bpp not in (24, 32):
        raise ValueError(f"unexpected bit depth: {bpp}")

    return width, height, data[pixel_offset:]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=os.path.join(REPO_ROOT, "build", "anniversary", "web"))
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--output", default="web_shot.bmp")
    parser.add_argument("--real-time", action="store_true",
                        help="run the browser in real time instead of with a virtual time budget; "
                             "the virtual clock deadlocks the synchronous range requests of the disc reader")
    args = parser.parse_args()

    if not os.path.isfile(os.path.join(args.build_dir, "sf33rd.wasm")):
        raise SystemExit(f"no web build in {args.build_dir}; run `make PLATFORM=web WEB_SHOT=1` first")

    target = os.path.join(args.build_dir, args.output)
    if os.path.exists(target):
        os.remove(target)

    rom_root = os.path.join(REPO_ROOT, "rom")
    if os.path.isfile(os.path.join(rom_root, "THIRD", "SF33RD.AFS")):
        RangeRequestHandler.rom_root = os.path.abspath(rom_root)
        print("the disc image is available")
    else:
        RangeRequestHandler.rom_root = None
        print("warning: no disc image; the game cannot reach the game screen")

    import socket

    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]

    root = os.path.abspath(args.build_dir)

    class ShotHandler(RangeRequestHandler):
        def __init__(self, *a, **kw):
            super().__init__(*a, directory=root, **kw)

        def log_message(self, fmt, *a):  # noqa: A003 - the name is fixed
            pass

    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), ShotHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print(f"serving {root} on http://127.0.0.1:{port}")

    browser = find_browser()
    profile = unique_profile()
    url = f"http://127.0.0.1:{port}/index.html"

    print(f"loading {url} in {browser}")
    command = [
        browser,
        "--headless=new",
        "--disable-gpu",
        "--no-sandbox",
        f"--user-data-dir={profile}",
        "--enable-logging=stderr",
    ]

    if args.real_time:
        command.append(url)
    else:
        command += [f"--virtual-time-budget={args.timeout * 1000}", "--dump-dom", url]

    process = subprocess.Popen(
        command,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
        errors="replace",
    )

    deadline = time.time() + args.timeout
    while time.time() < deadline:
        if os.path.isfile(target) and os.path.getsize(target) > 0:
            # The upload may still be in flight; give it a moment to finish.
            time.sleep(2)
            break
        if process.poll() is not None:
            break
        time.sleep(1)

    kill_tree(process)
    server.shutdown()
    remove_profile(profile)

    try:
        _, browser_log = process.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        browser_log = ""

    console = [line for line in (browser_log or "").splitlines() if "CONSOLE" in line]
    if console:
        print("\nthe browser reported:")
        for line in console:
            print(f"  {line}")

    if not os.path.isfile(target) or os.path.getsize(target) == 0:
        print("FAILED: the build did not send a screenshot")
        return 1

    size = os.path.getsize(target)
    try:
        width, height, pixels = read_bmp(target)
    except ValueError as error:
        print(f"FAILED: the screenshot is not a usable bitmap: {error}")
        return 1

    # A frame of the game is not a single flat colour: the check looks for
    # more than one distinct value, so that a black or blank screen fails.
    distinct = len(set(pixels[i:i + 3] for i in range(0, min(len(pixels), 3 * 4096), 3)))

    print(f"the build sent {size} bytes: a {width}x{height} bitmap, {distinct} distinct colours")

    if width < 100 or height < 100:
        print("FAILED: the screenshot is too small to be a frame of the game")
        return 1

    if distinct < 2:
        print("FAILED: the screenshot is a flat colour, the game did not render anything")
        return 1

    print(f"PASSED: the browser build rendered a frame ({target})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
