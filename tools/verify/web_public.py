#!/usr/bin/env python3
"""Functional check: the game has to boot from the published site.

The deployment on GitHub Pages is the deliverable of the publication: a page
that loads the game and streams the disc image over HTTP range requests.  This
script opens the public address in a headless browser and watches the traffic
of the browser.

The oracle is the netlog of the browser: it holds every request of the page
with its address and the number of bytes it read.  That is the record of what
the site served, which is the question of this check.  The game reports its
own progress with a request to "/__trace?t=<marker>", and the page forwards
everything the game prints to the same place, so the markers of the boot are
in the same record.  The published site has no dev server, so those requests
answer 404; their address is what carries the marker.

The answers of the server are asked for directly, one request per file: the
netlog of this browser does not hold them.

The wait ends as soon as the game reports a frame, and not after a fixed
time: the first read of the disc image after a deployment is served by a
network that has to fetch the 612 megabytes from the origin first, so the
boot takes longer than the runs that come after it.

Usage:
    tools/verify/web_public.py [--url https://trafgals.github.io/3s-decomp/]
                                [--timeout 240] [--min-disc-bytes 4194304]
                                [--parse-only]
"""

from __future__ import annotations

import argparse
import json
import os
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from browser import find_browser, kill_tree, remove_profile, unique_profile  # noqa: E402

# The markers the game sends while it boots.  They have to appear in this
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

# The marker the game sends when its frame loop is running.  It reports the
# frame counter and the frame rate once a second.
FRAME_MARKER = "AcrMain: frame"

# The files of the site that the page needs before the game can start.
PAGE_FILES = ["sf33rd.js", "sf33rd.wasm", "sf33rd.data"]


def read_events(path: str) -> tuple[dict, list[dict]]:
    """The events of a netlog, even when the file was cut short.

    The browser writes the netlog while it runs, so a browser that is killed
    leaves a file without its end. The events are read one by one, and the
    reading stops at the last whole one.

    The kinds of the events are numbers, and the table that names them is in
    the constants of the file.
    """
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    start = text.find('"events"')
    if start < 0:
        raise ValueError("the netlog holds no events")

    constants = {}
    head = text[:start].rstrip().rstrip(",")
    if head.endswith("}"):
        try:
            constants = json.loads(head + "}").get("constants", {}).get("logEventTypes", {})
        except ValueError:
            constants = {}
    if isinstance(constants, dict):
        constants = {number: name for name, number in constants.items()}

    start = text.find("[", start)
    if start < 0:
        raise ValueError("the netlog holds no list of events")

    decoder = json.JSONDecoder()
    events: list[dict] = []
    position = start + 1
    while position < len(text):
        while position < len(text) and text[position] in " \t\r\n,":
            position += 1
        if position >= len(text) or text[position] == "]":
            break
        try:
            event, position = decoder.raw_decode(text, position)
        except ValueError:
            break
        if isinstance(event, dict):
            events.append(event)
    return constants, events


class Netlog:
    """The requests of the browser, read from its netlog."""

    def __init__(self, path: str) -> None:
        constants, self.events = read_events(path)

        # Every request has an id, and the byte counts of a request carry the
        # same id.
        self.url_of: dict[int, str] = {}
        self.method_of: dict[int, str] = {}
        self.bytes_of: dict[int, int] = {}

        for event in self.events:
            params = event.get("params") or {}
            source = event.get("source") or {}
            kind = self.kind_of(event, constants)
            request_id = source.get("id")

            if not isinstance(request_id, int):
                continue
            if kind == "URL_REQUEST_START_JOB" and params.get("url"):
                self.url_of[request_id] = params["url"]
                self.method_of[request_id] = params.get("method", "GET")
            elif kind in ("URL_REQUEST_JOB_BYTES_READ", "URL_REQUEST_JOB_FILTERED_BYTES_READ"):
                count = params.get("byte_count")
                if isinstance(count, int):
                    self.bytes_of[request_id] = self.bytes_of.get(request_id, 0) + count

    @staticmethod
    def kind_of(event: dict, constants) -> str:
        """The name of the kind of an event."""
        kind = event.get("type")
        if isinstance(kind, int):
            if isinstance(constants, dict):
                return constants.get(kind, "")
            if 0 <= kind < len(constants):
                return constants[kind] or ""
        return kind if isinstance(kind, str) else ""

    def requests(self, prefix: str) -> list[tuple[str, str, int]]:
        """Every request whose address starts with the prefix."""
        found = []
        for request_id, url in self.url_of.items():
            if not url.startswith(prefix):
                continue
            found.append((
                url,
                self.method_of.get(request_id, "GET"),
                self.bytes_of.get(request_id, 0),
            ))
        return found

    def every_request(self) -> list[tuple[str, str, int]]:
        """Every request of the browser."""
        found = []
        for request_id, url in self.url_of.items():
            found.append((
                url,
                self.method_of.get(request_id, "GET"),
                self.bytes_of.get(request_id, 0),
            ))
        return found


class NetlogTail:
    """The part of a netlog that was written since the last look at it."""

    def __init__(self, path: str) -> None:
        self.path = path
        self.position = 0
        self.overlap = ""

    def new_text(self) -> str:
        """The text that was added since the last call."""
        try:
            with open(self.path, "rb") as handle:
                handle.seek(self.position)
                data = handle.read()
                self.position = handle.tell()
        except OSError:
            return ""
        text = self.overlap + data.decode("utf-8", errors="replace")
        # A marker can be cut in two by the write of the browser, so the last
        # bytes of the last text are kept for the next one.
        self.overlap = text[-256:]
        return text


def http_status(url: str, byte_range: str | None = None) -> int:
    """The answer of the server to a request, without the body."""
    request = urllib.request.Request(url, method="GET")
    if byte_range:
        request.add_header("Range", byte_range)
    try:
        with urllib.request.urlopen(request, timeout=60) as answer:
            answer.read(1024)
            return answer.status
    except urllib.error.HTTPError as error:
        return error.code
    except OSError:
        return 0


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def close_browser(port: int) -> None:
    """Asks the browser to close itself, so that it writes its netlog out."""
    endpoint = f"http://127.0.0.1:{port}/json/version"
    deadline = time.time() + 15
    address = ""
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(endpoint, timeout=2) as answer:
                address = json.load(answer).get("webSocketDebuggerUrl", "")
            break
        except Exception:  # noqa: BLE001 - the browser may not be up yet
            time.sleep(0.5)
    if not address:
        return
    try:
        import websocket

        connection = websocket.create_connection(address, timeout=5, origin=f"http://127.0.0.1:{port}")
        connection.send(json.dumps({"id": 1, "method": "Browser.close"}))
        time.sleep(2)
        connection.close()
    except Exception as error:  # noqa: BLE001 - the browser is killed anyway
        print(f"the browser did not close itself ({error})")


def trace_markers(netlog: Netlog, host: str) -> list[str]:
    """The markers the game and the page sent to the trace endpoint.

    The game sends them to "/__trace" of the root of the site, so they are
    not under the path of the page.
    """
    markers = []
    for url, _method, _bytes in netlog.every_request():
        parsed = urllib.parse.urlparse(url)
        if not parsed.path.endswith("/__trace"):
            continue
        if host and parsed.netloc != host:
            continue
        for value in urllib.parse.parse_qs(parsed.query).get("t", []):
            markers.append(value)
    return markers


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="https://trafgals.github.io/3s-decomp/")
    parser.add_argument("--timeout", type=int, default=240)
    parser.add_argument("--min-disc-bytes", type=int, default=4 * 1024 * 1024)
    parser.add_argument("--net-log", default=os.path.join("build", "public_netlog.json"),
                        help="where the browser writes the log of its traffic")
    parser.add_argument("--parse-only", action="store_true",
                        help="read the netlog of an earlier run instead of opening the page")
    args = parser.parse_args()

    # The address of the page and the address of the files of the site are
    # two different things: the query of the page belongs to the page alone.
    parsed = urllib.parse.urlparse(args.url)
    path = parsed.path or "/"
    if not path.endswith("/"):
        path += "/"
    base = urllib.parse.urlunparse((parsed.scheme, parsed.netloc, path, "", "", ""))
    query = urllib.parse.parse_qs(parsed.query)
    # The trace level of the browser build is taken from the query string of
    # the page, and it is zero by default: every marker costs a request.
    query["trace"] = ["1"]
    page = urllib.parse.urlunparse((
        parsed.scheme,
        parsed.netloc,
        path,
        "",
        urllib.parse.urlencode(query, doseq=True),
        "",
    ))

    netlog_path = os.path.abspath(args.net_log)
    os.makedirs(os.path.dirname(netlog_path), exist_ok=True)

    if not args.parse_only:
        browser = find_browser()
        profile = unique_profile()
        port = free_port()
        if os.path.isfile(netlog_path):
            os.remove(netlog_path)

        command = [
            browser,
            "--headless=new",
            "--disable-gpu",
            "--no-sandbox",
            f"--user-data-dir={profile}",
            f"--log-net-log={netlog_path}",
            "--net-log-capture-mode=Default",
            f"--remote-debugging-port={port}",
            "--remote-allow-origins=*",
            "--enable-logging=stderr",
            page,
        ]

        print(f"loading {page} in {browser}")
        process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            # The wait ends when the game reports a frame, and not after a
            # fixed time.
            tail = NetlogTail(netlog_path)
            marker = urllib.parse.quote(FRAME_MARKER)
            deadline = time.time() + args.timeout
            while time.time() < deadline:
                if marker in tail.new_text():
                    print("the game reported a frame")
                    break
                time.sleep(2)
            else:
                print(f"the game reported no frame in {args.timeout} seconds")
        finally:
            # The browser is asked to close itself first: the netlog is
            # written while the browser runs, so a browser that is killed
            # leaves a file without its end.
            close_browser(port)
            kill_tree(process)
            remove_profile(profile)

        print(f"the netlog of the browser is in {netlog_path}")
    if not os.path.isfile(netlog_path):
        print("FAILED: the browser wrote no netlog, so the check cannot see the traffic")
        return 1

    try:
        netlog = Netlog(netlog_path)
    except (OSError, ValueError) as error:
        print(f"FAILED: the netlog of the browser cannot be read ({error})")
        return 1

    failures = []

    print("\nthe requests of the page:")
    for url, method, count in netlog.requests(base):
        if "/__trace" in url:
            continue
        print(f"  {method} {url} ({count} bytes)")

    # The answers of the server are asked for directly: the netlog of the
    # browser holds the addresses and the byte counts, not the answers.
    print("\nthe answers of the server:")
    for name in PAGE_FILES:
        status = http_status(base + name)
        print(f"  {name} -> {status}")
        if status != 200:
            failures.append(f"the site did not serve {name} with 200")
    # The game reads the disc image in ranges, so the site has to answer a
    # request with a range with the part of the file that was asked for.
    status = http_status(base + "rom/THIRD/SF33RD.AFS", "bytes=0-1023")
    print(f"  rom/THIRD/SF33RD.AFS (a range) -> {status}")
    if status != 206:
        failures.append(f"the site did not serve the disc image in ranges ({status})")

    disc = netlog.requests(base + "rom/THIRD/SF33RD.AFS")
    if not disc:
        failures.append("the game never asked for the disc image")
    else:
        total = sum(r[2] for r in disc)
        print(f"\nthe disc image was fetched in {len(disc)} requests, {total} bytes")
        if total < args.min_disc_bytes:
            failures.append(f"the game only read {total} bytes of the disc image")

    for name in PAGE_FILES:
        if not any(r[2] > 0 for r in netlog.requests(base + name)):
            failures.append(f"the page did not load {name}")

    markers = trace_markers(netlog, parsed.netloc)
    print(f"\n{len(markers)} markers received:")
    for marker_text in markers:
        print(f"  {marker_text}")

    # The game sends the markers with a synchronous request, so they come in
    # the order the game sends them in: a marker that comes before the one
    # that has to come first is a boot that went wrong.
    position = 0
    for expected in EXPECTED_TRACES:
        found = False
        for index in range(position, len(markers)):
            if markers[index] == expected:
                position = index + 1
                found = True
                break
        if not found:
            failures.append(f"missing marker: {expected}")

    if any(MISSING_DISC in marker_text for marker_text in markers):
        failures.append("the game reported the disc data as missing although the site has it")

    frames = [marker_text for marker_text in markers if FRAME_MARKER in marker_text]
    if not frames:
        failures.append("the game never reached its frame loop")
    else:
        print(f"\nthe game loop reported {len(frames)} times, the last one: {frames[-1]}")

    errors = [marker_text for marker_text in markers if marker_text.startswith("console: ")]
    if errors:
        print("\nthe game printed:")
        for line in errors[-20:]:
            print(f"  {line}")

    print()
    if failures:
        print("FAILED:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASSED: the published site serves the game and it boots from the disc image")
    return 0


if __name__ == "__main__":
    sys.exit(main())
