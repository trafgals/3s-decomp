"""Shared setup of the fight tests (see tests/match/README.md)."""

import os
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools/match"))

import fight as F  # noqa: E402


def skip_without_game():
    """The tests need the disc's files and both headless builds."""
    path = os.environ.get("SF33_GAME_DATA")
    if not path or not (Path(path) / "rom/THIRD/SF33RD.AFS").is_file():
        raise unittest.SkipTest("SF33_GAME_DATA does not point at the game data")
    for build, binary in F.BINARIES.items():
        if not binary.is_file():
            raise unittest.SkipTest(f"the {build} headless build is missing: make match-build")


def both(script, **kw):
    """The same fight in the vanilla and in the 3s' build."""
    return F.run(script, build="vanilla", **kw), F.run(script, build="upper3s", **kw)
