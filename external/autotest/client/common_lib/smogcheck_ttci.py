# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Python library to interact with TTCI module for TPM testing.

Background
 - TTCI stands for TPM Test Controller Interface
 - TTCI is a custom-designed hardware board that can be used to test TPM module
 - TTCI board contains two modules: PCA9555 and INA219. This library provides
   methods to interact with these modules programmatically

Dependency
 - This library depends on a new C shared library called "libsmogcheck.so".
 - In order to run test cases built using this API, one needs a TTCI board

Notes:
 - An exception is raised if it doesn't make logical sense to continue program
   flow (e.g. I/O error prevents test case from executing)
 - An exception is caught and then converted to an error code if the caller
   expects to check for error code per API definition
"""

import logging
from autotest_lib.client.common_lib import smogcheck_ina219, smogcheck_pca9555


# I2C slave addresses of INA219 module
INA219_BPWR_SLV = 0x40  # Backup Power
INA219_MPWR_SLV = 0x44  # Main Power


class TtciError(Exception):
    """Base class for all errors in this module."""


class TtciController(object):
    """Object to control TTCI board used for TPM module testing."""

    def __init__(self):
        """Constructor.

        Mandatory params:
          err: error string.
          ina_backup_obj: an instance of InaController (for Backup Power port
                          of INA219 module).
          ina_main_obj: an instance of InaController (for Main Power port
                        of INA219 module).
          pca_obj: an instance of PcaController.

        Raises:
          TtciError: if error initializing TTCI controller.
        """
        self.err = None
        try:
            # Initialize PCA9555 module.
            self.pca_obj = smogcheck_pca9555.PcaController()

            # Initialize INA219 module.
            self.ina_main_obj = smogcheck_ina219.InaController(
                slave_addr=INA219_MPWR_SLV)
            self.ina_backup_obj = smogcheck_ina219.InaController(
                slave_addr=INA219_BPWR_SLV)
        except smogcheck_pca9555.PcaError, e:
            raise TtciError('Error initialize PCA9555 module: %s' % e)
        except smogcheck_ina219.InaError, e:
            raise TtciError('Error initialize INA219 module: %s' % e)

    def TTCI_Get_Main_Power_Metrics(self):
        """Gets voltage and current measurements from INA219 Main Power.

        See docstring of getPowerMetrics() in smogcheck_ina219.py.
        """
        return self.ina_main_obj.getPowerMetrics()

    def TTCI_Get_Backup_Power_Metrics(self):
        """Gets voltage and current measurements from INA219 Backup Power.

        See docstring of getPowerMetrics() in smogcheck_ina219.py.
        """
        return self.ina_backup_obj.getPowerMetrics()

    def TTCI_Set_Main_Power_Control(self, turn_on):
        """De/activated TPM Main Power.

        Args:
          turn_on: a boolean, on (true) = set bit to 1.

        See docstring of setPCAcontrol() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setPCAcontrol('main_power', turn_on=turn_on)

    def TTCI_Set_Backup_Power_Control(self, turn_on):
        """De/activated TPM Backup Power.

        Args:
          turn_on: a boolean, on (true) = set bit to 1.

        See docstring of setPCAcontrol() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setPCAcontrol('backup_power', turn_on=turn_on)

    def TTCI_Set_Reset_Control(self, turn_on):
        """De/activated TPM Reset.

        Exception note:
          for TPM Reset, true means setting bit value to 0 (not 1).

        Args:
          turn_on: a boolean, on (true) = set bit to 0.

        See docstring of setPCAcontrol() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setPCAcontrol('reset', turn_on=not(turn_on))

    def TTCI_Set_PP_Control(self, turn_on):
        """De/activated TPM Physical Presence.

        Args:
          turn_on: a boolean, on (true) = set bit to 1.

        See docstring of setPCAcontrol() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setPCAcontrol('pp', turn_on=turn_on)

    def TTCI_Set_TPM_I2C_Control(self, turn_on):
        """Enable/Disable I2C communication with TPM.

        Args:
          turn_on: a boolean, on (true) = set bit to 1.

        See docstring of setPCAcontrol() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setPCAcontrol('tpm_i2c', turn_on=turn_on)

    def TTCI_Get_Main_Power_Status(self):
        """Checks bit value of Main Power.

        See docstring of getPCAbitStatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getPCAbitStatus('main_power')

    def TTCI_Get_Backup_Power_Status(self):
        """Checks bit value of Backup Power.

        See docstring of getPCAbitStatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getPCAbitStatus('backup_power')

    def TTCI_Get_PP_Status(self):
        """Checks bit value of Physical Presence.

        See docstring of getPCAbitStatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getPCAbitStatus('pp')

    def TTCI_Get_TPM_I2C_Status(self):
        """Checks bit value of TPM I2C.

        See docstring of getPCAbitStatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getPCAbitStatus('tpm_i2c')

    def TTCI_Set_LEDs(self, bit_value, failure, warning):
        """De/activates PCA9555 LEDs.

        See docstring of setLEDs() in smogcheck_pca9555.py.
        """
        return self.pca_obj.setLEDs(bit_value, failure, warning)

    def TTCI_Get_Switch_Status(self):
        """Checks status of DIP Switches (2-bit).

        See docstring of getSwitchStatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getSwitchStatus()

    def TTCI_Get_LED_Status(self):
        """Checks LED status.

        See docstring of getLEDstatus() in smogcheck_pca9555.py.
        """
        return self.pca_obj.getLEDstatus()


def computeTimeElapsed(end, start):
    """Computes time difference in microseconds.

    Args:
      end: a datetime.datetime() object, end timestamp.
      start: a datetime.datetime() object, start timestamp.

    Returns:
      usec: an integer.
    """
    t = end - start
    usec = 1000000 * t.seconds + t.microseconds
    logging.info('Elapsed time = %d usec', usec)
    return usec
