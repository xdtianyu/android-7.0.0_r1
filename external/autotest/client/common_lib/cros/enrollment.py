# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome


def _ExecuteOobeCmd(browser, cmd):
    logging.info('Invoking ' + cmd)
    oobe = browser.oobe
    oobe.WaitForJavaScriptExpression('typeof Oobe !== \'undefined\'', 10)
    oobe.ExecuteJavaScript(cmd)


def SwitchToRemora(browser):
    """Switch to Remora enrollment.

    @param browser: telemetry browser object.
    """
    chrome.Chrome.wait_for_browser_restart(
            lambda: _ExecuteOobeCmd(browser,
                                    'Oobe.remoraRequisitionForTesting();'))
    utils.poll_for_condition(lambda: browser.oobe_exists, timeout=30)


def RemoraEnrollment(browser, user_id, password):
    """Enterprise login for a Remora device.

    @param browser: telemetry browser object.
    @param user_id: login credentials user_id.
    @param password: login credentials password.
    """
    SwitchToRemora(browser)
    chrome.Chrome.wait_for_browser_restart(
            lambda: browser.oobe.NavigateGaiaLogin(
                    user_id, password, enterprise_enroll=True,
                    for_user_triggered_enrollment=False))
