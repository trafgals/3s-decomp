#!/usr/bin/env python3
"""Check that changes to C sources are comment-only, as the compiler sees them.

For every .c/.h file that differs from a base commit, preprocess the base
version and the working-tree version the way the PS2 build does (the
Makefile's include paths, -DTARGET_PS2 -DDEBUG) and compare the C tokens.
Equal tokens mean the compiler is given the same program, so the matching
build cannot change. This needs no THIRD_U.BIN and no CodeWarrior, so it
can run anywhere gcc is installed.

Comments and whitespace are the only edits this lets through. Anything that
reaches the token stream fails, including __LINE__ (none of the sources use
it today, but inserting a comment line above a use would shift its value).

Usage: python3 tools/check_comments_only.py [--base REF]   (default: HEAD)
Exits 0 when every changed file is comment-only, 1 otherwise.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# The PS2 build's preprocessor view (Makefile: PS2_INCLUDES and PS2_DEFINES).
INCLUDES = ["include", "include/sdk", "include/cri", "include/cri/ee", "zlib", "include/gcc"]
DEFINES = ["TARGET_PS2", "DEBUG"]

# C tokens, independent of the whitespace between them: string and char literals
# first (so a "//" inside a string stays a string), then identifiers, numbers and
# single punctuation characters. Multi-character operators come out as several
# single characters, which compares just as well.
TOKEN = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|[A-Za-z_]\w*|\.?\d[\w.]*|\S')


def git(*args, cwd=ROOT):
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


def tokens(tree, rel):
    """Preprocess tree/rel with the build's flags and return its C tokens."""
    cmd = [os.environ.get("CC", "gcc"), "-E", "-P", "-x", "c"]
    cmd += [f"-I{tree / inc}" for inc in INCLUDES] + [f"-D{d}" for d in DEFINES]
    out = subprocess.run(cmd + [str(tree / rel)], capture_output=True, text=True)
    if out.returncode != 0:
        raise RuntimeError(f"preprocessing {rel} failed:\n{out.stderr.strip()}")
    return TOKEN.findall(out.stdout)


def first_difference(a, b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i
    return min(len(a), len(b))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--base", default="HEAD", help="commit to compare the working tree against (default: HEAD)")
    args = ap.parse_args()

    # Everything that differs from the base, committed or not, plus new untracked sources.
    changed = [l for l in git("diff", "--name-only", args.base, "--", "*.c", "*.h").splitlines() if l]
    untracked = [l for l in git("ls-files", "--others", "--exclude-standard", "--", "*.c", "*.h").splitlines() if l]
    failures = [f"{rel}: new file (a new source changes the build)" for rel in untracked]

    if not changed and not untracked:
        print(f"no C sources differ from {args.base}")
        return 0

    # The base version is preprocessed in a worktree of its own, so it sees the
    # base versions of the headers it includes.
    with tempfile.TemporaryDirectory(prefix="comments-only-") as tmp:
        base_tree = Path(tmp) / "base"
        git("worktree", "add", "--detach", str(base_tree), args.base)
        try:
            for rel in changed:
                if not (ROOT / rel).exists():
                    failures.append(f"{rel}: deleted")
                    continue
                if not (base_tree / rel).exists():
                    failures.append(f"{rel}: new file (a new source changes the build)")
                    continue
                before, after = tokens(base_tree, rel), tokens(ROOT, rel)
                if before == after:
                    print(f"  same tokens  {rel} ({len(after)} tokens)")
                    continue
                i = first_difference(before, after)
                failures.append(f"{rel}: token {i} differs: {' '.join(before[max(0, i - 5):i + 5])!r} -> "
                                f"{' '.join(after[max(0, i - 5):i + 5])!r}")
        finally:
            git("worktree", "remove", "--force", str(base_tree))

    for f in failures:
        print(f"  NOT COMMENT-ONLY  {f}")
    print(f"{len(changed) + len(untracked) - len(failures)} of {len(changed) + len(untracked)} changed sources "
          f"are comment-only against {args.base}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
