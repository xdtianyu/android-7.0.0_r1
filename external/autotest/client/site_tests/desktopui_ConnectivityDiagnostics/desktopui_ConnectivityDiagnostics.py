# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class desktopui_ConnectivityDiagnostics(test.test):
    """Basic sanity check of connectivity diagnostics in Chrome."""
    version = 1


    EXT_CODE = """
var complete = false;
var success = false;
var error = false;
// Send a message to the connectivity diagnostics app asking it to run tests.
chrome.runtime.sendMessage(
    "kodldpbjkkmmnilagfdheibampofhao",
    {
      command: "test"
    }, function(result) {
      complete = true;
      if (result instanceof Object) {
        success = result.success;
        error = result.error;
      } else {
        success = result;
        if (!success) {
          error = "Tests threw an exception";
        }
      }
      console.log(result);
    });
"""


    def run_once(self):
        with chrome.Chrome(disable_default_apps=False, autotest_ext=True) as cr:
            extension = cr.autotest_ext
            extension.EvaluateJavaScript(self.EXT_CODE)

            utils.poll_for_condition(
                    lambda: extension.EvaluateJavaScript('complete;'),
                    exception = error.TestError('Tests failed to complete'))

            if not extension.EvaluateJavaScript('success;'):
                raise error.TestFail(extension.EvaluateJavaScript('error;'))
