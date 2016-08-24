# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''
USB to I2C controller.
'''

import glob
import logging
import os
import re
import serial
import time

from autotest_lib.client.cros import tty

# Least significant bit of I2C address.
WRITE_BIT = 0
READ_BIT = 1


def create_i2c_controller(chipset_config):
    '''Factory method for I2CController.

    This function is a factory method to create an I2CController instance.

    @param chipset_config: Chipset configuration.
    @return An I2CController if succeeded.
    @throws AssertionError if a valid instance cannot be created.
    '''
    if chipset_config.split(':')[0] == 'SC18IM700':
        usb_uart_driver = chipset_config.split(':')[1]
        # Try to find a tty terminal that driver name matches usb_uart_driver.
        tty_path = tty.find_tty_by_driver(usb_uart_driver)
        if tty_path:
            return _I2CControllerSC18IM700(tty_path)

    assert False, "Unsupported configuration: %s" % chipset_config


class I2CController(object):
    '''
    The base class of I2C controller.
    '''

    # Constants indicate I2C bus status.
    I2C_OK = 1
    I2C_NACK_ON_ADDRESS = 2
    I2C_NACK_ON_DATA = 3
    I2C_TIME_OUT = 4

    def send_and_check_status(self, slave_addr, int_array):
        '''Sends data to I2C slave device and checks the bus status.

        @param slave_addr: The address of slave in 7bits format.
        @param int_array: The data to send in integer array.
        @param status_check: Whether to check I2C bus status.

        @return An integer indicates I2C bus status.
        '''
        self.send(slave_addr, int_array)
        return self.read_bus_status()

    def read_bus_status(self):
        '''Returns the I2C bus status.'''
        raise NotImplementedError

    def send(self, slave_addr, int_array):
        '''Sends data to I2C slave device.

        Caller should call read_bus_status() explicitly to confirm whether the
        data sent successfully.

        @param slave_addr: The address of slave in 7bits format.
        @param int_array: The data to send in integer array.
        '''
        raise NotImplementedError

    def read(self, slave_addr, bytes_to_read):
        '''Reads data from I2C slave device.

        @param slave_addr: The address of slave in 7bits format.
        @param bytes_to_read: The number of bytes to read from device.
        @return An array of data.
        '''
        raise NotImplementedError


class _I2CControllerSC18IM700(I2CController):
    '''
    Implementation of I2C Controller for NXP SC18IM700.
    '''
    SEC_WAIT_I2C = 0.1

    # Constants from official datasheet.
    # http://www.nxp.com/documents/data_sheet/SC18IM700.pdf
    I2C_STATUS = {0b11110000: I2CController.I2C_OK,
                  0b11110001: I2CController.I2C_NACK_ON_ADDRESS,
                  0b11110011: I2CController.I2C_NACK_ON_DATA,
                  0b11111000: I2CController.I2C_TIME_OUT}

    def __init__(self, device_path):
        '''Connects to NXP via serial port.

        @param device_path: The device path of serial port.
        '''
        self.logger = logging.getLogger('SC18IM700')
        self.logger.info('Setup serial device... [%s]', device_path)
        self.device_path = device_path
        self.serial = serial.Serial(port=self.device_path,
                                    baudrate=9600,
                                    bytesize=serial.EIGHTBITS,
                                    parity=serial.PARITY_NONE,
                                    stopbits=serial.STOPBITS_ONE,
                                    xonxoff=False,
                                    rtscts=True,
                                    interCharTimeout=1)
        self.logger.info('pySerial [%s] configuration : %s',
                         serial.VERSION, self.serial.__repr__())
        # Clean the buffer.
        self.serial.flush()

    def _write(self, data):
        '''Converts data to bytearray and writes to the serial port.'''
        self.serial.write(bytearray(data))
        self.serial.flush()

    def _read(self):
        '''Reads data from serial port(Non-Blocking).'''
        ret = self.serial.read(self.serial.inWaiting())
        self.logger.info('Hex and binary dump of datas - ')
        for char in ret:
            self.logger.info('  %x - %s', ord(char), bin(ord(char)))
        return ret

    @staticmethod
    def _convert_to_8bits_addr(slave_addr_7bits, lsb):
        '''Converts slave_addr from 7 bits to 8 bits with given LSB.'''
        assert (slave_addr_7bits >> 7) == 0, "Address must not exceed 7 bits."
        assert (lsb & ~0x01) == 0, "lsb must not exceed one bit."
        return (slave_addr_7bits << 1) | lsb

    def read_bus_status(self):
        cmd = [ord('R'), 0x0A, ord('P')]
        self._write(cmd)
        time.sleep(self.SEC_WAIT_I2C)
        ret = self._read()
        if (len(ret) == 1) and (ord(ret[0]) in self.I2C_STATUS):
            return self.I2C_STATUS[ord(ret[0])]
        raise IOError("I2C_STATUS_READ_FAILED")

    def send(self, slave_addr, int_array):
        cmd = ([ord('S'),
                self._convert_to_8bits_addr(slave_addr, WRITE_BIT),
                len(int_array)] +
               int_array + [ord('P')])
        self._write(cmd)

    def read(self, slave_addr, bytes_to_read):
        cmd = ([ord('S'),
                self._convert_to_8bits_addr(slave_addr, READ_BIT),
                bytes_to_read,
                ord('P')])
        self._write(cmd)
        time.sleep(self.SEC_WAIT_I2C)
        return self._read()

    def write_gpio(self, data):
        self._write([ord('O'), data, ord('P')])

    def read_gpio(self):
        self._write([ord('I'), ord('P')])
        time.sleep(self.SEC_WAIT_I2C)
        return self._read()

    def write_register(self, regs, datas):
        assert len(regs) == len(datas)
        cmd = [ord('W')]
        for i in range(len(regs)):
            cmd.append(regs[i])
            cmd.append(datas[i])
        cmd.append(ord('P'))
        self._write(cmd)

    def read_register(self, regs):
        cmd = [ord('R')] + regs + [ord('P')]
        self._write(cmd)
        time.sleep(self.SEC_WAIT_I2C)
        return self._read()
