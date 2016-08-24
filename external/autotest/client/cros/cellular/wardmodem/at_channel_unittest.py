#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import at_channel

import fcntl
import functools
import glib
import logging
import mox
import os
import tempfile
import unittest

import task_loop

class ATChannelTestCase(unittest.TestCase):
    """
    Test fixture for ATChannel class.

    """

    def setUp(self):
        self.mox = mox.Mox()

        master, slave = os.openpty()
        self._at_channel = at_channel.ATChannel(
                self._recieve_command_local_callback, slave, 'test')

        # Replace the channel inside _at_channel with a tempfile
        # We will use the tempfile to simulate a tty pair.
        os.close(master)
        os.close(slave)
        self._channel_file = tempfile.TemporaryFile(mode = 'w+')
        # These properties are a copy of the properties set in ATChannel for the
        # tty pair.
        flags = fcntl.fcntl(self._channel_file.fileno(), fcntl.F_GETFL)
        flags = flags | os.O_NONBLOCK
        fcntl.fcntl(self._channel_file.fileno(), fcntl.F_SETFL, flags)
        self._at_channel._channel = self._channel_file.fileno()
        # We need to seek() to the beginning of the file to simulate tty read.
        # So remember the head of the file.
        self._channel_file_head = self._channel_file.tell()

        # Also mock out the task_loop
        self._mox_task_loop = self.mox.CreateMock(task_loop.TaskLoop)
        self._at_channel._task_loop = self._mox_task_loop


    def tearDown(self):
        self._channel_file.close()

    # ##########################################################################
    # Tests

    def test_successful_send(self):
        """
        Test that a single AT command can be sent on the channel.

        """
        payload = 'A not so huge AT+CEREG command.'
        self._at_channel.send(payload)
        received_command = self._recieve_command_remote()
        self.assertTrue(received_command.endswith('\r\n'))
        self.assertEqual(payload.strip(), received_command.strip())

        # Change the AT command guard strings and check again.
        self._at_channel.at_prefix = '$$'
        self._at_channel.at_suffix = '##'
        payload = 'A not so huge AT+CEREG command.'
        self._at_channel.send(payload)
        received_command = self._recieve_command_remote()
        self.assertTrue(received_command.startswith('$$'))
        self.assertTrue(received_command.endswith('##'))
        self.assertEqual(payload.strip(),
                         received_command.strip('$$').strip('##'))


    def test_recieve_single_at_command(self):
        """
        Test that a single AT command can be received together on the channel.

        """
        payload = 'We send you our AT+good wishes too!\r\n'
        callback = lambda channel, payload: None
        self._at_channel._receiver_callback = callback
        self._mox_task_loop.post_task(callback, payload.strip())
        self.mox.ReplayAll()
        self._send_command_remote(payload)
        self._at_channel._handle_channel_cb(self._channel_file.fileno(),
                                          glib.IO_IN)
        self.mox.VerifyAll()


    def test_receive_at_commands_differet_terminators(self):
        """
        Test that AT commands are recieved correctly when different supported
        termination strings are being used.

        """
        # ; is a continuation marker. AT1;2 == AT1\r\nAT2
        payloads = ['AT1\r\nA', 'T2\rA', 'T3\nA', 'T4;', '5\r\n']
        callback = lambda channel, payload: None
        self._at_channel._receiver_callback = callback
        self._mox_task_loop.post_task(callback, 'AT1')
        self._mox_task_loop.post_task(callback, 'AT2')
        self._mox_task_loop.post_task(callback, 'AT3')
        self._mox_task_loop.post_task(callback, 'AT4')
        self._mox_task_loop.post_task(callback, 'AT5')

        self.mox.ReplayAll()
        for payload in payloads:
            self._send_command_remote(payload)
            self._at_channel._handle_channel_cb(self._channel_file.fileno(),
                                                glib.IO_IN)
        self.mox.VerifyAll()


    def test_recieve_at_commands_in_parts(self):
        """
        Test that a multiple AT commands can be received in parts on the
        channel.

        """
        payloads = ['AT1', '11\r\n', '\r\nAT22', '2\r\nAT333', '\r\n']
        callback = lambda channel, payload: None
        self._at_channel._receiver_callback = callback
        self._mox_task_loop.post_task(callback, 'AT111')
        self._mox_task_loop.post_task(callback, 'AT222')
        self._mox_task_loop.post_task(callback, 'AT333')

        self.mox.ReplayAll()
        for payload in payloads:
            self._send_command_remote(payload)
            self._at_channel._handle_channel_cb(self._channel_file.fileno(),
                                                glib.IO_IN)
        self.mox.VerifyAll()


    def test_recieve_long_at_commands(self):
        """
        Test that a multiple AT commands can be received in parts on the
        channel.

        """
        payloads = ['AT1+',
                    '123456789\r\nAT2+123456789\r\nAT3+1234567',
                    '89\r\n']
        callback = lambda channel, payload: None
        self._at_channel._receiver_callback = callback
        self._mox_task_loop.post_task(callback, 'AT1+123456789')
        self._mox_task_loop.post_task(callback, 'AT2+123456789')
        self._mox_task_loop.post_task(callback, 'AT3+123456789')

        self.mox.ReplayAll()
        at_channel.CHANNEL_READ_CHUNK_SIZE = 4
        for payload in payloads:
            self._send_command_remote(payload)
            self._at_channel._handle_channel_cb(self._channel_file.fileno(),
                                                glib.IO_IN)
        self.mox.VerifyAll()

    # ##########################################################################
    # Helper functions

    def _clean_channel_file(self):
        """
        Clean the tempfile used to simulate tty, and reset the r/w head.

        """
        self._channel_file.truncate(0)
        self._channel_file_head = self._channel_file.tell()


    def _send_command_remote(self, payload):
        """
        Simulate a command being sent from the remote tty port.

        @param payload: The command to send.

        """
        self._clean_channel_file()
        self._channel_file.write(payload)
        self._channel_file.flush()
        self._channel_file.seek(self._channel_file_head)


    def _recieve_command_remote(self):
        """
        Simluate a command being received at the remote tty port.

        """
        self._channel_file.flush()
        self._channel_file.seek(self._channel_file_head)
        payload_list = []
        for buf in iter(functools.partial(self._channel_file.read, 128), ''):
            payload_list.append(buf)
        self._clean_channel_file()
        return ''.join(payload_list)


    def _recieve_command_local_callback(self, payload):
        pass


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
