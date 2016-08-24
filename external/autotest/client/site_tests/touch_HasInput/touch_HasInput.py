# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import touch_playback_test_base


class touch_HasInput(touch_playback_test_base.touch_playback_test_base):
    """Check that device has the input type specified."""
    version = 1

    def run_once(self, input_type=''):
        """Entry point of this test.

        @param input_type: a string representing the required input type.  See
            the input_playback class for possible types.

        """
        if not input_type:
            raise error.TestError('Please supply an input type!')

        if not self.player.has(input_type):
            raise error.TestFail('Device does not have a %s!' % input_type)
