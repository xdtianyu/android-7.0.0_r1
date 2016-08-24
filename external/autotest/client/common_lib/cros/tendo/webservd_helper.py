# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import utils as bin_utils
from autotest_lib.client.common_lib import utils

def run_and_check_result(host, command):
    """Run a command on |host| and return whether it succeeded.

    @param host: Host object  if we're interested in a remote host.
    @param cmd: string command to run on |host|.
    @return True if the command succeeds. otherwise False.

    """
    run = utils.run
    if host is not None:
        run = host.run
    result = run(command, ignore_status=True)
    return result.exit_status == 0

def webservd_is_installed(host=None):
    """Check if the webservd binary is installed.

    @param host: Host object if we're interested in a remote host.
    @return True iff webservd is installed in this system.

    """
    return run_and_check_result(
            host, 'if [ -f /usr/bin/webservd ]; then exit 0; fi; exit 1')

def webservd_is_running(host=None, startup_timeout_seconds=5):
    """Check if the webservd binary is installed and running.

    @param host: Host object if we're interested in a remote host.
    @param startup_timeout_seconds: int time to wait for the server to start.
    @return True iff webservd is installed and running in this system.

    """
    if not webservd_is_installed(host):
        return False

    try:
        check_running = lambda: run_and_check_result(
                host, 'initctl status webservd | grep start/running')
        bin_utils.poll_for_condition(check_running,
                                     timeout=startup_timeout_seconds,
                                     desc='webservd startup')
    except bin_utils.TimeoutError:
        return False

    return True
