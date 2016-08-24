# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.webstore_test import ItemType
from autotest_lib.client.cros.webstore_test import webstore_test

class webstore_InstallItem(webstore_test):
    """
    Installs an item and tests that it installed correctly.

    This is used by several tests, which pass the parameters item_id,
    item_type, and install_type to the test. If it's an app, this
    class verifies that the app can launch.
    """
    version = 1

    def run(self, item_id, item_type, install_type):
        self.install_item(item_id, item_type, install_type)
        if item_type != ItemType.extension and item_type != ItemType.theme:
            self.launch_app(item_id)
