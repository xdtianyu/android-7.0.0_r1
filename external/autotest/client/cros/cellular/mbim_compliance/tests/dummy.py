# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class DummyTest(test.Test):
    """ A dummy test that always passes. """

    def run(self):
        """ Always passes. """
        pass
