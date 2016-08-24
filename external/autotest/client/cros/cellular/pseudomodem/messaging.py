# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service

import utils

from autotest_lib.client.cros.cellular import mm1_constants

# TODO(armansito): Have this class implement all Messaging methods
# and make Modems have a reference to an instance of Messaging
# OR have Modem implement this

class Messaging(dbus.service.Interface):
    """
    Python binding for the org.freedesktop.ModemManager1.Modem.Messaging
    interface. The Messaging interfaces handles sending SMS messages and
    notification of new incoming messages.

    """

    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM_MESSAGING, out_signature='ao')
    def List(self):
        """
        Retrieves all SMS messages.
        This method should only be used once and subsequent information
        retrieved either by listening for the "Added" and "Completed" signals,
        or by querying the specific SMS object of interest.

        @returns: The list of SMS object paths.

        """
        raise NotImplementedError()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM_MESSAGING, in_signature='o')
    def Delete(self, path):
        """
        Deletes an SMS message.

        @param path: The object path of the SMS to delete.
        Emits:
            Deleted

        """
        raise NotImplementedError()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM_MESSAGING,
                         in_signature='a{sv}', out_signature='o')
    def Create(self, properties):
        """
        Creates a new message object. The 'Number' and 'Text' properties are
        mandatory, others are optional. If the SMSC is not specified and one is
        required, the default SMSC is used.

        @param properties: Message properties from the SMS D-Bus interface.
        @returns: The object path of the new message object.

        """
        raise NotImplementedError()


    @dbus.service.signal(mm1_constants.I_MODEM_MESSAGING, signature='ob')
    def Added(self, path, received):
        """
        Emitted when any part of a new SMS has been received or added (but not
        for subsequent parts, if any). For messages received from the network,
        not all parts may have been received and the message may not be
        complete.

        Check the 'State' property to determine if the message is complete. The
        "Completed" signal will also be emitted when the message is complete.

        @param path: Object path of the new SMS.
        @param received: True if the message was received from the network, as
                         opposed to being added locally.

        """
        raise NotImplementedError()


    @dbus.service.signal(mm1_constants.I_MODEM_MESSAGING, signature='o')
    def Completed(self, path):
        """
        Emitted when the complete-ness status of an SMS message changes.

        An SMS may not necessarily be complete when the first part is received;
        this signal will be emitted when all parts have been received, even for
        single-part messages.

        @param path: Object path of the new SMS.

        """
        raise NotImplementedError()


    @dbus.service.signal(mm1_constants.I_MODEM_MESSAGING, signature='o')
    def Deleted(self, path):
        """
        Emitted when a message has been deleted.

        @param path: Object path of the now deleted SMS.

        """
        raise NotImplementedError()
