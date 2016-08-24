# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server import test
from autotest_lib.server.hosts import cros_host


class provision_FactoryImage(test.test):
    """Installs a specified factory image onto a servo-connected DUT."""
    version = 1

    def run_once(self, host, image_url):
        """Install image from URL `image_url` on `host`.

        @param host Host object representing DUT to be re-imaged.
        @param image_url URL of a test image to be installed.
        """
        logging.info('Installing image from url %s', image_url)
        #TODO(beeps): once crbug.com/259126 is resolved apply a label.
        host.servo_install(
                image_url=image_url,
                usb_boot_timeout=cros_host.CrosHost.USB_BOOT_TIMEOUT*3,
                install_timeout=cros_host.CrosHost.INSTALL_TIMEOUT*2)
