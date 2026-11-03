#!/bin/bash
#
# memory_cleaner_v2.sh - remove every System V IPC resource that belongs to you.
#
# Why this is needed: shared memory segments, semaphore arrays and message
# queues are owned by the kernel, not by your process. If a program is killed
# before it reaches shmctl(IPC_RMID) / semctl(IPC_RMID) / msgctl(IPC_RMID) the
# resource outlives it, and the next run fails because the key already exists.
#
# Usage:
#   ./memory_cleaner_v2.sh              remove them
#   ./memory_cleaner_v2.sh --dry-run    list what would go, remove nothing
#
# Ownership is matched on your numeric UID, read from /proc/sysvipc, rather
# than on the owner name printed by `ipcs`. That column is truncated to ten
# characters, so a login such as "aaronjeriel.pica" never matches its own
# name - which is why an older copy of this script had to hard-code
# ME='aaronjerie' to work at all. A UID needs no such workaround and the
# script stays correct for every user on the machine.
#
# The uid column sits in a different position in each of the three files, so
# its index is looked up from the header line instead of being hard-coded.

set -u

DRY_RUN=0

case "${1:-}" in
    -n|--dry-run)
        DRY_RUN=1
        ;;
    -h|--help)
        # print this file's header block, whatever length it grows to
        awk 'NR == 1 && /^#!/ { next }
             /^#/ { sub(/^# ?/, ""); print; next }
             { exit }' "$0"
        exit 0
        ;;
    "")
        ;;
    *)
        printf 'unknown option: %s (try --help)\n' "$1" >&2
        exit 2
        ;;
esac

MY_UID=$(id -u)
MY_NAME=$(id -un)
removed=0

# ids_owned <proc-file> - print the id of every entry whose uid is ours.
ids_owned() {
    awk -v me="$MY_UID" '
        NR == 1 {
            for (i = 1; i <= NF; i++)
                if ($i == "uid") uidcol = i
            next
        }
        uidcol && $uidcol == me { print $2 }
    ' "$1"
}

# clean <proc-file> <ipcrm-flag> <label>
clean() {
    local file=$1 flag=$2 label=$3
    local ids id

    if [ ! -r "$file" ]; then
        printf 'cannot read %s - skipping %s\n' "$file" "$label" >&2
        return
    fi

    ids=$(ids_owned "$file")
    if [ -z "$ids" ]; then
        printf '  no %s\n' "$label"
        return
    fi

    for id in $ids; do
        if [ "$DRY_RUN" -eq 1 ]; then
            printf '  would remove %s %s\n' "$label" "$id"
            removed=$((removed + 1))
        elif ipcrm "$flag" "$id" 2>/dev/null; then
            printf '  removed %s %s\n' "$label" "$id"
            removed=$((removed + 1))
        else
            printf '  could not remove %s %s (already gone?)\n' "$label" "$id" >&2
        fi
    done
}

if [ ! -r /proc/sysvipc/shm ] && [ ! -r /proc/sysvipc/sem ] && [ ! -r /proc/sysvipc/msg ]; then
    printf '%s: /proc/sysvipc is not available, so IPC resources cannot be listed.\n' "${0##*/}" >&2
    printf 'This script needs Linux. Fall back to: ipcs -m ; ipcs -s ; ipcs -q\n' >&2
    exit 1
fi

if [ "$DRY_RUN" -eq 1 ]; then
    printf 'IPC resources owned by %s (uid %s) - dry run, nothing will be removed:\n' "$MY_NAME" "$MY_UID"
else
    printf 'IPC resources owned by %s (uid %s):\n' "$MY_NAME" "$MY_UID"
fi

clean /proc/sysvipc/shm -m 'shared memory segment'
clean /proc/sysvipc/sem -s 'semaphore array'
clean /proc/sysvipc/msg -q 'message queue'

if [ "$removed" -eq 0 ]; then
    printf 'Nothing to do.\n'
elif [ "$DRY_RUN" -eq 1 ]; then
    printf 'Done - %d resource(s) would be removed.\n' "$removed"
else
    printf 'Done - %d resource(s) removed.\n' "$removed"
fi
