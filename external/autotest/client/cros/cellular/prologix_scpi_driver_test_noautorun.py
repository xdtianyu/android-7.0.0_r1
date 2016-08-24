#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import copy
import mock
import prologix_scpi_driver
import scpi
import unittest
import cellular_logging
import cellular_system_error

log = cellular_logging.SetupCellularLogging('scpi_test')

# TODO:(byronk):
# a hack for now. Should look this up in labconfig_data. crbug.com/225108
# TODO:(byronk):
# replace SystemError with a specific exception crbug.com/225127

scpi_instruments = [
    # Agilent 8960 call box
    {'name_part': "8960", 'gpib_addr': '14', 'ip': '172.22.50.118'},
    # PXT is called 6621
    {'name_part': "6621", 'gpib_addr': '14', 'ip': "172.22.50.244"}
]


class BasicPrologixTest(unittest.TestCase):
    """
    Basic connection test
    """

    def test_bad_ip_address(self):
        """
        Connect to the wrong port and check for the right error message.
        """
        instr = copy.copy(scpi_instruments[0])
        instr['ip'] = '192.168.0.0'  # str(int(instr['gpib_addr'])+1)
        log.debug(instr)
        with self.assertRaises(Exception) as ex:
            self._get_idns_and_verify(instruments=[instr], opc=True)
        self.assertIsInstance(ex.exception,
                              cellular_system_error.SocketTimeout)

    def test_ConnectToPortSuccess(self):
        """ Make a socket connection """
        s = scpi_instruments[0]
        prologix_scpi_driver.connect_to_port(s['ip'], 1234, 5)

    def test_ConnectToPortBadIP(self):
        """ Make a socket connection """
        with self.assertRaises(Exception) as ex:
            prologix_scpi_driver.connect_to_port('192.168.255.111', 1234, 1)
        self.assertIsInstance(ex.exception,
                              cellular_system_error.SocketTimeout)

    def test_BadGpibAddress(self):
        """
        How does the code behave if we can't connect.
        """
        instr = copy.copy(scpi_instruments[0])
        instr['gpib_addr'] = 9  # str(int(instr['gpib_addr'])+1)
        with self.assertRaises(Exception) as ex:
            self._get_idns_and_verify(instruments=[instr], opc=True)
        self.assertIsInstance(ex.exception,
                              cellular_system_error.InstrumentTimeout)

    @mock.patch.object(prologix_scpi_driver.PrologixScpiDriver, '_DirectQuery')
    def test_NonClearReadBufferBeforeInit(self, patched_driver):
        """
        Sometimes the Prologix box will have junk in it's read buffer
        There is code to read the junk out until setting the ++addr works.
        Test that here.
        """
        s = scpi_instruments[0]
        patched_driver.side_effect = ['junk1', 'junk2', s['gpib_addr']]
        driver = prologix_scpi_driver.PrologixScpiDriver(
            hostname=s['ip'],
            port=1234,
            gpib_address=s['gpib_addr'],
            read_timeout_seconds=2)

    def test_Reset(self):
        for instr in scpi_instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=True,
                                                  read_timeout_seconds=20)
            scpi_connection.Reset()
            self.scpi_connection.Close()

    def test_SimpleVerify(self):
        """
        call SimpleVerify.
        """
        # TODO(byronk): make sure this test only runs on the 8960. This
        # command doesn't work on other boxes
        for instr in scpi_instruments[:1]:
            assert instr['name_part'] == '8960'
            scpi_connection = self._open_prologix(instr, opc_on_stanza=True,
                                                  read_timeout_seconds=2)
            # Check to see if the power state is off.
            # setting most instrument to off should be ok.
            scpi_connection.SimpleVerify('call:ms:pow:targ', '+0')
            self.scpi_connection.Close()

    def test_FetchErrors(self):
        """
        call FetchErrors
        """
        for instr in scpi_instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=True,
                                                  read_timeout_seconds=2)
            scpi_connection._WaitAndFetchErrors()
            self.scpi_connection.Close()

    def test_BadScpiCommand(self):
        """
        Send a bad command. We should fail gracefully.
        """
        for instr in scpi_instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=True,
                                                  read_timeout_seconds=1)
            try:
                scpi_connection.Query('*IDN')
            except cellular_system_error.InstrumentTimeout:
                assert \
                 "Should have raised a Instrument Timeout on a bad SCPI command"

    def test_ErrorCheckerContextAndStanzaSendingOpcFalse(self):
        """
        Send a stanza, which uses the context manager
        """
        for instr in scpi_instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=False,
                                                  read_timeout_seconds=5)
            scpi_connection.SendStanza(['*WAI'])
            scpi_connection.Close()

    def test_ErrorCheckerContextAndStanzaSendingOpcTrue(self):
        """
        Send a stanza, which uses the context manager
        """
        for instr in scpi_instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=True,
                                                  read_timeout_seconds=5)
            scpi_connection.SendStanza(['*WAI'])
            scpi_connection.Close()

    def test_GetIdnOpcTrue(self):
        """
        Test with opc True. OPC= operation complete. Asking this
        question *OPC? after commands blocks until the command finishes.
        This prevents us from sending commands faster then then the
        instrument can handle. True is usually the right setting.

        """
        self._get_idns_and_verify(instruments=scpi_instruments, opc=True)

    def test_GetIdnOpcFalse(self):
        """
        Now with OPC off.
        """
        self._get_idns_and_verify(instruments=scpi_instruments, opc=False)

    def _open_prologix(self, instr, opc_on_stanza, read_timeout_seconds=2):
        """
        Build the prologix object.
        """
        ip_addr = instr['ip']
        name_part = instr['name_part']
        gpib_addr = instr['gpib_addr']
        log.debug("trying %s at %s" % (name_part, ip_addr))
        driver = prologix_scpi_driver.PrologixScpiDriver(
            hostname=ip_addr,
            port=1234,
            gpib_address=gpib_addr,
            read_timeout_seconds=read_timeout_seconds)
        self.scpi_connection = scpi.Scpi(driver)
        log.debug("setting opc to %s " % opc_on_stanza)
        self.scpi_connection.opc_on_stanza = opc_on_stanza
        return self.scpi_connection

    def _get_idns_and_verify(self, instruments, opc=False):
        """
        Get the idn string from all the instruments, and check that it
        contains the desired substring. This is a quick sanity check only.
        """
        for instr in instruments:
            scpi_connection = self._open_prologix(instr, opc_on_stanza=opc)
            response = scpi_connection.Query('*IDN?')
            log.debug("looking for %s  in response string: %s " %
                      (instr['name_part'], response))
            assert instr['name_part'] in response
            self.scpi_connection.Close()

if __name__ == '__main__':
    unittest.main()
