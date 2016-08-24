# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint
import sys
import time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants, cros_logging
from autotest_lib.client.cros import httpd


class desktopui_FlashSanityCheck(test.test):
    """
    Sanity test that ensures flash instance is launched when a swf is played.
    """
    version = 4

    _messages_log_reader = None
    _ui_log_reader = None
    _test_url = None
    _testServer = None

    def initialize(self):
        logging.info('initialize() - Run html server.')
        self._test_url = 'http://localhost:8000/index.html'
        self._testServer = httpd.HTTPListener(8000, docroot=self.bindir)
        self._testServer.run()
        logging.info('initialize() - Wait 5 seconds for server to run.')
        time.sleep(5)

    def cleanup(self):
        if self._testServer is not None:
            self._testServer.stop()

    def run_flash_sanity_test(self, browser, time_to_wait_secs):
        """Run the Flash sanity test.

        @param browser: The Browser object to run the test with.
        @param time_to_wait_secs: wait time for swf file to load.

        """
        tab = None
        # BUG(485108): Work around a telemetry timing out after login.
        try:
            logging.info('Getting tab from telemetry...')
            tab = browser.tabs[0]
        except:
            logging.warning('Unexpected exception getting tab: %s',
                            pprint.pformat(sys.exc_info()[0]))
        if tab is None:
            return False

        logging.info('Initialize reading system logs.')
        self._messages_log_reader = cros_logging.LogReader()
        self._messages_log_reader.set_start_by_current()
        self._ui_log_reader = cros_logging.LogReader('/var/log/ui/ui.LATEST')
        self._ui_log_reader.set_start_by_current()
        logging.info('Done initializing system logs.')

        # Ensure that the swf got pulled.
        pulled = False
        try:
            latch = self._testServer.add_wait_url('/Trivial.swf')
            tab.Navigate(self._test_url)
            tab.WaitForDocumentReadyStateToBeComplete()
            logging.info('Waiting up to %ds for document.', time_to_wait_secs)
            latch.wait(time_to_wait_secs)
            pulled = True
        except:
            logging.warning('Unexpected exception wating for document: %s',
                            pprint.pformat(sys.exc_info()[0]))
        if not pulled:
            return False

        logging.info('Waiting for Pepper process.')
        # Verify that we see a ppapi process and assume it is Flash.
        ppapi = utils.wait_for_value_changed(
            lambda: (utils.get_process_list('chrome', '--type=ppapi')),
            old_value=[],
            timeout_sec=5)
        logging.info('ppapi process list at start: %s', ', '.join(ppapi))
        if not ppapi:
            msg = 'flash/platform/pepper/pep_'
            if not self._ui_log_reader.can_find(msg):
                raise error.TestFail(
                    'Flash did not start (logs) and no ppapi process found.')
            # There is a chrome bug where the command line of the ppapi and
            # other processes is shown as "type=zygote". Bail out if we see more
            # than 2. Notice, we already did the waiting, so there is no need to
            # do more of it.
            zygote = utils.get_process_list('chrome', '--type=zygote')
            if len(zygote) > 2:
                logging.warning('Flash probably launched by Chrome as zygote: '
                                '<%s>.', ', '.join(zygote))
                return False

        # We have a ppapi process. Let it run for a little and see if it is
        # still alive.
        logging.info('Running Flash content for a little while.')
        time.sleep(5)
        logging.info('Verifying the Pepper process is still around.')
        ppapi = utils.wait_for_value_changed(
            lambda: (utils.get_process_list('chrome', '--type=ppapi')),
            old_value=[],
            timeout_sec=3)
        # Notice that we are not checking for equality of ppapi on purpose.
        logging.info('PPapi process list found: <%s>', ', '.join(ppapi))

        # Any better pattern matching?
        msg = ' Received crash notification for ' + constants.BROWSER
        if self._messages_log_reader.can_find(msg):
            raise error.TestFail('Browser crashed during test.')

        if not ppapi:
            raise error.TestFail('Pepper process disappeared during test.')

        # At a minimum Flash identifies itself during process start.
        msg = 'flash/platform/pepper/pep_'
        if not self._ui_log_reader.can_find(msg):
            raise error.TestFail('Saw ppapi process but no Flash output.')

        return True

    def run_once(self, time_to_wait_secs=5):
        utils.verify_flash_installed()
        retries = 10
        flash_tested = False
        while not flash_tested and retries > 0:
            retries = retries - 1
            with chrome.Chrome() as cr:
                flash_tested = self.run_flash_sanity_test(cr.browser,
                                                          time_to_wait_secs)
        if not flash_tested:
            raise error.TestFail('Unable to test Flash due to other problems.')
