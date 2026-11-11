#!/usr/bin/env bash
#
# The verification gate of the port.
#
# Every check here is a functional check: it builds or runs something and
# looks at the result, instead of looking at the source.
#
#  1. the unit tests of the 3s' layer
#  2. the retail PS2 build is not touched by the patch
#  3. the host build is not touched by the patch
#  4. the host build runs the game at the full frame rate
#  5. the browser build boots and reads the disc image
#
# Usage: tools/verify/gate.sh [--skip-browser]
#
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

SKIP_BROWSER=0
for arg in "$@"; do
    case "$arg" in
    --skip-browser) SKIP_BROWSER=1 ;;
    *)
        echo "unknown option: $arg" >&2
        exit 2
        ;;
    esac
done

FAILED=()

step() {
    echo
    echo "=============================================================="
    echo "== $1"
    echo "=============================================================="
}

run() {
    local name="$1"
    shift

    step "$name"

    if "$@"; then
        echo "PASSED: $name"
    else
        echo "FAILED: $name"
        FAILED+=("$name")
    fi
}

# 1. The unit tests of the 3s' layer.
run "the unit tests" make BUILD_DIR=build/test test

# 2. The retail PS2 build keeps its sources. The check compares the
#    preprocessed sources of the files the 3s' layer touches with the ones
#    of the commit before the port was restored.
run "the retail PS2 sources are unchanged" bash tools/verify/ps2_touched_sources.sh

# 3. The host build does not change when the patch is applied: the patch
#    has to stay behind #ifdef UPPER_3S.
run "the vanilla host build is unchanged" bash tools/verify/vanilla_unchanged.sh

# 4. The host build runs the game.
run "the host build runs the game loop" bash -c '
    set -e
    make PLATFORM=linux UPPER_3S=1 BUILD_DIR=build/native -j"$(nproc)" >/dev/null
    DISPLAY=:0 timeout 40 build/native/sf33rd > build/native.log 2>&1 || true
    bash build/check_markers.sh build/native.log
'

# 5. The browser build boots and reads the disc image.
if [ "$SKIP_BROWSER" -eq 0 ]; then
    if python3 -c "
import sys
sys.path.insert(0, 'tools/verify')
from browser import find_browser
find_browser()
" >/dev/null 2>&1; then
        run "the browser build boots" python3 tools/verify/web_boot.py --real-time --timeout 150
    else
        echo
        echo "SKIPPED: the browser build - no headless browser on this machine."
        echo "         The browser build is checked on the machine that has one:"
        echo "         python3 tools/verify/web_boot.py --real-time --timeout 150"
        echo "         python3 tools/verify/web_shot.py --real-time --timeout 150"
    fi
else
    echo
    echo "SKIPPED: the browser build"
fi

echo
echo "=============================================================="
if [ ${#FAILED[@]} -eq 0 ]; then
    echo "== the gate passed"
    exit 0
fi

echo "== the gate failed:"
for name in "${FAILED[@]}"; do
    echo "   - $name"
done
exit 1
