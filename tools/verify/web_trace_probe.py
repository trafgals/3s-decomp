#!/usr/bin/env python3
"""Collects the progress markers of the browser build and summarises them.

The browser build reports its progress with the markers of the trace layer,
which the dev server receives as ordinary requests. The full log of a boot is
far too long to read, so this script prints a summary: how many markers of
each kind arrived, the first ones in order, and the last ones.

Usage:
    tools/verify/web_trace_probe.py [--seconds 60] [--trace 2] [--show 40]
"""

from __future__ import annotations

import argparse
import http.server
import os
import re
import socket
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

# The markers of the scheduler start with this.
THREAD_MARKER = re.compile(r"^thread: (\w+)")


class TraceHandler(RangeRequestHandler):
    traces: list[str] = []
    requests: list[str] = []
    root: str = ""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=self.root, **kwargs)

    def do_POST(self):  # noqa: N802 - the name is fixed by http.server
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/__trace":
            query = urllib.parse.parse_qs(parsed.query)
            for value in query.get("t", []):
                TraceHandler.traces.append(value)
        self.send_response(204)
        self.end_headers()

    def do_GET(self):  # noqa: N802 - the name is fixed by http.server
        TraceHandler.requests.append(self.path)
        super().do_GET()

    def log_message(self, fmt, *args):  # noqa: A003 - the name is fixed
        pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=os.path.join(REPO_ROOT, "build", "anniversary", "web"))
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--trace", type=int, default=2)
    parser.add_argument("--show", type=int, default=40)
    args = parser.parse_args()

    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]

    root = os.path.abspath(args.build_dir)
    handler = type("Handler", (TraceHandler,), {"root": root})

    rom_root = os.path.join(REPO_ROOT, "rom")
    if os.path.isfile(os.path.join(rom_root, "THIRD", "SF33RD.AFS")):
        RangeRequestHandler.rom_root = os.path.abspath(rom_root)
    else:
        RangeRequestHandler.rom_root = None

    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()

    browser = find_browser()
    profile = unique_profile()
    url = f"http://127.0.0.1:{port}/index.html?trace={args.trace}"

    print(f"serving {root}")
    print(f"loading {url}")

    process = subprocess.Popen(
        [browser, "--headless=new", "--disable-gpu", "--no-sandbox",
         f"--user-data-dir={profile}", "--enable-logging=stderr", url],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, errors="replace")

    time.sleep(args.seconds)
    kill_tree(process)
    try:
        _, browser_log = process.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        browser_log = ""

    time.sleep(1)
    server.shutdown()
    remove_profile(profile)

    seen = TraceHandler.traces
    print(f"\n{len(seen)} markers received")

    counts: dict[str, int] = {}
    for trace in seen:
        match = THREAD_MARKER.match(trace)
        key = f"thread: {match.group(1)}" if match else trace
        counts[key] = counts.get(key, 0) + 1

    print("\nthe markers and how often they arrived:")
    for key, count in sorted(counts.items(), key=lambda item: -item[1]):
        print(f"  {count:6d}  {key}")

    print(f"\nthe first {min(args.show, len(seen))} markers:")
    for trace in seen[:args.show]:
        print(f"  {trace}")

    print(f"\nthe last {min(args.show, len(seen))} markers:")
    for trace in seen[-args.show:]:
        print(f"  {trace}")

    disc = [path for path in TraceHandler.requests if "rom/" in path]
    print(f"\n{len(TraceHandler.requests)} requests, {len(disc)} of them for the disc")
    for path in TraceHandler.requests[:20]:
        print(f"  {path}")
    if len(TraceHandler.requests) > 20:
        print(f"  ... and {len(TraceHandler.requests) - 20} more")

    console = [line for line in (browser_log or "").splitlines() if "CONSOLE" in line]
    if console:
        print("\nthe browser reported:")
        for line in console[:30]:
            print(f"  {line}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
