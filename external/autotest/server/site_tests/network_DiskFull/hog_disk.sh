#!/bin/sh

set -e

MAX_TIMEOUT_SECONDS=300

usage() {
    echo "$0 <mount point> <timeout seconds>"
    exit 1
}

# Get the size of the filesystem mounted at $1, in bytes.
get_mount_size_bytes() {
    local mount_point="$1"

    # Filesystem              1024-blocks  Used Available Capacity Mounted on
    # /dev/mapper/encstateful      290968 47492    243476      17% /var
    #
    # awk uses double-representation internally; we'll hit problems if
    # the filesystem has more than 2^53 bytes (8 petabytes).
    df -P "$mount_point" |
    awk '($6 == "'"$mount_point"'") { printf "%.0f", $2*1024; exit }'
}

if [ $# -ne 2 ]; then
    usage
fi

mount_point="$1"
timeout_seconds="$2"

if [ "$timeout_seconds" -gt $MAX_TIMEOUT_SECONDS ]; then
    echo "max timeout is "$MAX_TIMEOUT_SECONDS" seconds";
    exit 1
fi

mount_size_bytes=$(get_mount_size_bytes /var)
temp_file=$(mktemp --tmpdir="$mount_point" hog_disk.XXXXXXXXXX)
trap 'rm -f "$temp_file"' EXIT
trap 'exit' HUP INT QUIT TERM

for i in $(seq 1 $(( timeout_seconds * 10 ))); do
    fallocate --length "$mount_size_bytes" "$temp_file" 2>/dev/null || true
    sleep 0.1
done
