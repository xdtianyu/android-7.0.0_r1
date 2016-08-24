# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import backchannel, network

from autotest_lib.client.cros.cellular import cellular, cell_tools, environment

import contextlib, logging, re, socket, string, time, urllib2

from autotest_lib.client.cros import flimflam_test_path
import flimflam, routing

# Cellular smoke test and documentation for writing cell tests

class cellular_Smoke(test.test):
    version = 1

    # The autotest infrastructure calls run_once.  The control file
    # fetches the JSON lab config and passes it in as a python object

    def run_once(self, config, technology):
        # The DefaultCellularTestContext builds:
        #  * a backchannel ethernet context.  This makes a virtual
        #    device that connects the DUT to the test infrastructure.
        #    It has restrictive routes and is outside of flimflam's
        #    control.  This makes the tests resilient to flimflam
        #    restarts and helps to ensure that the test is actually
        #    sending traffic on the cellular link
        #  * an OtherDeviceShutdownContext, which shuts down other
        #    network devices on the host.  Again, this is to ensure
        #    that test traffic goes over the modem
        #  * A cellular test environment context, which lets us
        #    interact with the cell network.

        with environment.DefaultCellularTestContext(config) as c:
            env = c.env
            flim = flimflam.FlimFlam()
            env.StartDefault(technology)
            network.ResetAllModems(flim)
            cell_tools.PrepareModemForTechnology('', technology)

            # TODO(rochberg) Need to figure out what isn't settling here.
            # Going to wait 'til after ResetAllModems changes land.
            time.sleep(10)

            # Clear all errors before we start.
            # Resetting the modem above may have caused some errors on the
            # 8960 (eg. lost connection, etc).
            env.emulator.ClearErrors()

            service = env.CheckedConnectToCellular()
            env.CheckHttpConnectivity()
            env.CheckedDisconnectFromCellular(service)
