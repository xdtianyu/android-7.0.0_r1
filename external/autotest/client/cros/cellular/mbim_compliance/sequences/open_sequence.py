# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import struct
from usb import control

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import sequence

# The maximun datagram size used in SetMaxDatagramSize request.
MAX_DATAGRAM_SIZE = 1514


#TODO(rpius): Move to a more appropriate location. Maybe a utility file?
class NtbParameters(object):
    """ The class for NTB Parameter Structure. """

    _FIELDS = [('H','wLength'),
               ('H','bmNtbFormatsSupported'),
               ('I','dwNtbInMaxSize'),
               ('H','wNdpInDivisor'),
               ('H','wNdpInPayloadRemainder'),
               ('H','wNdpInAlignment'),
               ('H','reserved'),
               ('I','dwNtbOutMaxSize'),
               ('H','wNdpOutDivisor'),
               ('H','wNdpOutPayloadRemainder'),
               ('H','wNdpOutAlignment'),
               ('H','wNtbOutMaxDatagrams')]


    def __init__(self, *args):
        _, field_names = zip(*self._FIELDS)
        if len(args) != len(field_names):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceError,
                    'Expected %d arguments for %s constructor, got %d.' % (
                            len(field_names),self.__class__.__name__,len(args)))

        fields = zip(field_names, args)
        for field in fields:
            setattr(self, field[0], field[1])


    @classmethod
    def get_format_string(cls):
        """
        @returns The format string composed of concatenated field formats.
        """
        field_formats, _ = zip(*cls._FIELDS)
        return ''.join(field_format for field_format in field_formats)


class OpenSequence(sequence.Sequence):
    """ Base case for all MBIM open sequneces. """

    def set_alternate_setting(self, interface_number, alternate_setting):
        """
        Set alternate setting to |alternate_setting| for the target interface.

        @param inteface_number: the index of target interface
        @param alternate_setting: expected value of alternate setting

        """
        logging.debug('SetInterface request: %d to interface-%d.',
                      alternate_setting, interface_number)
        control.set_interface(self.device_context.device,
                              interface_number,
                              alternate_setting)


    def reset_function(self, interface_number):
        """
        Send ResetFunction() request to the target interface.

        @param interface_number: the index of target interface

        """
        logging.debug('ResetFunction request to interface-%d.',
                      interface_number)
        self.device_context.device.ctrl_transfer(bmRequestType=0b00100001,
                                                 bRequest=0x05,
                                                 wValue=0,
                                                 wIndex=interface_number,
                                                 data_or_wLength=None)


    def get_ntb_parameters(self, interface_number):
        """
        Retrieve NTB parameters of the target interface.

        @param interface_number: the index of target interface
        @returns NTB parameters in byte stream.

        """
        logging.debug('GetNtbParameters request to interface-%d.',
                      interface_number)
        ntb_parameters = self.device_context.device.ctrl_transfer(
                bmRequestType=0b10100001,
                bRequest=0x80,
                wValue=0,
                wIndex=interface_number,
                data_or_wLength=28)
        logging.debug('Response: %s', ntb_parameters)
        format_string = NtbParameters.get_format_string()
        return NtbParameters(
                *struct.unpack_from('<' + format_string, ntb_parameters))


    def set_ntb_format(self, interface_number, ntb_format):
        """
        Send SetNtbFormat() request to the target interface.

        @param interface_number: the index of target interface
        @param ntb_format: The NTB format should be either |NTB_16| or |NTB_32|.

        """
        logging.debug('SetNtbFormat request: %d to interface-%d.',
                      ntb_format, interface_number)
        response = self.device_context.device.ctrl_transfer(
                bmRequestType=0b00100001,
                bRequest=0x84,
                wValue=ntb_format,
                wIndex=interface_number,
                data_or_wLength=None)
        logging.debug('Response: %s', response)


    def get_ntb_format(self, interface_number):
        """
        Send GetNtbFormat() request to the target interface.

        @param interface_number: the index of target interface
        @returns ntb_format: The NTB format currently set.

        """
        logging.debug('GetNtbFormat request to interface-%d.',
                      interface_number)
        response = self.device_context.device.ctrl_transfer(
                bmRequestType=0b10100001,
                bRequest=0x83,
                wValue=0,
                wIndex=interface_number,
                data_or_wLength=2)
        logging.debug('Response: %s', response)
        return response


    def set_ntb_input_size(self, interface_number, dw_ntb_in_max_size):
        """
        Send SetNtbInputSize() request to the target interface.

        @param interface_number:the index of target interface
        @param dw_ntb_in_max_size: The maxinum NTB size to set.

        """
        logging.debug('SetNtbInputSize request: %d to interface-%d.',
                      dw_ntb_in_max_size, interface_number)
        data = struct.pack('<I', dw_ntb_in_max_size)
        response = self.device_context.device.ctrl_transfer(
                bmRequestType=0b00100001,
                bRequest=0x86,
                wIndex=interface_number,
                data_or_wLength=data)
        logging.debug('Response: %s', response)


    def set_max_datagram_size(self, interface_number):
        """
        Send SetMaxDatagramSize() request to the target interface.

        @param interface_number: the index of target interface

        """
        logging.debug('SetMaxDatagramSize request: %d to interface-%d.',
                      MAX_DATAGRAM_SIZE, interface_number)
        data = struct.pack('<H', MAX_DATAGRAM_SIZE)
        response = self.device_context.device.ctrl_transfer(
                bmRequestType=0b00100001,
                bRequest=0x88,
                wIndex=interface_number,
                data_or_wLength=data)
        logging.debug('Response: %s', response)
