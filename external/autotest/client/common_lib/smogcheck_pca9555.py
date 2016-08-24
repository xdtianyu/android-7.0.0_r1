# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Python library to interact with PCA9555 module for TPM testing.

Background
 - PCA9555 is one of two modules on TTCI board
 - This library provides methods to interact with PCA9555 programmatically

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
from autotest_lib.client.common_lib import i2c_slave


# I2C constants
PCA9555_SLV = 0x27  # I2C slave address of PCA9555

# PCA9555 registers
PCA_REG = {
    'IN0': 0,    # Input Port 0
    'IN1': 1,    # Input Port 1
    'OUT0': 2,   # Output Port 0
    'OUT1': 3,   # Output Port 1
    'PI0': 4,    # Polarity Inversion 0
    'PI1': 5,    # Polarity Inversion 1
    'CONF0': 6,  # Configuration 0
    'CONF1': 7,  # Configuration 1
    }

# Each '1' represents turning on corresponding LED via writing to PCA9555
# Output Port Registers
PCA_BIT_ONE = {
    'unalloc_0':    0x01,
    'unalloc_1':    0x02,
    'unalloc_2':    0x04,
    'tpm_i2c':      0x08,
    'yellow_led':   0x10,
    'main_power':   0x10,
    'red_led':      0x20,
    'backup_power': 0x20,
    'reset':        0x40,
    'pp':           0x80,
   }

# Constants used to initialize PCA registers
# TODO(tgao): document these bits after stevenh replies
PCA_OUT0_INIT_VAL = 0xff7f
PCA_CONF0_INIT_VAL = 0xc007


class PcaError(Exception):
    """Base class for all errors in this module."""


class PcaController(i2c_slave.I2cSlave):
    """Object to control PCA9555 module on TTCI board."""

    def __init__(self):
        """Initialize PCA9555 module on the TTCI board.

        Raises:
          PcaError: if error initializing PCA9555 module.
        """
        super(PcaController, self).__init__()
        logging.info('Attempt to initialize PCA9555 module')
        try:
            self.setSlaveAddress(PCA9555_SLV)
            self.writeWord(PCA_REG['OUT0'], PCA_OUT0_INIT_VAL)
            self.writeWord(PCA_REG['PI0'], 0)
            self.writeWord(PCA_REG['CONF0'], PCA_CONF0_INIT_VAL)
        except PcaError, e:
            raise PcaError('Error initializing PCA9555: %s' % e)

    def setPCAcontrol(self, key, turn_on):
        """Sets specific bit value in Output Port 0 of PCA9555.

        Args:
          key: a string, valid dict keys in PCA_BIT_ONE.
          turn_on: a boolean, true = set bit value to 1.

        Returns:
          an integer, 0 for success and -1 for error.
        """
        logging.info('Attempt to set %r bit to %r', key, turn_on)
        try:
            byte_read = self.readByte(PCA_REG['OUT0'])
            if turn_on:
                write_byte = byte_read | PCA_BIT_ONE[key]
            else:
                write_byte = byte_read & ~PCA_BIT_ONE[key]
            self.writeByte(PCA_REG['OUT0'], write_byte)
            return 0
        except PcaError, e:
            logging.error('Error setting PCA9555 Output Port 0: %s', e)
            return -1

    def _computeLEDmask(self, bit_value, failure, warning):
        """Computes proper bit mask to set LED values.

        Args:
          <see docstring for TTCI_Set_LEDs()>

        Returns:
          an integer, 8-bit mask.

        Raises:
          PcaError: if bit value is out of range.
        """
        bit_mask = 0
        if bit_value < 0 or bit_value > 15:
            raise PcaError('Error: bit_value out of range [0, 15]')

        bit_mask = bit_value
        if failure:
            bit_mask |= 0x20
        if warning:
            bit_mask |= 0x10

        return bit_mask

    def getPCAbitStatus(self, key):
        """Gets specific bit value from Output Port 0 of PCA9555.

        Args:
          key: a string, valid dict keys in PCA_BIT_ONE.

        Returns:
          an integer, 0 for success and -1 for error.
          status: a boolean, True if bit value is '1' and False if bit value
                  is '0'.
        """
        status = False
        try:
            if PCA_BIT_ONE[key] & self.readByte(PCA_REG['OUT0']):
                status = True
            return (0, status)
        except PcaError, e:
            logging.error('Error reading from PCA9555 Output Port 0: %s', e)
            return (-1, status)

    def setLEDs(self, bit_value, failure, warning):
        """De/activate PCA9555 LEDs.

        Mapping of LED to bit values in Output Port 1 (register 3)
        (default bit value = 1 <--> LED OFF)
          LED 0 (GREEN):  O1.0 (mask = 0x01)
          LED 1 (GREEN):  O1.1 (mask = 0x02)
          LED 2 (GREEN):  O1.2 (mask = 0x04)
          LED 3 (GREEN):  O1.3 (mask = 0x08)
          LED 4 (YELLOW): O1.4 (mask = 0x10)
          LED 5 (RED):    O1.5 (mask = 0x20)

        To change LED bit values:
        1) read byte value from register 3
        2) set all 6 lower bits to 1 (LED OFF) by logical OR with 0x3f
        3) set appropriate bits to 0 (LED ON) by logical XOR with proper mask
        4) write updated byte value to register 3

        An example: bit_value=9, failure=False, warning=True
        1) read back, say, 0x96, or 1001 0110 (LEDs 0, 3, 5 ON)
        2) 0x96 | 0x3f = 0xbf (all LEDs OFF)
        3) bit_value=9 -> turn on LEDs 1, 2
           failure=False -> keep LED 5 off
           warning=True -> turn on LED 4
           proper mask = 0001 0110, or 0x16
           0xbf ^ 0x16 = 0xa9, or 1010 1001 (LEDs 1, 2, 4 ON)
        4) write 0xa9 to register 3

        Args:
          bit_value: an integer between 0 and 15, representing 4-bit binary
                     value for green LEDs (i.e. 0~3).
          failure: a boolean, true = set red LED value to 0.
          warning: a boolean, true = set yellow LED value to 0.

        Returns:
          an integer, 0 for success and -1 for error.
        """
        logging.info('Attempt to set LED values: bit_value=%r, failure=%r, '
                     'warning=%r', bit_value, failure, warning)
        try:
            byte_read = self.readByte(PCA_REG['OUT1'])
            reset_low6 = byte_read | 0x3f
            bit_mask = self._computeLEDmask(bit_value, failure, warning)
            write_byte = reset_low6 ^ bit_mask
            logging.debug('byte_read = 0x%x, reset_low6 = 0x%x, '
                          'bit_mask = 0x%x, write_byte = 0x%x',
                          byte_read, reset_low6, bit_mask, write_byte)
            self.writeByte(PCA_REG['OUT1'], write_byte)
            return 0
        except PcaError, e:
            logging.error('Error setting PCA9555 Output Port 0: %s', e)
            return -1

    def getSwitchStatus(self):
        """Checks status of DIP Switches (2-bit).

        Returns:
          ret: an integer, error code. 0 = no error.
          status: an integer, valid value in range [0, 3].
        """
        logging.info('Attempt to read DIP switch status')
        ret = -1
        status = -1
        try:
            byte_read = self.readByte(PCA_REG['IN1'])
            # Right shift 6-bit to get 2 high-order bits
            status = byte_read >> 6
            logging.info('DIP switch status = 0x%x', status)
            ret = 0
        except PcaError, e:
            logging.error('No byte read from PCA9555 Input Port 1: %s', e)

        return (ret, status)

    def getLEDstatus(self):
        """Checks LED status.

        Returns:
          ret: an integer, 0 for success and -1 for error.
          bit_value: an integer between 0 and 15, representing 4-bit binary
                     value for green LEDs (i.e. 0~3). Default: -1.
          failure: a boolean, true = red LED has value 0. Default: False.
          warning: a boolean, true = yellow LED has value 0. Default: False.
        """
        ret = -1
        bit_value = -1
        failure = False
        warning = False

        try:
            byte_read = self.readByte(PCA_REG['OUT1'])
            if not (byte_read | PCA_BIT_ONE['red_led']):
                failure = True
            if not (byte_read | PCA_BIT_ONE['yellow_led']):
                warning = True
            bit_value = byte_read & 0xf  # Get lower 4-bit value
            logging.info('LED bit_value = %r, failure = %r, warning = %r',
                         bit_value, failure, warning)
            ret = 0
        except PcaError, e:
            logging.error('No byte read from PCA9555 Output Port 1: %s', e)

        return (ret, bit_value, failure, warning)
