# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.client.cros.cellular import scpi
from autotest_lib.client.cros.cellular import cellular_logging
from autotest_lib.client.cros.cellular import base_station_8960
from autotest_lib.client.cros.cellular import prologix_scpi_driver
from autotest_lib.client.cros.cellular import labconfig

log = cellular_logging.SetupCellularLogging('base_station_8960_test')

config = labconfig.Configuration(['--cell', 'mtv', '--technology', 'CDMA'])

class test_8960(unittest.TestCase):
    """
    Test the 8960 class.
    """
    def test_make_one(self):
        x = config.cell['basestations'][0]
        adapter = x['gpib_adapter']
        scpi_device = scpi.Scpi(
            prologix_scpi_driver.PrologixScpiDriver(
                hostname=adapter['address'],
                port=adapter['ip_port'],
                gpib_address=adapter['gpib_address'],
                read_timeout_seconds=5),
                opc_on_stanza=True)
        call_box = base_station_8960.BaseStation8960(scpi_device,
                                                     no_initialization=False)

if __name__ == '__main__':
    unittest.main()
