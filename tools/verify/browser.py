#!/usr/bin/env python3
"""Helpers for driving the headless browser of the web build.

Two details matter here and both of them have bitten us before:

*   The browser has to run with a profile of its own. A fixed profile
    directory is shared by every run, and a browser that is still alive
    holds a lock on it: the next launch hands its address to the old
    browser and quits, so the page under test is never loaded and the run
    reports nonsense.
*   The browser has to be killed with its whole process tree. The browser
    process spawns helpers that outlive it, and those helpers keep the
    profile locked for the next run.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile

EDGE_CANDIDATES = [
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
    shutil.which("msedge") or "",
    shutil.which("chrome") or "",
]


def find_browser() -> str:
    """Returns the path of the headless browser to use."""
    for candidate in EDGE_CANDIDATES:
        if candidate and os.path.exists(candidate):
            return candidate
    raise SystemExit("no headless browser found (looked for Edge and Chrome)")


def unique_profile() -> str:
    """Returns a profile directory that no other run is using."""
    return tempfile.mkdtemp(prefix="sf33rd-browser-")


def remove_profile(profile: str) -> None:
    """Deletes a profile directory, ignoring whatever is left in it."""
    shutil.rmtree(profile, ignore_errors=True)


def kill_tree(process: subprocess.Popen) -> None:
    """Kills the browser and every process it spawned."""
    if process.poll() is not None:
        return

    if os.name == "nt":
        subprocess.run(
            ["taskkill", "/T", "/F", "/PID", str(process.pid)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    else:
        try:
            os.killpg(os.getpgid(process.pid), 9)
        except (ProcessLookupError, PermissionError):
            process.kill()

    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
