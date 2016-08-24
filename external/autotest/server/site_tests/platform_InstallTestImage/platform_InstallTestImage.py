# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test

class platform_InstallTestImage(test.test):
    """Installs a specified test image onto a servo-connected DUT."""
    version = 1

    def run_once(self, host, image_url):
        """Install image from URL `image_url` on `host`.

        @param host Host object representing DUT to be re-imaged.
        @param image_url URL of a test image to be installed.

        """
        host.servo_install(image_url=image_url)
