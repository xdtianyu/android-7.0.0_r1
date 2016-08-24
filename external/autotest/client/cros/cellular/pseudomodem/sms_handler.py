# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import dbus
import dbus.types
import logging

import sms

from autotest_lib.client.cros.cellular import mm1_constants

class SmsHandlerException(Exception):
    """ Exception class for errors raised by SmsHandler. """
    pass


class SmsHandler(object):
    """
    Handles all SMS operations, which includes storing received SMS messages,
    as well as notifying the modem when a new SMS is received.

    """

    # TODO(armansito): Apply a character limit to SMS messages for multi-part
    # delivery. This constant here is defined for future reference but is
    # currently unusued. The value that is currently assigned to it is
    # arbitrary and a more meaningful default value should be used, though it
    # really doesn't matter from a testing perspective.
    SMS_CHAR_LIMIT = 200

    def __init__(self, modem, bus=None):
        self._modem = modem
        self._messages = {}  # Mapping from DBus Object paths to sms.SMS.
        self._bus = bus


    @property
    def bus(self):
        """
        Returns the current bus assigned to this object. This is the bus
        on which new SMS objects will be created.

        @returns: An instance of dbus.Bus.

        """
        return self._bus


    @bus.setter
    def bus(self, bus):
        """
        Sets the current bus on which SMS objects should be created.

        @param bus: An instance of dbus.Bus.

        """
        self._bus = bus


    @classmethod
    def set_char_limit(cls, limit):
        cls.SMS_CHAR_LIMIT = limit


    def clear_messages(self):
        """ Clears all SMS messages. """
        self._messages.clear()


    def delete_message(self, path):
        """
        Removes the message with DBus object path |path|. This operation
        has no effect if and SMS object with path |path| is unknown.

        @param path: DBus object path of the SMS object to remove.

        """
        try:
            self._messages.pop(path)
        except KeyError:
            logging.info('SMS object with path "%s" not found.', path)
            pass


    def list_messages(self):
        """
        Returns a list of DBus object paths belonging to stored SMS messages.

        """
        return self._messages.keys()


    def get_message_with_path(self, path):
        """
        Returns the SMS message with the DBus object path that matches |path|.

        @param path: DBus object path of the requested SMS object.
        @returns: An instance of sms.SMS or None, if an SMS object with the
                requested path is not found.

        """
        sms_object = self._messages.get(path, None)
        if sms_object:
            assert sms_object.path == path
        return sms_object


    def construct_sms(self, text, sender):
        """
        Constructs an SMS object and stores it internally. SMS messages should
        be created using this method instead of being instantiated directly.
        Once an SMS is created, it can be obtained via get_message_with_path.

        @param text: The message contents, in UTF-8 format.
        @param sender: The phone number of the sender.
        @returns: An instance of sms.SMS.

        """
        if self._bus is None:
            raise SmsHandlerException('A bus has to be set before SMS objects '
                                      'can be created.')
        sms_object = sms.SMS(self._bus, sender, text)
        self._messages[sms_object.path] = sms_object
        # TODO(armansito): Split SMSs that are too big into multiple chunks.
        return sms_object


    def send_sms(self, text, sender):
        """
        Queues up an SMS to be sent and simulates SMS delivery state updates.

        @param text: The message contents, in UTF-8 format.
        @param sender: The phone number of the sender.

        """
        # TODO(armansito): Support this if it's ever needed (unlikely).
        raise SmsHandlerException('Sending SMSs is not supported.')


    def receive_sms(self, text, sender, is_status_report=False):
        """
        Simulates a received SMS message.

        @param text: The message contents, in UTF-8 format.
        @param sender: The phone number of the sender.
        @param is_status_report: If True, the SMS will be formatted as a status
                report.

        """
        sms_object = self.construct_sms(text, sender)

        # Use the current time for both DischargeTimestamp and Timestamp. Our
        # SMS messages travel faster than the speed of light.
        timestamp = datetime.datetime.isoformat(datetime.datetime.now())
        sms_object.Set(mm1_constants.I_SMS, 'Timestamp', timestamp)
        sms_object.Set(mm1_constants.I_SMS, 'DischargeTimestamp', timestamp)

        # Receive messages right away.
        sms_object.Set(mm1_constants.I_SMS, 'State',
                       mm1_constants.MM_SMS_STATE_RECEIVED)
        sms_object.Set(mm1_constants.I_SMS, 'PduType',
                       mm1_constants.MM_SMS_PDU_TYPE_DELIVER)

        # Emit an Added message.
        self._modem.Added(dbus.types.ObjectPath(sms_object.path), True)
