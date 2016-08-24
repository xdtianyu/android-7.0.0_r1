# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.cros.cellular import mm1_constants

class ModemSimple(dbus.service.Interface):
    """
    Python binding for the org.freedesktop.ModemManager1.Modem.Simple
    interface. All subclasses of Modem must implement this interface.
    The Simple interface allows controlling and querying the status of
    modems.

    """

    # Remember to decorate your concrete implementation with
    # @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM_SIMPLE,
                         in_signature='a{sv}', out_signature='o',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Connect(self, properties, return_cb, raise_cb):
        """
        Do everything needed to connect the modem using the given properties.

        This method will attempt to find a matching packet data bearer and
        activate it if necessary, returning the bearer's IP details. If no
        matching bearer is found, a new bearer will be created and activated,
        but this operation may fail if no resources are available to complete
        this connection attempt (i.e., if a conflicting bearer is already
        active).

        This call may make a large number of changes to modem configuration
        based on properties passed in. For example, given a PIN-locked,
        disabled GSM/UMTS modem, this call may unlock the SIM PIN, alter the
        access technology preference, wait for network registration (or force
        registration to a specific provider), create a new packet data bearer
        using the given "apn", and connect that bearer.

        @param properties: See the ModemManager Reference Manual for the allowed
                key/value pairs in properties.
        @param return_cb: The callback to execute to send an asynchronous
                response for the initial Connect request.
        @param raise_cb: The callback to execute to send an asynchronous error
                in response to the initial Connect request.
        @returns: On successfult connect, returns the object path of the connected
                packet data bearer used for the connection attempt. The value
                is returned asynchronously via return_cb.

        """
        raise NotImplementedError()


    # Remember to decorate your concrete implementation with
    # @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM_SIMPLE, in_signature='o',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Disconnect(self, bearer, return_cb, raise_cb, *return_cb_args):
        """
        Disconnect an active packet data connection.

        @param bearer: The object path of the data bearer to disconnect. If the
                path is "/" (i.e. no object given) this method will disconnect
                all active packet data bearers.
        @param return_cb: The callback to execute to send an asynchronous
                response for the initial Disconnect request.
        @param raise_cb: The callback to execute to send an asynchronous error
                in response to the initial Disconnect request.
        @param return_cb_args: Optional arguments which will be supplied to
                return_cb. This allows control flow to be set when this method
                is called from within the pseudo modem manager.

        """
        raise NotImplementedError()


    # Remember to decorate your concrete implementation with
    # @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM_SIMPLE, out_signature='a{sv}')
    def GetStatus(self):
        """
        Gets the general modem status.

        @returns: Dictionary of properties. See the ModemManager Reference Manual
                for the predefined common properties.

        """
        raise NotImplementedError()
