# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import entity
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class Assertion(entity.Entity):
    """ Base class for all assertions. """

    def check(self):
        """ Check that the assertion holds. """
        mbim_errors.log_and_raise(NotImplementedError)
