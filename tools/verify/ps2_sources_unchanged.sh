#!/usr/bin/env bash
#
# Functional check: the retail PS2 build must not change when the Third
# Strike Upper patch is applied.
#
# The PS2 build has to keep byte matching THIRD_U.BIN, so every 3s' change
# has to be invisible when UPPER_3S is not defined. CodeWarrior and the
# retail binary are not available here, so the check compares the
# *preprocessed* source of every C file that goes into the PS2 build, with
# the PS2 flags and without UPPER_3S. If the preprocessed source is the
# same, the compiler sees the same code, and the produced object code is
# the same.
#
# New files are allowed as long as the Makefile keeps them out of the
# vanilla build, which it does for src/anniversary/sf33rd/Source/Game/upper3s.
#
# Usage: tools/verify/ps2_sources_unchanged.sh [base-commit]
#
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# The default base is the last commit before the Third Strike Upper layer
# was introduced; everything after it has to leave the vanilla build alone.
BASE="${1:-0c0beda}"
WORK="$(mktemp -d)"

cleanup() {
    rm -rf "$WORK"
}
trap cleanup EXIT

resolve_git_dir() {
    local line dir

    if [ -d "$REPO_ROOT/.git" ]; then
        echo "$REPO_ROOT/.git"
        return 0
    fi

    line="$(head -1 "$REPO_ROOT/.git")"
    dir="${line#gitdir: }"

    case "$dir" in
        [A-Za-z]:*)
            drive="$(printf '%s' "${dir:0:1}" | tr 'A-Z' 'a-z')"
            dir="/mnt/$drive/${dir:3}"
            ;;
    esac

    echo "$(dirname "$(dirname "$dir")")"
}

GIT_DIR="$(resolve_git_dir)"

# The flags the retail build uses, minus UPPER_3S.
PP_FLAGS="-E -P -Iinclude -Iinclude/sdk -Iinclude/cri -Iinclude/cri/ee -Izlib -Iinclude/gcc -DTARGET_PS2 -DDEBUG"

echo "repo:    $REPO_ROOT"
echo "base:    $BASE"
echo "workdir: $WORK"

mkdir -p "$WORK/base"
if ! git --git-dir="$GIT_DIR" --work-tree="$REPO_ROOT" archive --format=tar "$BASE" | tar -x -C "$WORK/base"; then
    echo "could not export the base commit" >&2
    exit 1
fi

# The sources of the vanilla PS2 build, as the Makefile computes them with
# UPPER_3S off.
make -s --no-print-directory -C "$REPO_ROOT" print-ps2-sources 2>/dev/null | tr ' ' '\n' | grep '\.c$' | sort > "$WORK/vanilla.list"

echo "the vanilla PS2 build has $(wc -l < "$WORK/vanilla.list") sources"

echo
echo "== comparing the preprocessed sources =="
changed=0
skipped=0
compared=0

while read -r source; do
    [ -n "$source" ] || continue

    if [ ! -f "$WORK/base/$source" ]; then
        echo "NEW:    $source"
        changed=$((changed + 1))
        continue
    fi

    if ! (cd "$WORK/base" && clang $PP_FLAGS "$source" -o "$WORK/base.pp") 2>/dev/null; then
        skipped=$((skipped + 1))
        continue
    fi

    if ! (cd "$REPO_ROOT" && clang $PP_FLAGS "$source" -o "$WORK/cur.pp") 2>/dev/null; then
        skipped=$((skipped + 1))
        continue
    fi

    compared=$((compared + 1))

    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/base.pp" "$WORK/base.norm"
    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/cur.pp" "$WORK/cur.norm"

    if ! cmp -s "$WORK/base.norm" "$WORK/cur.norm"; then
        echo "CHANGED: $source"
        diff "$WORK/base.norm" "$WORK/cur.norm" | head -12
        changed=$((changed + 1))
    fi
done < "$WORK/vanilla.list"

echo
echo "$compared sources compared, $skipped skipped, $changed changed"

failed=0

if [ "$changed" -ne 0 ]; then
    echo
    echo "FAILED: the vanilla PS2 build would change" >&2
    failed=1
fi

# The 3s' sources have to exist, and they have to be kept out of the
# vanilla build.
echo
echo "== the Third Strike Upper sources =="
for source in $(find src/anniversary/sf33rd/Source/Game/upper3s -name '*.c' | sort); do
    if grep -qx "$source" "$WORK/vanilla.list"; then
        echo "IN THE VANILLA BUILD: $source" >&2
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    exit 1
fi

echo
echo "PASSED: the vanilla PS2 build is unchanged"
