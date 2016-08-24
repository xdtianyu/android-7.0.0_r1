# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import unittest
import cell_tools
import cellular_logging


import dbus

from autotest_lib.client.cros.cellular import labconfig
# Mock out routing in the network file
import mock
import sys
sys.modules['routing'] = mock.MagicMock()
from autotest_lib.client.cros import network
import flimflam
import base_station_pxt
import prologix_scpi_driver
import scpi
import environment

config = labconfig.Configuration(['--cell', 'mtv', '--technology', 'LTE'])
import time

logger = cellular_logging.SetupCellularLogging('cell_tools_test')

technology_lte = 'Technology:LTE'


class test_cell_tools(unittest.TestCase):

    def test_CellularSmokeNoCallBoxSetup(self):
        self._reset_everything()
        logger.debug('making flimflam object..')
        self.flim = flimflam.FlimFlam()
        logger.debug('Find Cellular Device ...')
        self.device = self.flim.FindCellularDevice()
        logger.debug('Find Celluar Service..')
        self.service = self.flim.FindCellularService()
        logger.debug('Set Auto Connect to False ..')
        self.service.SetProperty('AutoConnect', dbus.Boolean(False))

        logger.debug('Reset all modems ..')
        network.ResetAllModems(self.flim)
        logger.debug('Prepare Modem for LTE..')
        cell_tools.PrepareModemForTechnology('', 'Technology:LTE')

        logger.debug('Make another flimflam..')
        self.flim = flimflam.FlimFlam()
        logger.debug('Sleep for 5...')
        time.sleep(5)
        logger.debug('Connect to Cellular...')
        cell_tools.ConnectToCellular(self.flim, timeout=60)
        logger.debug('Clearing errors...')
        env.emulator.ClearErrors()
        logger.debug('Check connect to cellular ...')
        service = env.CheckedConnectToCellular()

    def test_TurnOnPxtAndConnectToCellularWorks(self):
        self._reset_everything()
        self.flim = flimflam.FlimFlam()
        #self.device = self.flim.FindCellularDevice()
        self.service = self.flim.FindCellularService()
        self.service.SetProperty('AutoConnect', dbus.Boolean(False))
        with environment.DefaultCellularTestContext(config) as c:
            env = c.env
            env.StartDefault(technology_lte)
            self.flim = flimflam.FlimFlam()  # because the manger destroys it?
            cell_tools.ConnectToCellular(self.flim, timeout=60)

    def test_TurnOnPxtAndConnectToCellularWorksAddSmoke(self):
        self._reset_everything()
        self.flim = flimflam.FlimFlam()
        #self.device = self.flim.FindCellularDevice()
        self.service = self.flim.FindCellularService()
        self.service.SetProperty('AutoConnect', dbus.Boolean(False))
        with environment.DefaultCellularTestContext(config) as c:
            env = c.env
            env.StartDefault(technology_lte)
            cell_tools.PrepareModemForTechnology('', technology_lte)
            self.flim = flimflam.FlimFlam()  # because the manger destroys it?
            #network.ResetAllModems(self.flim)
            self.flim = flimflam.FlimFlam()  # because the manger destroys it?
            cell_tools.ConnectToCellular(self.flim, timeout=60)
            env.emulator.ClearErrors()
            service = env.CheckedConnectToCellular()
            #env.CheckHttpConnectivity()
            env.CheckedDisconnectFromCellular(service)

    def _reset_everything(self):
        """Rest the modem, ModemManger, and Shill"""
        import os
        logger.debug('Resetting Modem...')
        os.system('modem reset')
        logger.debug('Resetting shill...')
        os.system('restart shill')
        logger.debug('Resetting modemmanager...')
        os.system('restart modemmanager')

if __name__ == '__main__':
    unittest.main()
