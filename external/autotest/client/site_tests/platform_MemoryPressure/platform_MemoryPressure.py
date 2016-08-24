# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

class platform_MemoryPressure(test.test):
    version = 1

    def run_once(self, tab_open_secs=1.5, timeout_secs=180):
        perf_results = {}
        time_limit = time.time() + timeout_secs
        err = False
        # 1 for initial tab opened
        n_tabs = 1

        # Open tabs until a tab discard notification arrives, or a time limit
        # is reached.
        with chrome.Chrome() as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            while time.time() <= time_limit and not err:
                tab = cr.browser.tabs.New()
                n_tabs += 1
                # The program in js-bloat.html allocates a few large arrays and
                # forces them in memory by touching some of their elements.
                tab.Navigate(cr.browser.platform.http_server.UrlOf(
                        os.path.join(self.bindir, 'js-bloat.html')))
                tab.WaitForDocumentReadyStateToBeComplete()
                time.sleep(tab_open_secs)
                if n_tabs > len(cr.browser.tabs):
                    err = True

        if err:
            logging.info("tab discard after %d tabs", n_tabs)
        else:
            msg = "FAIL: no tab discard after opening %d tabs in %ds" % \
                (n_tabs, timeout_secs)
            logging.error(msg)
            raise error.TestError(msg)
        perf_results["NumberOfTabsAtFirstDiscard"] = n_tabs
        self.write_perf_keyval(perf_results)
