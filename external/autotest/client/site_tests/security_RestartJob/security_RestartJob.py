# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import os.path
import socket

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, cros_ui, login


class security_RestartJob(test.test):
    """Verifies that RestartJob cannot be abused to execute arbitrary processes.
    """
    version = 1


    _FLAGFILE = '/tmp/security_RestartJob_regression'


    def _ps(self, proc=constants.BROWSER):
        """Grab the oldest pid for a process named |proc|."""
        pscmd = 'ps -C %s -o pid --no-header | head -1' % proc
        return utils.system_output(pscmd)


    def run_once(self):
        """Main test code."""
        login.wait_for_browser()
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.SessionManager',
                               '/org/chromium/SessionManager')
        sessionmanager = dbus.Interface(proxy,
                                        'org.chromium.SessionManagerInterface')

        # Craft a malicious replacement for the target process.
        cmd = ['touch', self._FLAGFILE]

        # Try to get our malicious replacement to run via RestartJob.
        try:
            remote, local = socket.socketpair(socket.AF_UNIX)
            logging.info('Calling RestartJob(<socket>, %r)', cmd)
            sessionmanager.RestartJob(dbus.types.UnixFd(remote), cmd)
            # Fails if the RestartJob call doesn't generate an error.
            raise error.TestFail('RestartJob regression!')
        except dbus.DBusException as e:
            logging.info(e.get_dbus_message())
            pass
        except OSError as e:
            raise error.TestError('Could not create sockets for creds: %s', e)
        finally:
            try:
                local.close()
            except OSError:
                pass

        if os.path.exists(self._FLAGFILE):
            raise error.TestFail('RestartJobWithAuth regression!')


    def cleanup(self):
        """Reset the UI, since this test killed Chrome."""
        cros_ui.nuke()
