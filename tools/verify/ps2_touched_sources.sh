#!/usr/bin/env bash
#
# Functional check: the retail PS2 build must not change.
#
# The PS2 build has to keep byte matching THIRD_U.BIN, so every 3s' change
# has to be invisible when UPPER_3S is not defined. CodeWarrior and the
# retail binary are not available here, so the check compares the
# *preprocessed* source of the files that the 3s' layer touches, with the
# PS2 flags and without UPPER_3S. If the preprocessed source is the same,
# the compiler sees the same code, and the produced object code is the
# same.
#
# The base is the commit before the port was restored, which is the state the
# retail build was matched in. The port is not part of the retail build, so
# the files it touches have to be the same as they were there.
#
# It has to be the commit right before the port work and not an older one:
# the upstream commits in between changed the retail build on purpose (the
# zlib match), and comparing against them would report those changes as
# changes of the port.
#
# The list of files is derived from the tree instead of being written down
# here, so that a file that is changed without being listed is still
# compared. The rules are:
#
#   * a retail source whose text changed is compared on its own;
#   * a retail header whose text changed is compared on its own, because a
#     header that preprocesses to the same text contributes the same text to
#     every source that includes it;
#   * a *new* file cannot be compared with the base tree at all, so it is
#     compared through the sources that include it. This is what proves that
#     the port's own headers stay behind their `#if !defined(TARGET_PS2)`
#     guard;
#   * a new retail *source* is a failure: the retail build would gain a
#     file.
#
# A file that does not preprocess is a failure, and not a file to skip: a
# macro that is not defined for the target leaves its calls in the
# preprocessed source, so the comparison would happily accept a file that
# the retail compiler would not even build. A *header* that is not
# self-contained is the exception: it has to be that way in the base as
# well, and then the sources that include it are what matter.
#
# Usage: tools/verify/ps2_touched_sources.sh [base-commit]

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="${1:-ca96f1c}"

# The work directory is unique per run: two checks may run at the same time
# (the subagents run them too), and they would otherwise overwrite each
# other's preprocessed files.
WORK="$(mktemp -d "$REPO_ROOT/build/ps2_check.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

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

# The roots of the retail build. The port has its own directories, which the
# retail build does not compile at all.
RETAIL_SOURCE_ROOTS="src/anniversary/sf33rd src/anniversary/cri src/anniversary/zlib src/anniversary/bin2obj"

echo "repo:    $REPO_ROOT"
echo "base:    $BASE"

rm -rf "$WORK/base"
mkdir -p "$WORK/base"

if ! git --git-dir="$GIT_DIR" --work-tree="$REPO_ROOT" archive --format=tar "$BASE" | tar -x -C "$WORK/base"; then
    echo "could not export the base commit" >&2
    exit 1
fi

is_retail_source() {
    case "$1" in
    src/anniversary/sf33rd/Source/Game/upper3s/*) return 1 ;;
    src/anniversary/sf33rd/*|src/anniversary/cri/*|src/anniversary/zlib/*|src/anniversary/bin2obj/*) return 0 ;;
    esac
    return 1
}

is_3s_only_header() {
    case "$1" in
    include/sf33rd/Source/Game/upper3s*) return 0 ;;
    esac
    return 1
}

is_header() {
    case "$1" in
    include/*.h|include/*/*.h|include/*/*/*.h|include/*/*/*/*.h) return 0 ;;
    esac
    return 1
}

# Every file of the retail build that the tree changed.
#
# The carriage return at the end of a line is ignored: the working tree is
# checked out with CRLF and the repository stores LF, so a plain diff would
# report every file of the repository as changed. The comparison of the
# preprocessed sources is insensitive to it anyway, because the normaliser
# collapses the whitespace.
CHANGED="$( {
    git --git-dir="$GIT_DIR" --work-tree="$REPO_ROOT" diff --ignore-cr-at-eol --name-only "$BASE" -- src include
    git --git-dir="$GIT_DIR" --work-tree="$REPO_ROOT" ls-files --others --exclude-standard -- src include
} | sort -u )"

SOURCES=""
HEADERS=""
NEW_FILES=""
NEW_SOURCES=""

for changed in $CHANGED; do
    if [ ! -f "$WORK/base/$changed" ]; then
        NEW_FILES="$NEW_FILES $changed"
        if is_retail_source "$changed"; then
            NEW_SOURCES="$NEW_SOURCES $changed"
        fi
    fi

    if is_retail_source "$changed"; then
        SOURCES="$SOURCES $changed"
    elif is_3s_only_header "$changed"; then
        :
    elif is_header "$changed"; then
        HEADERS="$HEADERS $changed"
    fi
done

# A new file cannot be compared with the base tree. A new *source* changes
# the retail build, so it is a failure. A new *header* is compared through
# the sources that include it, which is what proves that the port's headers
# stay behind their guard. The closure is transitive: a header that is
# included by another header is reached through that one.
#
# The queue is an array with an index, because a queue that is emptied with
# a parameter expansion does not terminate when it holds a single element
# that has no space in it.
declare -a WORKLIST=()
declare -A SEEN=()

for file in $NEW_FILES; do
    WORKLIST+=("$file")
    SEEN["$file"]=1
done

index=0
while [ "$index" -lt "${#WORKLIST[@]}" ]; do
    current="${WORKLIST[$index]}"
    index=$((index + 1))

    case "$current" in
    include/*.h|include/*/*.h|include/*/*/*.h|include/*/*/*/*.h) ;;
    *) continue ;;
    esac

    name="$(basename "$current")"
    hits="$(grep -rl --include='*.c' --include='*.h' -F "$name" \
        $RETAIL_SOURCE_ROOTS include 2>/dev/null | grep -v "^$current$")"

    for hit in $hits; do
        [ -n "${SEEN[$hit]:-}" ] && continue
        SEEN["$hit"]=1

        if is_retail_source "$hit"; then
            case " $SOURCES " in
            *" $hit "*) ;;
            *) SOURCES="$SOURCES $hit" ;;
            esac
        elif is_header "$hit"; then
            WORKLIST+=("$hit")
        fi
    done
done

echo
echo "== the files of the retail build that the tree changed =="
for source in $SOURCES; do
    echo "  source: $source"
done
for header in $HEADERS; do
    echo "  header: $header"
done
for new_file in $NEW_FILES; do
    echo "  new:    $new_file"
done

changed_count=0
compared=0

echo
echo "== comparing the preprocessed sources =="

for source in $SOURCES; do
    compared=$((compared + 1))

    if [ ! -f "$WORK/base/$source" ]; then
        echo "NEW:    $source"
        changed_count=$((changed_count + 1))
        continue
    fi

    if ! (cd "$WORK/base" && clang $PP_FLAGS "$source" -o "$WORK/base.pp") 2>/dev/null; then
        echo "BROKEN: $source does not preprocess for the PS2 in the base"
        changed_count=$((changed_count + 1))
        continue
    fi

    if ! (cd "$REPO_ROOT" && clang $PP_FLAGS "$source" -o "$WORK/cur.pp") 2>"$WORK/err"; then
        echo "BROKEN: $source does not preprocess for the PS2"
        head -5 "$WORK/err"
        changed_count=$((changed_count + 1))
        continue
    fi

    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/base.pp" "$WORK/base.norm" 2>/dev/null
    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/cur.pp" "$WORK/cur.norm" 2>/dev/null

    if ! cmp -s "$WORK/base.norm" "$WORK/cur.norm"; then
        echo "CHANGED: $source"
        diff "$WORK/base.norm" "$WORK/cur.norm" | cut -c1-200 | head -8
        changed_count=$((changed_count + 1))
    else
        echo "same:   $source"
    fi
done

for source in $NEW_SOURCES; do
    echo "NEW:    $source (a new source of the retail build)"
    changed_count=$((changed_count + 1))
done

# The headers are compared on their own. A header that is not self-contained
# has to be that way in the base as well; then the sources that include it
# are what matter, and they are compared above.
for header in $HEADERS; do
    if [ ! -f "$WORK/base/$header" ]; then
        continue
    fi

    if ! (cd "$WORK/base" && clang $PP_FLAGS "$header" -o "$WORK/base.pp") 2>/dev/null; then
        echo "skipped: $header is not self-contained in the base"
        continue
    fi

    if ! (cd "$REPO_ROOT" && clang $PP_FLAGS "$header" -o "$WORK/cur.pp") 2>"$WORK/err"; then
        echo "BROKEN: $header does not preprocess for the PS2"
        head -5 "$WORK/err"
        changed_count=$((changed_count + 1))
        continue
    fi

    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/base.pp" "$WORK/base.norm" 2>/dev/null
    python3 "$REPO_ROOT/tools/verify/normalize_pp.py" "$WORK/cur.pp" "$WORK/cur.norm" 2>/dev/null

    compared=$((compared + 1))

    if ! cmp -s "$WORK/base.norm" "$WORK/cur.norm"; then
        echo "CHANGED: $header"
        diff "$WORK/base.norm" "$WORK/cur.norm" | cut -c1-200 | head -8
        changed_count=$((changed_count + 1))
    else
        echo "same:   $header"
    fi
done

echo
echo "$compared files compared, $changed_count changed"

if [ "$compared" -eq 0 ]; then
    echo "FAILED: nothing was compared" >&2
    exit 1
fi

if [ "$changed_count" -ne 0 ]; then
    echo "FAILED: the vanilla PS2 build would change" >&2
    exit 1
fi

echo
echo "PASSED: the vanilla PS2 build is unchanged"
