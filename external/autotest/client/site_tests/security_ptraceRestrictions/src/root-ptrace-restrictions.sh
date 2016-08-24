#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
set -e
if [ "$(whoami)" != "root" ]; then
    echo "Must be root for this test" >&2
    exit 1
fi
NONROOT="$1"

export LANG=C
pid=
dir=

function start_sleeper()
{
    dir=$(mktemp -d -t sleeper-XXXXXX)
    mkfifo "$dir"/status
    minijail0 -p -- ./inside-pidns.sh "$1" $NONROOT >"$dir"/status &
    pid=$!
    # Immediately forget about minijail process. We will find sleeper next.
    disown $pid
    # Wait for sleeper to start up.
    read status < "$dir"/status
    # Find sleeper pid.
    while [ $(ps -p $pid -o comm=) != "sleeper" ]; do
        pid=$(ps -ef | awk '{ if ($3 == '"$pid"') { print $2 }}')
        if [ -z "$pid" ]; then
            echo "Failed to locate pidns sleeper." >&2
            exit 1
        fi
    done
}

function kill_sleeper()
{
    kill $pid
    rm -rf "$dir"
}

rc=0

# Validate that prctl(PR_SET_PTRACER, 0, ...) cannot be ptraced across pidns.
start_sleeper 0
OUT=$(su -c 'gdb -ex "attach '"$pid"'" -ex "quit" --batch' $NONROOT \
        </dev/null 2>&1)
prctl="prctl(PR_SET_PTRACER, 0, ...)"
if echo "$OUT" | grep -q 'Operation not permitted'; then
    echo "ok: $prctl correctly not allowed ptrace"
else
    echo "FAIL: $prctl unexpectedly allowed ptrace"
    rc=1
fi
kill_sleeper

# Validate that prctl(PR_SET_PTRACER, -1, ...) can be ptraced across pidns.
start_sleeper -1
OUT=$(su -c 'gdb -ex "attach '"$pid"'" -ex "quit" --batch' $NONROOT \
        </dev/null 2>&1)
prctl="prctl(PR_SET_PTRACER, -1, ...)"
if echo "$OUT" | grep -q 'Quit anyway'; then
    echo "ok: $prctl correctly allowed ptrace"
else
    echo "FAIL: $prctl unexpectedly not allowed ptrace"
    rc=1
fi
kill_sleeper

exit $rc
