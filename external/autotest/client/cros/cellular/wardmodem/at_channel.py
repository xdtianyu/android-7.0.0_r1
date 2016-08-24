# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fcntl
import glib
import logging
import os
import re
import termios
import tty

import task_loop

class ATChannel(object):
    """
    Send a single AT command in either direction asynchronously.

    This class represents the AT command channel. The program can
      (1) Request *one* AT command to be sent on the channel.
      (2) Get notified of a received AT command.

    """

    CHANNEL_READ_CHUNK_SIZE = 128

    GLIB_CB_CONDITION_STR = {
        glib.IO_IN: 'glib.IO_IN',
        glib.IO_OUT: 'glib.IO_OUT',
        glib.IO_PRI: 'glib.IO_PRI',
        glib.IO_ERR: 'glib.IO_ERR',
        glib.IO_HUP: 'glib.IO_HUP'
    }

    # And exception with error code 11 is raised when a write to some file
    # descriptor fails because the channel is full.
    IO_ERROR_CHANNEL_FULL = 11

    def __init__(self, receiver_callback, channel, channel_name='',
                 at_prefix='', at_suffix='\r\n'):
        """
        @param receiver_callback: The callback function to be called when an AT
                command is received over the channel. The signature of the
                callback must be

                def receiver_callback(self, command)

        @param channel: The file descriptor for channel, as returned by e.g.
                os.open().

        @param channel_name: [Optional] Name of the channel to be used for
                logging.

        @param at_prefix: AT commands sent out on this channel will be prefixed
                with |at_prefix|. Default ''.

        @param at_suffix: AT commands sent out on this channel will be
                terminated with |at_suffix|. Default '\r\n'.

        @raises IOError if some file operation on |channel| fails.

        """
        super(ATChannel, self).__init__()
        assert receiver_callback and channel

        self._receiver_callback = receiver_callback
        self._channel = channel
        self._channel_name = channel_name
        self._at_prefix = at_prefix
        self._at_suffix = at_suffix

        self._logger = logging.getLogger(__name__)
        self._task_loop = task_loop.get_instance()
        self._received_command = ''  # Used to store partially received command.

        flags = fcntl.fcntl(self._channel, fcntl.F_GETFL)
        flags = flags | os.O_RDWR | os.O_NONBLOCK
        fcntl.fcntl(self._channel, fcntl.F_SETFL, flags)
        try:
            tty.setraw(self._channel, tty.TCSANOW)
        except termios.error as ttyerror:
            raise IOError(ttyerror.args)

        # glib does not raise errors, merely prints to stderr.
        # If we've come so far, assume channel is well behaved.
        self._channel_cb_handler = glib.io_add_watch(
                self._channel,
                glib.IO_IN | glib.IO_PRI | glib.IO_ERR | glib.IO_HUP,
                self._handle_channel_cb,
                priority=glib.PRIORITY_HIGH)


    @property
    def at_prefix(self):
        """ The string used to prefix AT commands sent on the channel. """
        return self._at_prefix


    @at_prefix.setter
    def at_prefix(self, value):
        """
        Set the string to use to prefix AT commands.

        This can vary by the modem being used.

        @param value: The string prefix.

        """
        self._logger.debug('AT command prefix set to: |%s|', value)
        self._at_prefix = value


    @property
    def at_suffix(self):
        """ The string used to terminate AT commands sent on the channel. """
        return self._at_suffix


    @at_suffix.setter
    def at_suffix(self, value):
        """
        Set the string to use to terminate AT commands.

        This can vary by the modem being used.

        @param value: The string terminator.

        """
        self._logger.debug('AT command suffix set to: |%s|', value)
        self._at_suffix = value


    def __del__(self):
        glib.source_remove(self._channel_cb_handler)


    def send(self, at_command):
        """
        Send an AT command on the channel.

        @param at_command: The AT command to send.

        @return: True if send was successful, False if send failed because the
                channel was full.

        @raises: OSError if send failed for any reason other than that the
                channel was full.

        """
        at_command = self._prepare_for_send(at_command)
        try:
            os.write(self._channel, at_command)
        except OSError as write_error:
            if write_error.args[0] == self.IO_ERROR_CHANNEL_FULL:
                self._logger.warning('%s Send Failed: |%s|',
                                     self._channel_name, repr(at_command))
                return False
            raise write_error

        self._logger.debug('%s Sent: |%s|',
                           self._channel_name, repr(at_command))
        return True


    def _process_received_command(self):
        """
        Process a command from the channel once it has been fully received.

        """
        self._logger.debug('%s Received: |%s|',
                           self._channel_name, repr(self._received_command))
        self._task_loop.post_task(self._receiver_callback,
                                  self._received_command)


    def _handle_channel_cb(self, channel, cb_condition):
        """
        Callback used by the channel when there is any data to read.

        @param channel: The channel which issued the signal.

        @param cb_condition: one of glib.IO_* conditions that caused the signal.

        @return: True, so as to continue watching the channel for further
                signals.

        """
        if channel != self._channel:
            self._logger.warning('%s Signal received on unknown channel. '
                                 'Expected: |%d|, obtained |%d|. Ignoring.',
                                 self._channel_name, self._channel, channel)
            return True
        if cb_condition == glib.IO_IN or cb_condition == glib.IO_PRI:
            self._read_channel()
            return True
        self._logger.warning('%s Unexpected cb condition %s received. Ignored.',
                             self._channel_name,
                             self.GLIB_CB_CONDITION_STR[cb_condition])
        return True


    def _read_channel(self):
        """
        Read data from channel when the channel indicates available data.

        """
        incoming_list = []
        try:
            while True:
                s = os.read(self._channel, self.CHANNEL_READ_CHUNK_SIZE)
                if not s:
                    break
                incoming_list.append(s)
        except OSError as read_error:
            if not read_error.args[0] == self.IO_ERROR_CHANNEL_FULL:
                raise read_error
        if not incoming_list:
            return
        incoming = ''.join(incoming_list)
        if not incoming:
            return

        # TODO(pprabhu) Currently, we split incoming AT commands on '\r' or
        # '\n'. It may be that some modems that expect the terminator sequence
        # to be '\r\n' send spurious '\r's on the channel. If so, we must ignore
        # spurious '\r' or '\n'.

        # (1) replace ; by \rAT.
        # ';' can be used to string together AT commands.
        # So
        #  AT1;2
        # is the same as sending two commands:
        #  AT1
        #  AT2
        incoming = re.sub(';', '\rAT', incoming)

        # (2) Replace any occurence of a terminator with '\r\r'.
        # This ensures that splitting at the terminator actually gives us an
        # empty part. viz --
        #  'some_string\nother_string' --> 'some_string\r\rother_string'
        #  --> ['some_string', '', 'other_string']
        # We use the empty string generated to detect completed commands.
        incoming = re.sub('\r|\n|;', '\r\r', incoming)

        # (3) Split into AT commands.
        parts = re.split('\r', incoming)
        for part in parts:
            if (not part) and self._received_command:
                self._process_received_command()
                self._received_command = ''
            elif part:
                self._received_command = self._received_command + part


    def _prepare_for_send(self, command):
        """
        Sanitize AT command before sending on channel.

        @param command: The command to sanitize.

        @reutrn: The sanitized command.

        """
        command = command.strip()
        assert command.find('\r') == -1
        assert command.find('\n') == -1
        command = self.at_prefix + command + self.at_suffix
        return command
