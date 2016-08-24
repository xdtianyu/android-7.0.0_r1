# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Python library to interact with INA219 module for TPM testing.

Background
 - INA219 is one of two modules on TTCI board
 - This library provides methods to interact with INA219 programmatically

Dependency
 - This library depends on a new C shared library called "libsmogcheck.so".
 - In order to run test cases built using this API, one needs a TTCI board

Notes:
 - An exception is raised if it doesn't make logical sense to continue program
   flow (e.g. I/O error prevents test case from executing)
 - An exception is caught and then converted to an error code if the caller
   expects to check for error code per API definition
"""

import logging, re
from autotest_lib.client.common_lib import i2c_slave


# INA219 registers
INA_REG = {
    'CONF': 0,        # Configuration Register
    'SHUNT_VOLT': 1,  # Shunt Voltage
    'BUS_VOLT': 2,    # Bus Voltage
    'POWER': 3,       # Power
    'CURRENT': 4,     # Current
    'CALIB': 5,       # Calibration
    }

# Regex pattern for measurement value
HEX_STR_PATTERN = re.compile('^0x([0-9a-f]{2})([0-9a-f]{2})$')

# Constants used to initialize INA219 registers
# TODO(tgao): add docstring for these values after stevenh replies
INA_CONF_INIT_VAL = 0x9f31
INA_CALIB_INIT_VAL = 0xc90e

# Default values used to calculate/interpret voltage and current measurements.
DEFAULT_MEAS_RANGE_VALUE = {
    'current': {'max': 0.1, 'min': 0.0, 'denom': 10000.0,
                'reg': INA_REG['CURRENT']},
    'voltage': {'max': 3.35, 'min': 3.25, 'denom': 2000.0,
                'reg': INA_REG['BUS_VOLT']},
    }


class InaError(Exception):
    """Base class for all errors in this module."""


class InaController(i2c_slave.I2cSlave):
    """Object to control INA219 module on TTCI board."""

    def __init__(self, slave_addr=None, range_dict=None):
        """Constructor.

        Mandatory params:
          slave_addr: slave address to set. Default: None.

        Optional param:
          range_dict: desired max/min thresholds for measurement values.
                      Default: DEFAULT_MEAS_RANGE_VALUE.

        Args:
          slave_addr: an integer, address of main or backup power.
          range_dict: desired max/min thresholds for measurement values.

        Raises:
          InaError: if error initializing INA219 module or invalid range_dict.
        """
        super(InaController, self).__init__()
        if slave_addr is None:
            raise InaError('Error slave_addr expected')

        try:
            if range_dict is None:
                range_dict = DEFAULT_MEAS_RANGE_VALUE
            else:
                self._validateRangeDict(DEFAULT_MEAS_RANGE_VALUE, range_dict)
            self.range_dict = range_dict

            self.setSlaveAddress(slave_addr)
            self.writeWord(INA_REG['CONF'], INA_CONF_INIT_VAL)
            self.writeWord(INA_REG['CALIB'], INA_CALIB_INIT_VAL)
        except InaError, e:
            raise InaError('Error initializing INA219: %s' % e)

    def _validateRangeDict(self, d_ref, d_in):
        """Validates keys and types of value in range_dict.

        Iterate over d_ref to make sure all keys exist in d_in and
        values are of the correct type.

        Args:
          d_ref: a dictionary, used as reference.
          d_in: a dictionary, to be validated against reference.

        Raises:
          InaError: if range_dict is invalid.
        """
        for k, v in d_ref.iteritems():
            if k not in d_in:
                raise InaError('Key %s not present in dict %r' % (k, d_in))
            if type(v) != type(d_in[k]):
                raise InaError(
                    'Value type mismatch for key %s. Expected: %s; actual = %s'
                    % (k, type(v), type(d_in[k])))
            if type(v) is dict:
                self._validateRangeDict(v, d_in[k])

    def readMeasure(self, measure):
        """Reads requested measurement.

        Args:
          measure: a string, 'current' or 'voltage'.

        Returns:
          a float, measurement in native units. Or None if error.

        Raises:
          InaError: if error reading requested measurement.
        """
        try:
            hex_str = '0x%.4x' % self.readWord(self.range_dict[measure]['reg'])
            logging.debug('Word read = %r', hex_str)
            return self._checkMeasureRange(hex_str, measure)
        except InaError, e:
            logging.error('Error reading %s: %s', measure, e)

    def getPowerMetrics(self):
        """Get measurement metrics for Main Power.

        Returns:
          an integer, 0 for success and -1 for error.
          a float, voltage value in Volts. Or None if error.
          a float, current value in Amps. Or None if error.
        """
        logging.info('Attempt to get power metrics')
        try:
            return (0, self.readMeasure('voltage'),
                    self.readMeasure('current'))
        except InaError, e:
            logging.error('getPowerMetrics(): %s', e)
            return (-1, None, None)

    def _checkMeasureRange(self, hex_str, measure):
        """Checks if measurement value falls within a pre-specified range.

        Args:
          hex_str: a string (hex value).
          measure: a string, 'current' or 'voltage'.

        Returns:
          measure_float: a float, measurement value.

        Raises:
          InaError: if value doesn't fall in range.
        """
        measure_float = self._convertHexToFloat(
            hex_str, self.range_dict[measure]['denom'])
        measure_msg = '%s value %.2f' % (measure, measure_float)
        range_msg = '[%(min).2f, %(max).2f]' % self.range_dict[measure]
        if (measure_float < self.range_dict[measure]['min'] or
            measure_float > self.range_dict[measure]['max']):
            raise InaError('%s is out of range %s' % measure_msg, range_msg)
        logging.info('%s is in range %s', measure_msg, range_msg)
        return measure_float

    def _convertHexToFloat(self, hex_str, denom):
        """Performs measurement calculation.

        The measurement reading from INA219 module is a 2-byte hex string.
        To convert this hex string to a float, we need to swap these two bytes
        and perform a division. An example:
          response = 0xca19
          swap bytes to get '0x19ca'
          convert to decimal value = 6602
          divide decimal by 2000.0 = 3.301 (volts)

        Args:
          hex_str: a string (raw hex value).
          denom: a float, denominator used for hex-to-float conversion.

        Returns:
          a float, measurement value.

        Raises:
          InaError: if error converting measurement to float.
        """
        match = HEX_STR_PATTERN.match(hex_str)
        if not match:
            raise InaError('Error: hex string %s does not match '
                           'expected pattern' % hex_str)

        decimal = int('0x%s%s' % (match.group(2), match.group(1)), 16)
        return decimal/denom
