# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities used with Brillo hosts."""

_RUN_BACKGROUND_TEMPLATE = '( %(cmd)s ) </dev/null >/dev/null 2>&1 & echo -n $!'

_WAIT_CMD_TEMPLATE = """\
to=%(timeout)d; \
while test ${to} -ne 0; do \
  test $(ps %(pid)d | wc -l) -gt 1 || break; \
  sleep 1; \
  to=$((to - 1)); \
done; \
test ${to} -ne 0 -o $(ps %(pid)d | wc -l) -eq 1 \
"""


def run_in_background(host, cmd):
    """Runs a command in the background on the DUT.

    @param host: A host object representing the DUT.
    @param cmd: The command to run.

    @return The background process ID (integer).
    """
    background_cmd = _RUN_BACKGROUND_TEMPLATE % {'cmd': cmd}
    return int(host.run_output(background_cmd).strip())


def wait_for_process(host, pid, timeout=-1):
    """Waits for a process on the DUT to terminate.

    @param host: A host object representing the DUT.
    @param pid: The process ID (integer).
    @param timeout: Number of seconds to wait; default is wait forever.

    @return True if process terminated within the alotted time, False otherwise.
    """
    wait_cmd = _WAIT_CMD_TEMPLATE % {'pid': pid, 'timeout': timeout}
    return host.run(wait_cmd, ignore_status=True).exit_status == 0
