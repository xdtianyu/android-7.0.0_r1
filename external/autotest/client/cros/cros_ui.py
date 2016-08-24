# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common, logging, os, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants

# Log messages used to signal when we're restarting UI. Used to detect
# crashes by cros_ui_test.UITest.
UI_RESTART_ATTEMPT_MSG = 'cros_ui.py: Attempting StopSession...'
UI_RESTART_COMPLETE_MSG = 'cros_ui.py: StopSession complete.'
RESTART_UI_TIMEOUT = 90  # longer because we may be crash dumping now.


def get_chrome_session_ident(host=None):
    """Return an identifier that changes whenever Chrome restarts.

    This function returns a value that is unique to the most
    recently started Chrome process; the returned value changes
    each time Chrome restarts and displays the login screen.  The
    change in the value can be used to detect a successful Chrome
    restart.

    Note that uniqueness is only guaranteed until the host reboots.

    Args:
        host:  If not None, a host object on which to test Chrome
            state, rather than running commands on the local host.

    """
    if host:
        return host.run(constants.LOGIN_PROMPT_STATUS_COMMAND).stdout
    return utils.run(constants.LOGIN_PROMPT_STATUS_COMMAND).stdout


def wait_for_chrome_ready(old_session, host=None,
                          timeout=RESTART_UI_TIMEOUT):
    """Wait until a new Chrome login prompt is on screen and ready.

    The standard formula to check whether the prompt has appeared yet
    is with a pattern like the following:

       session = get_chrome_session_ident()
       logout()
       wait_for_chrome_ready(session)

    Args:
        old_session:  identifier for the login prompt prior to
            restarting Chrome.
        host:  If not None, a host object on which to test Chrome
            state, rather than running commands on the local host.
        timeout: float number of seconds to wait

    Raises:
        TimeoutError: Login prompt didn't get up before timeout

    """
    utils.poll_for_condition(
        condition=lambda: old_session != get_chrome_session_ident(host),
        exception=utils.TimeoutError('Timed out waiting for login prompt'),
        timeout=timeout, sleep_interval=1.0)


def stop_and_wait_for_chrome_to_exit(timeout_secs=40):
    """Stops the UI and waits for chrome to exit.

    Stops the UI and waits for all chrome processes to exit or until
    timeout_secs is reached.

    Args:
        timeout_secs: float number of seconds to wait.

    Returns:
        True upon successfully stopping the UI and all chrome processes exiting.
        False otherwise.
    """
    status = stop(allow_fail=True)
    if status:
        logging.error('stop ui returned non-zero status: %s', status)
        return False
    start_time = time.time()
    while time.time() - start_time < timeout_secs:
        status = utils.system('pgrep chrome', ignore_status=True)
        if status == 1: return True
        time.sleep(1)
    logging.error('stop ui failed to stop chrome within %s seconds',
                  timeout_secs)
    return False


def stop(allow_fail=False):
    return utils.system("stop ui", ignore_status=allow_fail)


def start(allow_fail=False, wait_for_login_prompt=True):
    """Start the login manager and wait for the prompt to show up."""
    session = get_chrome_session_ident()
    result = utils.system("start ui", ignore_status=allow_fail)
    # If allow_fail is set, the caller might be calling us when the UI job
    # is already running. In that case, the above command fails.
    if result == 0 and wait_for_login_prompt:
        wait_for_chrome_ready(session)
    return result


def restart(report_stop_failure=False):
    """Restart the session manager.

    - If the user is logged in, the session will be terminated.
    - If the UI is currently down, just go ahead and bring it up unless the
      caller has requested that a failure to stop be reported.
    - To ensure all processes are up and ready, this function will wait
      for the login prompt to show up and be marked as visible.

    @param report_stop_failure: False by default, set to True if you care about
                                the UI being up at the time of call and
                                successfully torn down by this call.
    """
    session = get_chrome_session_ident()

    # Log what we're about to do to /var/log/messages. Used to log crashes later
    # in cleanup by cros_ui_test.UITest.
    utils.system('logger "%s"' % UI_RESTART_ATTEMPT_MSG)

    try:
        if stop(allow_fail=not report_stop_failure) != 0:
            raise error.TestError('Could not stop session')
        start(wait_for_login_prompt=False)
        # Wait for login prompt to appear to indicate that all processes are
        # up and running again.
        wait_for_chrome_ready(session)
    finally:
        utils.system('logger "%s"' % UI_RESTART_COMPLETE_MSG)


def nuke():
    """Nuke the login manager, waiting for it to restart."""
    restart(lambda: utils.nuke_process_by_name(constants.SESSION_MANAGER))


def is_up():
    """Return True if the UI is up, False if not."""
    return 'start/running' in utils.system_output('initctl status ui')


def clear_respawn_state():
    """Removes bookkeeping related to respawning crashed UI."""
    for filename in [constants.UI_RESPAWN_TIMESTAMPS_FILE,
                     constants.UI_TOO_CRASHY_TIMESTAMPS_FILE]:
        try:
            os.unlink(filename)
        except OSError:
            pass  # It's already gone.
