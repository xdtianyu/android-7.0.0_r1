#!/usr/bin/python -t
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import threading
import time

import common
from autotest_lib.site_utils.lib import infra
from autotest_lib.site_utils.stats import registry


def main():
    """
    Runs all of the registered functions in stats/
    """

    threads = []
    pollers = registry.registered_functions()

    for sam in infra.sam_servers():
        for f in pollers.get('sam', []):
            threads.append(threading.Thread(target=f, args=(sam,)))

    for drone in infra.drone_servers():
        for f in pollers.get('drone', []):
            threads.append(threading.Thread(target=f, args=(drone,)))

    for devserver in infra.devserver_servers():
        for f in pollers.get('devserver', []):
            threads.append(threading.Thread(target=f, args=(devserver,)))

    for f in pollers.get(None, []):
        threads.append(threading.Thread(target=f))

    for thread in threads:
        thread.daemon = True
        thread.start()

    # Now we want to stay responsive to ctrl-c, so we need to just idle the main
    # thread.  If we notice that all of our threads disappeared though, there's
    # no point in continuing to run.
    while threading.active_count() > 0:
        time.sleep(1)


if __name__ == '__main__':
    sys.exit(main())
