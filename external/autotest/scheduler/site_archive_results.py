# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common, logging
from autotest_lib.client.common_lib import global_config, utils
from autotest_lib.scheduler import drone_utility


class SiteResultsArchiver(object):


    def archive_results(self, path):
        """
        Skip archiving results.

        Avoid copying results back to atlantis1 AKA cautotest. This copy is
        unnecessary because all results are backed up to Google Storage via
        gs_offloader.
        """
        pass
