#!/usr/bin/env python3
"""Runs a page of the build in a headless browser and prints its result.

Usage:
    tools/verify/web_page.py [--build-dir build/anniversary/web] [--page rangetest.html]
"""

from __future__ import annotations

import argparse
import http.server
import os
import socket
import subprocess
import sys
import threading
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "web"))
from serve import RangeRequestHandler  # noqa: E402

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "verify"))
from browser import find_browser, kill_tree, remove_profile, unique_profile  # noqa: E402

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=os.path.join(REPO_ROOT, "build", "anniversary", "web"))
    parser.add_argument("--page", default="rangetest.html")
    parser.add_argument("--timeout", type=int, default=90)
    args = parser.parse_args()

    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]

    root = os.path.abspath(args.build_dir)
    rom_root = os.path.join(REPO_ROOT, "rom")
    RangeRequestHandler.rom_root = os.path.abspath(rom_root) if os.path.isdir(rom_root) else None

    class PageHandler(RangeRequestHandler):
        def __init__(self, *a, **kw):
            super().__init__(*a, directory=root, **kw)

    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), PageHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()

    browser = find_browser()
    profile = unique_profile()
    url = f"http://127.0.0.1:{port}/{args.page}"

    print(f"loading {url}")
    process = subprocess.Popen(
        [browser, "--headless=new", "--disable-gpu", "--no-sandbox",
         f"--user-data-dir={profile}", "--enable-logging=stderr",
         f"--virtual-time-budget={args.timeout * 1000}", "--dump-dom", url],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, errors="replace")

    try:
        out, err = process.communicate(timeout=args.timeout + 30)
    except subprocess.TimeoutExpired:
        kill_tree(process)
        out, err = process.communicate()

    server.shutdown()
    remove_profile(profile)

    print("\n--- the page ---")
    for line in (out or "").splitlines():
        if "results" in line or line.strip().startswith("plain GET") or "GET with" in line:
            print(line.strip())

    console = [line for line in (err or "").splitlines() if "CONSOLE" in line]
    if console:
        print("\n--- the console ---")
        for line in console[:30]:
            print(line)

    return 0


if __name__ == "__main__":
    sys.exit(main())
