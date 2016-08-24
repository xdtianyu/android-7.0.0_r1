# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time, utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, cros_ui
from autotest_lib.client.cros.graphics import graphics_utils

class desktopui_GmailLatency(test.test):
    version = 1

    def run_once(self):
        url = 'http://azlaba29.mtv.corp.google.com:9380/auto/google3/java/'\
                'com/google/caribou/ui/pinto/modules/auto/tests/'\
                'latencytest_auto.html'
        js_expr = 'domAutomationController.send(!!window.G_testRunner'\
                '&& window.G_testRunner.isFinished())'

        # timeout is in ms, so allow a 5 minute timeout
        # as of jan-11 it normally takes about 2 minutes on x86-mario
        timeout = 5 * 60 * 1000

        os.chdir(self.bindir)

        # Select correct binary.
        cpuType = utils.get_cpu_arch()
        url_fetch_test = 'url_fetch_test'
        if cpuType == "arm":
            url_fetch_test += '.arm'

        # Stop chrome from restarting and kill login manager.
        try:
            orig_pid = utils.system_output('pgrep %s' %
                constants.SESSION_MANAGER)
            open(constants.DISABLE_BROWSER_RESTART_MAGIC_FILE, 'w').close()
        except IOError, e:
            logging.debug(e)
            raise error.TestError('Failed to disable browser restarting.')

        # We could kill with signal 9 so that the session manager doesn't exit.
        # But this seems to leave the screen blank while the test is running.
        # So do it this way (which means clean_exit is always False)
        utils.nuke_process_by_name(name=constants.BROWSER)

        clean_exit = False
        try:
            time.sleep(1)
            new_pid = utils.system_output('pgrep %s' %
                constants.SESSION_MANAGER)
            if orig_pid != new_pid:
                # This is expected behaviour of the session manager.
                pass

            # Copy over chrome, chrome.pak, locales, chromeos needed for test.
            utils.system('cp -r %s/* .' % '/opt/google/chrome')

            # Setup parameters
            params = ('--url="%s" --wait_js_expr="%s" --wait_js_timeout=%d' %
                        (url, js_expr, timeout))
            graphics_utils.xsystem('./%s %s' % (url_fetch_test, params))

        except error.CmdError, e:
            logging.debug(e)
            raise error.TestFail('Gmail Latency test was unsuccessful in %s'
                                 % os.getcwd())

        finally:
            # Allow chrome to be restarted again.
            os.unlink(constants.DISABLE_BROWSER_RESTART_MAGIC_FILE)

            # Reset the UI but only if we need to (avoid double reset).
            if not clean_exit:
                cros_ui.nuke()
