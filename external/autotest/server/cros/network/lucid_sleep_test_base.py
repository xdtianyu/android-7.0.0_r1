# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import dark_resume_utils
from autotest_lib.server.cros.network import wifi_cell_test_base

class LucidSleepTestBase(wifi_cell_test_base.WiFiCellTestBase):
    """An abstract base class for Lucid Sleep autotests in WiFi cells.

       Lucid Sleep tests are WiFi cell tests that perform wake-on-WiFi-related
       setup and cleanup routines.
    """

    @property
    def dr_utils(self):
        """@return the dark resume utilities for this test."""
        return self._dr_utils


    def initialize(self, host):
        super(LucidSleepTestBase, self).initialize(host)
        self._dr_utils = dark_resume_utils.DarkResumeUtils(host)


    def warmup(self, host, raw_cmdline_args, additional_params=None):
        super(LucidSleepTestBase, self).warmup(
                host, raw_cmdline_args, additional_params)
        if (self.context.client.is_wake_on_wifi_supported() is False):
            raise error.TestNAError('Wake on WiFi is not supported by this DUT')


    def cleanup(self):
        self._dr_utils.teardown()
        super(LucidSleepTestBase, self).cleanup()
