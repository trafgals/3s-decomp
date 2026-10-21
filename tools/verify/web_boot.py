#!/usr/bin/env python3
"""Functional check: the browser build has to boot.

The browser build is the deliverable, so this script starts the dev server,
loads the page in a headless browser and checks that the game actually
reaches its initialisation - and that it fails *gracefully* when the disc
data is missing, instead of hanging the tab.

The progress markers are sent by the game itself (port/web/web_trace.c) to
the dev server, which logs them as ordinary requests.

Usage:
    tools/verify/web_boot.py [--build-dir build/anniversary/web] [--timeout 60]
"""

from __future__ import annotations

import argparse
import http.server
import os
import re
import shutil
import socket
import subprocess
import sys
import threading
import time
import urllib.parse

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# The dev server knows how to answer the range requests the game uses to
# stream the disc image; the check has to use the same handler.
sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "web"))
from serve import RangeRequestHandler  # noqa: E402

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "verify"))
from browser import find_browser, kill_tree, remove_profile, unique_profile  # noqa: E402

# The markers the game sends while it boots. They have to appear in this
# order: the game cannot get to the later ones without the earlier ones.
EXPECTED_TRACES = [
    "main entry",
    "SDLApp_Init: before SDL_Init",
    "SDLApp_Init: after SDL_Init",
    "SDLApp_Init: after window/renderer",
    "AcrMain: after SDLApp_Init",
    "flInitialize: start",
    "flInitialize: after system_work_init",
    "flInitialize: after system_hard_init",
    "flInitialize: after VramFullClear",
    "flInitialize: after InitRenderBuff",
    "flInitialize: after SwapDBuff",
    "flInitialize: after PADInitialize",
]

# The marker the game sends when it cannot read the disc image.
MISSING_DISC = "could not load the debug font"

# The marker the game sends when its frame loop is running. It reports the
# frame counter and the frame rate once a second.
FRAME_MARKER = "AcrMain: frame"


class TraceHandler(RangeRequestHandler):
    """Serves the build and records the progress markers."""

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
        # The disc traffic is the interesting part: the game streams the
        # archive with range requests, so the paths tell how far it got.
        TraceHandler.requests.append(self.path)
        super().do_GET()

    def log_message(self, fmt, *args):  # noqa: A003 - the name is fixed
        pass


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=os.path.join(REPO_ROOT, "build", "anniversary", "web"))
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument("--real-time", action="store_true",
                        help="run the browser in real time instead of with a virtual time budget; "
                             "the virtual clock deadlocks the synchronous range requests of the disc reader")
    args = parser.parse_args()

    if not os.path.isfile(os.path.join(args.build_dir, "sf33rd.wasm")):
        raise SystemExit(f"no web build in {args.build_dir}; run `make PLATFORM=web` first")

    port = free_port()
    handler = type("Handler", (TraceHandler,), {"root": os.path.abspath(args.build_dir)})
    rom_root = os.path.join(REPO_ROOT, "rom")
    if os.path.isfile(os.path.join(rom_root, "THIRD", "SF33RD.AFS")):
        RangeRequestHandler.rom_root = os.path.abspath(rom_root)
        has_disc = True
    else:
        RangeRequestHandler.rom_root = None
        has_disc = False
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print(f"serving {args.build_dir} on http://127.0.0.1:{port}")
    print("the disc image is available" if has_disc else "no disc image: the game has to report it")

    browser = find_browser()
    profile = unique_profile()
    # The trace level of the browser build is taken from the query string of
    # the page, and it is zero by default: every marker costs a synchronous
    # request to the dev server.
    url = f"http://127.0.0.1:{port}/index.html?trace=1"

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
        # The page is simply left open: the game reports its progress with
        # beacons, so there is no need to make the browser dump anything.
        command.append(url)
        process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                   text=True, errors="replace")
        time.sleep(args.timeout)
        kill_tree(process)
        try:
            _, browser_log = process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            browser_log = ""
        dom = ""
        console = [line for line in (browser_log or "").splitlines() if "CONSOLE" in line]
        if console:
            print("\nthe browser reported:")
            for line in console:
                print(f"  {line}")
    else:
        command += [f"--virtual-time-budget={args.timeout * 1000}", "--dump-dom", url]
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            errors="replace",
        )

        dom = ""
        try:
            out, _ = process.communicate(timeout=args.timeout + 30)
            dom = out or ""
        except subprocess.TimeoutExpired:
            kill_tree(process)
            dom = ""

    # The last beacon is sent right before the game aborts; give the
    # browser a moment to flush it.
    time.sleep(2)

    server.shutdown()
    remove_profile(profile)

    failures = []

    seen = TraceHandler.traces
    print(f"\n{len(seen)} progress markers received:")
    for trace in seen:
        print(f"  {trace}")

    disc_requests = [path for path in TraceHandler.requests if "rom/" in path]
    print(f"\n{len(TraceHandler.requests)} requests, {len(disc_requests)} of them for the disc:")
    for path in TraceHandler.requests[:40]:
        print(f"  {path}")
    if len(TraceHandler.requests) > 40:
        print(f"  ... and {len(TraceHandler.requests) - 40} more")

    position = 0
    for expected in EXPECTED_TRACES:
        found = False
        for index in range(position, len(seen)):
            if seen[index] == expected:
                position = index + 1
                found = True
                break
        if not found:
            # The markers are sent as beacons, which are not ordered, so a
            # marker that is out of order still counts as seen.
            if expected not in seen:
                failures.append(f"missing marker: {expected}")

    if has_disc:
        # The disc data is there, so the game must not report it as missing.
        if any(MISSING_DISC in trace for trace in seen):
            failures.append("the game reported the disc data as missing although it is there")
    else:
        # The disc data is not part of the repository, so the game has to
        # report the missing files rather than spin in the fatal-error trap.
        if not any(MISSING_DISC in trace for trace in seen):
            failures.append("the game did not report the missing disc files")

    # The game loop reports itself once a second. Without it the game boots
    # but never reaches the loop, which is not a playable build.
    if has_disc:
        loop_frames = [trace for trace in seen if FRAME_MARKER in trace]
        if not loop_frames:
            failures.append("the game never reached its frame loop")
        else:
            print(f"\nthe game loop reported {len(loop_frames)} times, the last one: {loop_frames[-1]}")

    print()
    if failures:
        print("FAILED:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    if has_disc:
        print("PASSED: the browser build boots and reads the disc image")
    else:
        print("PASSED: the browser build boots and reports the missing disc data")
    return 0


if __name__ == "__main__":
    sys.exit(main())
