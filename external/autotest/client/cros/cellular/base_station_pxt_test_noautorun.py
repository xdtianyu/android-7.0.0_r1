# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import scpi
import cellular_logging
import unittest

import common
from autotest_lib.client.cros.cellular import labconfig
import base_station_pxt
import prologix_scpi_driver

log = cellular_logging.SetupCellularLogging('base_station_pxt_test')

config = labconfig.Configuration(['--cell', 'mtv', '--technology', 'CDMA'])


class test_pxt(unittest.TestCase):
    """
    Test the pxt class.
    """

    def test_BasicInit(self):
        self._call_box_init()
        self._call_box_close()

    def _call_box_init(self):
        x = config.cell['basestations'][1]
        adapter = x['gpib_adapter']
        scpi_device = scpi.Scpi(
            prologix_scpi_driver.PrologixScpiDriver(
                hostname=adapter['address'],
                port=adapter['ip_port'],
                gpib_address=adapter['gpib_address'],
                read_timeout_seconds=5),
                opc_on_stanza=True)
        self.call_box = base_station_pxt.BaseStationPxt(
            scpi_device, no_initialization=False)

    def _call_box_close(self):
        self.call_box.Close()

    def test_GetRatUeDataStatus(self):
        """Test this function on the PXT class"""
        self._call_box_init()
        self.call_box.SetTechnology('Technology:LTE')
        print self.call_box.GetRatUeDataStatus()
        self._call_box_close()


if __name__ == '__main__':
    unittest.main()
