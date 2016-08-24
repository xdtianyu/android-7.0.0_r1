# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, logging, subprocess, time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, smogcheck_ttci, smogcheck_util


class hardware_TPMttci(test.test):
    version = 1

    def setup(self):
        smogcheck_util.enableI2C()
        self.ttci_obj = None

    def _prepareTtciController(self):
        """Prepare PcaController and InaController instances for use.

        Returns:
          an operational PcaController instance, ready to use.
          an operational InaController instance, ready to use.

        Raises:
          TestFail: if error creating a new TtciController instance.
        """
        try:
            self.ttci_obj = smogcheck_ttci.TtciController()
        except smogcheck_ttci.TtciError, e:
            raise error.TestFail('Error creating a TtciController: %s' % e)

    def _getMainPowerStatus(self):
        """Wraps TTCI_Get_Main_Power_Status().

        Raises:
          TestFail: if error getting main power status.
        """
        ret, status = self.ttci_obj.TTCI_Get_Main_Power_Status()
        if ret:
            raise error.TestFail('TTCI_Get_Main_Power_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('Main Power status = %r', status)


    def _getBackupPowerStatus(self):
        """Wraps TTCI_Get_Backup_Power_Status().

        Raises:
          TestFail: if error getting backup power status.
        """
        ret, status = self.ttci_obj.TTCI_Get_Backup_Power_Status()
        if ret:
            raise error.TestFail('TTCI_Get_Backup_Power_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('Backup Power status = %r', status)


    def _getTPMPhysicalPresenceStatus(self):
        """Wraps TTCI_Get_PP_Status().

        Raises:
          TestFail: if error getting Physical Presence status.
        """
        ret, status = self.ttci_obj.TTCI_Get_PP_Status()
        if ret:
            raise error.TestFail('TTCI_Get_PP_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('PP status = %r', status)


    def _getTpmI2cStatus(self):
        """Wraps TTCI_Get_TPM_I2C_Status().

        Raises:
          TestFail: if error getting TPM I2C status.
        """
        ret, status = self.ttci_obj.TTCI_Get_TPM_I2C_Status()
        if ret:
            raise error.TestFail('TTCI_Get_TPM_I2C_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('TPM I2C status = %r', status)

    def run_once(self):
        # Initialize modules on TTCI
        self._prepareTtciController()

        start_time = datetime.datetime.now()
        # Turn on LEDs sequentially
        if self.ttci_obj.TTCI_Set_LEDs(0x1, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x3, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x7, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0xf, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0xf, failure=False, warning=True):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0xf, failure=True, warning=True):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        # Turn off LEDs sequentially
        if self.ttci_obj.TTCI_Set_LEDs(0xf, failure=False, warning=True):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0xf, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x7, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x3, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x1, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_LEDs(0x0, failure=False, warning=False):
            raise error.TestFail('TTCI_Set_LEDs() error: %s' %
                                 self.ttci_obj.err)

        # Get bit status
        ret, status = self.ttci_obj.TTCI_Get_Switch_Status()
        if ret:
            raise error.TestFail('TTCI_Get_Switch_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('Switch status = %r', status)

        ret, bit_value, failure, warning = self.ttci_obj.TTCI_Get_LED_Status()
        if ret:
            raise error.TestFail('TTCI_Get_LED_Status() error: %s' %
                                 self.ttci_obj.err)
        logging.info('LED status: bit_value=%r, failure=%r, warning=%r',
                     bit_value, failure, warning)

        # Test Main Power
        self._getMainPowerStatus()
        if self.ttci_obj.TTCI_Set_Main_Power_Control(turn_on=True):
            raise error.TestFail('TTCI_Set_Main_Power_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getMainPowerStatus()
        if self.ttci_obj.TTCI_Set_Main_Power_Control(turn_on=False):
            raise error.TestFail('TTCI_Set_Main_Power_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getMainPowerStatus()

        # Test Backup Power
        self._getBackupPowerStatus()
        if self.ttci_obj.TTCI_Set_Backup_Power_Control(turn_on=True):
            raise error.TestFail('TTCI_Set_Backup_Power_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getBackupPowerStatus()
        if self.ttci_obj.TTCI_Set_Backup_Power_Control(turn_on=False):
            raise error.TestFail('TTCI_Set_Backup_Power_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getBackupPowerStatus()

        # Test Physical Presence
        self._getTPMPhysicalPresenceStatus()
        if self.ttci_obj.TTCI_Set_PP_Control(turn_on=True):
            raise error.TestFail('TTCI_Set_PP_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getTPMPhysicalPresenceStatus()
        if self.ttci_obj.TTCI_Set_PP_Control(turn_on=False):
            raise error.TestFail('TTCI_Set_PP_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getTPMPhysicalPresenceStatus()

        # Test TPM I2C bit
        self._getTpmI2cStatus()
        if self.ttci_obj.TTCI_Set_TPM_I2C_Control(turn_on=True):
            raise error.TestFail('TTCI_Set_TPM_I2C_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getTpmI2cStatus()
        if self.ttci_obj.TTCI_Set_TPM_I2C_Control(turn_on=False):
            raise error.TestFail('TTCI_Set_TPM_I2C_Control() error: %s' %
                                 self.ttci_obj.err)
        self._getTpmI2cStatus()

        # Test Reset
        if self.ttci_obj.TTCI_Set_Reset_Control(turn_on=True):
            raise error.TestFail('TTCI_Set_TPM_I2C_Control() error: %s' %
                                 self.ttci_obj.err)

        if self.ttci_obj.TTCI_Set_Reset_Control(turn_on=False):
            raise error.TestFail('TTCI_Set_TPM_I2C_Control() error: %s' %
                                 self.ttci_obj.err)

        end_time = datetime.datetime.now()
        smogcheck_util.computeTimeElapsed(end_time, start_time)
