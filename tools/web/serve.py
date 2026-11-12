#!/usr/bin/env python3
"""Development server for the web build.

The game streams the disc image over HTTP range requests, so the server has
to understand the Range header - Python's SimpleHTTPRequestHandler does not.
It also needs to serve .wasm, .data and .js with the right content types.

Usage:
    python3 tools/web/serve.py [--port 8000] [--root build/anniversary/web]
"""

import argparse
import os
import re
import sys
import urllib.parse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

RANGE_RE = re.compile(r"bytes=(\d*)-(\d*)")


class RangeRequestHandler(SimpleHTTPRequestHandler):
    """A static file handler that supports HTTP range requests."""

    # The disc image is far too big to live in the served directory, so the
    # /rom/ prefix is served from the repository instead.
    rom_root = None

    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".data": "application/octet-stream",
    }

    def translate_path(self, path):
        clean = path.split("?", 1)[0].split("#", 1)[0]

        if self.rom_root and (clean == "/rom" or clean.startswith("/rom/")):
            root = os.path.abspath(self.rom_root)
            target = os.path.abspath(os.path.join(root, clean[len("/rom"):].lstrip("/")))

            # Do not let a ../ in the request escape the rom directory.
            if target != root and not target.startswith(root + os.sep):
                return root

            return target

        return super().translate_path(path)

    def send_head(self):
        header = self.headers.get("Range")
        if not header:
            return super().send_head()

        match = RANGE_RE.fullmatch(header.strip())
        if match is None:
            self.send_error(416, "Invalid Range header")
            return None

        path = self.translate_path(self.path)
        if os.path.isdir(path):
            return super().send_head()

        try:
            handle = open(path, "rb")
        except OSError:
            self.send_error(404, "File not found")
            return None

        size = os.fstat(handle.fileno()).st_size
        first, last = match.group(1), match.group(2)

        if first == "" and last == "":
            handle.close()
            self.send_error(416, "Invalid Range header")
            return None

        if first == "":
            # Suffix range: the last N bytes.
            length = min(int(last), size)
            start = size - length
            end = size - 1
        else:
            start = int(first)
            end = int(last) if last else size - 1
            end = min(end, size - 1)

        if start > end or start >= size:
            handle.close()
            self.send_response(416)
            self.send_header("Content-Range", "bytes */%d" % size)
            self.end_headers()
            return None

        length = end - start + 1
        self.send_response(206)
        self.send_header("Content-Type", self.guess_type(path))
        self.send_header("Content-Range", "bytes %d-%d/%d" % (start, end, size))
        self.send_header("Content-Length", str(length))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()

        handle.seek(start)
        # Stream the body in chunks so that large files do not sit in memory.
        remaining = length
        while remaining > 0:
            chunk = handle.read(min(64 * 1024, remaining))
            if not chunk:
                break
            self.wfile.write(chunk)
            remaining -= len(chunk)

        handle.close()
        return None

    def end_headers(self):
        if not self.headers.get("Range"):
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    def do_POST(self):
        # Trace beacons from the web build arrive here.
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else b""

        if self.path.split("?", 1)[0] == "/__shot":
            self.save_shot(body)
            return

        self.send_response(204)
        self.end_headers()

    def save_shot(self, body):
        """Writes a screenshot sent by the build to a file of the root."""
        query = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
        name = (query.get("name") or ["web_shot.bmp"])[0]
        name = os.path.basename(name)

        if not body:
            self.send_error(400, "Empty screenshot")
            return

        target = os.path.join(self.directory, name)

        with open(target, "wb") as handle:
            handle.write(body)

        print(f"wrote {target} ({len(body)} bytes)")
        self.send_response(204)
        self.end_headers()

    def log_message(self, fmt, *args):
        # log_error() calls this with a status code as the first argument.
        if args and isinstance(args[0], str) and "Range" in args[0]:
            return
        super().log_message(fmt, *args)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--root", default="build/anniversary/web")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--rom", default="rom",
                        help="the directory the /rom/ prefix is served from")
    args = parser.parse_args()

    if not os.path.isdir(args.root):
        print(f"error: {args.root} does not exist; build the web target first", file=sys.stderr)
        return 1

    rom_root = os.path.abspath(args.rom) if os.path.isdir(args.rom) else None

    if rom_root is None:
        print("warning: no rom directory found; the disc image cannot be streamed", file=sys.stderr)

    handler = partial(RangeRequestHandler, directory=os.path.abspath(args.root))
    RangeRequestHandler.rom_root = rom_root
    server = ThreadingHTTPServer((args.host, args.port), handler)
    print(f"Serving {os.path.abspath(args.root)} on http://localhost:{args.port}/")
    if rom_root:
        print(f"The disc image is streamed from {rom_root} at /rom/.")
    print("The disc image is streamed from the same server; make sure it is reachable.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nstopping")
    return 0


if __name__ == "__main__":
    sys.exit(main())
