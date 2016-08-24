# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A base class to interact with I2C slave device.

Dependency
 - This library depends on a new C shared library called "libsmogcheck.so".
"""

import ctypes, logging


# I2C constants
I2C_BUS = 2

# Path of shared library.
SMOGCHECK_C_LIB = "/usr/local/lib/libsmogcheck.so.0"


class I2cError(Exception):
    """Base class for all errors in this module."""


class I2cSlave(object):
    """A generic I2C slave object that supports basic I2C bus input/output."""

    def __init__(self, adapter_nr=None, load_lib=None):
        """Constructor.

        Mandatory params:
          adapter_nr: adapter's number address. Default: I2C_BUS.
          fd: file descriptor to communicate with I2C bus.
          lib_obj: ctypes library object to interface with SMOGCHECK_C_LIB.
          load_lib: a string, name of C shared library object to load.
          slave_addr: slave address to set. Default: None.

        Args:
          lib: a string, name of C shared library object to load.
        """
        self.slave_addr = None

        if adapter_nr is None:
            adapter_nr = I2C_BUS
        self.adapter_nr = adapter_nr

        if load_lib is None:
            load_lib = SMOGCHECK_C_LIB
        self.load_lib = load_lib

        # Load shared library object.
        self.lib_obj = self._loadSharedLibrary()
        self.fd = self._getDeviceFile()

    def _loadSharedLibrary(self):
        """Loads C shared library .so file.

        Returns:
          a new instance of the shared (C) library.

        Raises:
          I2cError: if error loading the shared library.
        """
        logging.info('Attempt to load shared library %s', self.load_lib)
        try:
            return ctypes.cdll.LoadLibrary(self.load_lib)
        except OSError, e:
            raise I2cError('Error loading C library %s: %s' %
                            (self.load_lib, e))
        logging.info('Successfully loaded shared library %s', self.load_lib)

    def _getDeviceFile(self):
        """Gets a file descriptor of a device file.

        Returns:
          fd: an integer, file descriptor to communicate with I2C bus.

        Raises:
          I2cError: if error getting device file.
        """
        logging.info('Attempt to get device file for adapter %s',
                     self.adapter_nr)
        fd = self.lib_obj.GetDeviceFile(self.adapter_nr)
        if fd < 0:
            raise I2cError('Error getting device file for adapter %s' %
                            self.adapter_nr)

        logging.info('Got device file for adapter %s', self.adapter_nr)
        return fd

    def setSlaveAddress(self, addr):
        """Sets slave address on I2C bus to be communicated with.

        TODO(tgao): add retry loop and raise error if all retries fail.
        (so that caller does not have to check self.err for status)

        We use 7-bit address space for I2C, which has 128 addresses total.
        Besides 16 reserved addresses, the total usable address space is 112.
        See - http://www.i2c-bus.org/addressing/

        Args:
          addr: a (positive) integer, 7-bit I2C slave address.

        Raises:
          I2cError: if slave address is invalid or can't be set.
        """
        if self.slave_addr == addr:
            logging.info('Slave address already set, noop: %s', addr)
            return

        if addr < 0x8 or addr > 0x77:
            raise I2cError('Error: invalid I2C slave address %s', addr)

        logging.info('Attempt to set slave address: %s', addr)
        if not self.fd:
            self.fd = self._getDeviceFile()

        ret = self.lib_obj.SetSlaveAddress(self.fd, addr)
        if ret < 0:
            raise I2cError('Error communicating to slave address %s' % addr)

        self.slave_addr = addr
        logging.info('Slave address set to: %s', addr)

    def writeByte(self, reg, byte):
        """Writes a byte to a specific register.

        TODO(tgao): add retry loop and raise error if all retries fail.

        Args:
          reg: a (positive) integer, register number.
          byte: a char (8-bit byte), value to write.

        Raises:
          I2cError: if error writing byte to I2C bus.
        """
        logging.info('Attempt to write byte %r to reg %r', byte, reg)
        if self.lib_obj.WriteByte(self.fd, reg, byte) < 0:
            raise I2cError('Error writing byte 0x%x to reg %r' % (byte, reg))

        logging.info('Successfully wrote byte 0x%x to reg %r', byte, reg)

    def readByte(self, reg):
        """Reads a byte from a specific register.

        TODO(tgao): add retry loop and raise error if all retries fail.

        Args:
          reg: a (positive) integer, register number.

        Returns:
          byte_read: a char (8-bit byte), value read from register.

        Raises:
          I2cError: if error reading byte from I2C bus.
        """
        logging.info('Attempt to read byte from register %r', reg)
        byte_read = self.lib_obj.ReadByte(self.fd, reg)
        if byte_read < 0:
            raise I2cError('Error reading byte from reg %r' % reg)

        logging.info('Successfully read byte 0x%x from reg %r',
                     byte_read, reg)
        return byte_read

    def writeWord(self, reg, word):
        """Writes a word to a specific register.

        TODO(tgao): add retry loop and raise error if all retries fail.

        Args:
          reg: a (positive) integer, register number.
          word: a 16-bit unsigned integer, value to write.

        Raises:
          I2cError: if error writing word to I2C bus.
        """
        logging.info('Attempt to write word %r to reg %r', word, reg)
        if self.lib_obj.WriteWord(self.fd, reg, ctypes.c_uint16(word)) < 0:
            raise I2cError('Error writing word 0x%x to reg %r' % (word, reg))

        logging.info('Successfully wrote word 0x%x to reg %r',
                     word, reg)

    def readWord(self, reg):
        """Reads a word from a specific register.

        TODO(tgao): add retry loop and raise error if all retries fail.

        Args:
          reg: a (positive) integer, register number.

        Returns:
          a 16-bit unsigned integer, value read from register.

        Raises:
          I2cError: if error reading word from I2C bus.
        """
        logging.info('Attempt to read word from register %r', reg)
        word_read = self.lib_obj.ReadWord(self.fd, reg)
        if word_read < 0:
            raise I2cError('Error reading word from reg %r' % reg)

        logging.info('Successfully read word 0x%x from reg %r',
                     word_read, reg)
        return word_read
