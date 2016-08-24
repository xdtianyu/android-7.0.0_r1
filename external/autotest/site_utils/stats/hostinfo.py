# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.site_utils.lib import infra
from autotest_lib.site_utils.stats import registry


@registry.loop_stat('sam')
def time_wait(server):
    """
    Submits a stat for the number of TIME_WAIT sockets that are on the server.

    @param server: The AFE server.
    """
    out = infra.execute_command(server, 'ss -o state time-wait | wc -l')
    stat = autotest_stats.Gauge(server, bare=True)
    # ss prints out a header for the columns also, so we subtract one to report
    # about only the data.
    stat.send('time_wait_sockets', int(out.strip())-1)

