# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service

import dbus_std_ifaces
import pm_constants
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class Testing(dbus_std_ifaces.DBusProperties):
    """
    The testing object allows the pseudomodem to be configured on the fly
    over D-Bus. It exposes a basic set of commands that can be used to
    simulate network events (such as SMS) or various other modem configurations
    that are needed for testing/debugging.

    """

    def __init__(self, modem, bus):
        self._modem = modem
        dbus_std_ifaces.DBusProperties.__init__(self,
                                                pm_constants.TESTING_PATH,
                                                bus)


    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING, out_signature='b')
    def IsAlive(self):
        """
        A heartbeat method.

        This method can be called by clients to check that pseudomodem is alive.

        @returns: True, always.

        """
        return True


    def _InitializeProperties(self):
        return { pm_constants.I_TESTING: { 'Modem': self._modem.path } }


    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING, in_signature='ss')
    def ReceiveSms(self, sender, text):
        """
        Simulates a fake SMS.

        @param sender: String containing the phone number of the sender.
        @param text: String containing the SMS message contents.

        """
        self._modem.sms_handler.receive_sms(text, sender)


    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING, in_signature='s')
    def UpdatePcoInfo(self, pco_value):
        """
        Sets the VendorPcoInfo to the specified value. If the Modem.Modem3gpp
        properties are currently not exposed (e.g. due to a locked or absent
        SIM), this method will do nothing.

        @param pco_value: The PCO string.

        """
        if mm1_constants.I_MODEM_3GPP in self._modem.properties:
            self._modem.AssignPcoValue(pco_value)

    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING, in_signature='uu')
    def SetSubscriptionState(self,
                             unregistered_subscription_state,
                             registered_subscription_state):
        """
        Sets the SubscriptionState to the specified value. If the
        Modem.Modem3gpp properties are currently not exposed (e.g. due to a
        locked or absent SIM), this method will do nothing.

        @param unregistered_subscription_state: This value is returned as the
                subscription state when the modem is not registered on the
                network. See mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_*.
        @param registered_subscription_state: This value is returned as the
                subscription state when the modem is registered on the network.
                See mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_*.

        """
        if mm1_constants.I_MODEM_3GPP in self._modem.properties:
            self._modem.AssignSubscriptionState(unregistered_subscription_state,
                                                registered_subscription_state)
