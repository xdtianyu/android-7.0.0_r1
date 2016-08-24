#!/usr/bin/python
#
# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Script to check the number of long-running processes.

This script gets the number of processes for "gsutil" and "autoserv"
that are running more than 24 hours, and throws the number to stats
dashboard.

This script depends on the "etimes" user-defined format of "ps".
Goobuntu 14.04 has the version of ps that supports etimes, but not
Goobuntu 12.04.
"""


import socket
import subprocess

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats


STATS_KEY = 'hung_processes.%s' % socket.gethostname().replace('.', '_')


def check_proc(prog, max_elapsed_sec):
    """Check the number of long-running processes for a given program.

    Finds out the number of processes for a given program that have run
    more than a given elapsed time.
    Sends out the number to stats dashboard.

    @param prog: Program name.
    @param max_elapsed_sec: Max elapsed time in seconds. Processes that
                            have run more than this value will be caught.
    """
    cmd = ('ps -eo etimes,args | grep "%s" | awk \'{if($1 > %d) print $0}\' | '
           'wc -l' % (prog, max_elapsed_sec))
    count = int(subprocess.check_output(cmd, shell = True))
    autotest_stats.Gauge(STATS_KEY).send(prog, count)


def main():
    for p in ('gsutil', 'autoserv'):
        check_proc(p, 86400)


if __name__ == '__main__':
    main()
