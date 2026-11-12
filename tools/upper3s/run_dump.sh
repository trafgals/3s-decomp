#!/bin/bash
# Runs the disc dump until every character has been dumped.
#
# The memory of the game cannot hold the data of all twenty characters at
# once, so the dump is done over several runs: the tool picks up where the
# last run stopped.
#
# The dump tool is built with
#
#   make PLATFORM=linux UPPER_3S=1 UPPER3S_DUMP=1 BUILD_DIR=build/dump -j6
#
# and the dump it writes is `upper3s_dump.txt` in the root of the repository.
set -u

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "${root}" || exit 1

for run in $(seq 1 15); do
    count=0

    # The dump of an older version has a different layout, so its markers do
    # not count: the tool throws the file away and starts over.
    if [ "$(head -1 upper3s_dump.txt 2>/dev/null)" = "DUMP 7" ]; then
        count=$(grep -c "^END " upper3s_dump.txt 2>/dev/null || echo 0)
    fi

    echo "=== run ${run}: ${count} characters dumped ==="

    if [ "${count}" -ge 20 ]; then
        echo "the dump is complete"
        exit 0
    fi

    timeout 900 build/dump/sf33rd >> build/dump_run.log 2>&1
done

count=0

if [ "$(head -1 upper3s_dump.txt 2>/dev/null)" = "DUMP 7" ]; then
    count=$(grep -c "^END " upper3s_dump.txt 2>/dev/null || echo 0)
fi

echo "=== ${count} characters dumped ==="

if [ "${count}" -ge 20 ]; then
    echo "the dump is complete"
    exit 0
fi

echo "the dump is not complete"
exit 1
