#!/usr/bin/env bash
#
# Functional check: the vanilla build must not contain any part of the Third
# Strike Upper patch.
#
# The retail PS2 build has to keep byte matching THIRD_U.BIN. That build
# cannot be run here (it needs CodeWarrior and the retail binary), so
# tools/verify/ps2_touched_sources.sh checks it by comparing the
# preprocessed sources of the files the patch touches.
#
# This script checks the *host* build: it builds it with the patch off and
# looks for the patch in the result.
#
#   - the sources of the patch are not compiled at all
#   - the binary holds no symbol of the patch
#
# A hook of the patch that was left outside of its #ifdef would break the
# link, because the sources of the patch are not part of the build: the
# build of the script fails in that case.
#
# Two binaries of two commits are not compared any more. The port itself
# changes - it has its own scheduler, its own file system and its own
# progress markers - and every one of those changes moved the host build,
# so the comparison reported the work on the port as a failure of the
# patch.
#
# Usage: tools/verify/vanilla_unchanged.sh
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="$(mktemp -d)"

cleanup() {
    rm -rf "$WORK"
}
trap cleanup EXIT

cd "$REPO_ROOT"

echo "repo: $REPO_ROOT"

# --------------------------------------------------------------- the sources
echo
echo "== the sources of the patch are not compiled =="

make PLATFORM=linux BUILD_DIR="$WORK/build" print-o-files > "$WORK/o-files.txt"

if grep -q '/upper3s/' "$WORK/o-files.txt"; then
    echo "FAILED: the sources of the patch are part of the vanilla build" >&2
    grep '/upper3s/' "$WORK/o-files.txt" >&2
    exit 1
fi

echo "PASSED: the sources of the patch are not part of the vanilla build"

# ----------------------------------------------------------------- the build
echo
echo "== building the vanilla build =="

make PLATFORM=linux BUILD_DIR="$WORK/build" -j"$(nproc)" > "$WORK/build.log" 2>&1 || {
    echo "the vanilla build failed:" >&2
    tail -20 "$WORK/build.log" >&2
    exit 1
}

# ---------------------------------------------------------------- the symbols
echo
echo "== the binary holds no symbol of the patch =="

if nm "$WORK/build/sf33rd" 2>/dev/null | grep -q 'upper3s_'; then
    echo "FAILED: the vanilla build holds symbols of the patch" >&2
    nm "$WORK/build/sf33rd" | grep 'upper3s_' | head >&2
    exit 1
fi

echo "PASSED: the binary holds no symbol of the patch"

echo
echo "PASSED: the vanilla build is unchanged by the patch"
