# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging, pickle, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import backchannel, http_speed, network

from autotest_lib.client.cros.cellular import cellular, cell_tools, environment

from autotest_lib.client.cros import flimflam_test_path
import flimflam


class cellular_Throughput(test.test):
    version = 1

    def run_once(self, config, technology):
        with environment.DefaultCellularTestContext(config) as c:
            env = c.env
            flim = flimflam.FlimFlam()
            env.StartDefault(technology)
            network.ResetAllModems(flim)
            cell_tools.PrepareModemForTechnology('', technology)

            # TODO(rochberg): Figure out why this is necessary
            time.sleep(10)

            # Clear all errors before we start.
            # Resetting the modem above may have caused some errors on the
            # 8960 (eg. lost connection, etc).
            env.emulator.ClearErrors()

            service = env.CheckedConnectToCellular()

            # TODO(rochberg): Factor this and the counts stuff out
            # so that individual tests don't have to care.
            env.emulator.LogStats()
            env.emulator.ResetDataCounters()

            perf = http_speed.HttpSpeed(
                env.config.cell['perfserver']['download_url_format_string'],
                env.config.cell['perfserver']['upload_url'])


            # TODO(rochberg):  Can/should we these values into the
            # write_perf_keyval dictionary?  Now we just log them.
            env.emulator.GetDataCounters()

            env.CheckedDisconnectFromCellular(service)

            self.write_perf_keyval(perf)
