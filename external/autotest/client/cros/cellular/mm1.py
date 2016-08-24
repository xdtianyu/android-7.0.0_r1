#!/usr/bin/env python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Python bindings of ModemManager1 DBUS constants."""

from dbus.exceptions import DBusException

# The root object
OMM = '/org/freedesktop/ModemManager1'

# Interfaces
OFDOM = 'org.freedesktop.DBus.ObjectManager'
MODEM_MANAGER_INTERFACE = 'org.freedesktop.ModemManager1'
MODEM_INTERFACE = MODEM_MANAGER_INTERFACE + '.Modem'
MODEM_MODEM3GPP_INTERFACE = MODEM_INTERFACE + '.Modem3gpp'
MODEM_SIMPLE_INTERFACE = MODEM_INTERFACE + '.Simple'
MODEM_MODEMCDMA_INTERFACE = MODEM_INTERFACE + '.Cdma'
MODEM_MESSAGING_INTERFACE = MODEM_INTERFACE + '.Messaging'
SIM_INTERFACE = MODEM_MANAGER_INTERFACE + '.Sim'
SMS_INTERFACE = MODEM_MANAGER_INTERFACE + '.Sms'

# Modem States from Modemmanager-enums.h
MM_MODEM_STATE_FAILED = -1
MM_MODEM_STATE_UNKNOWN = 0
MM_MODEM_STATE_INITIALIZING = 1
MM_MODEM_STATE_LOCKED = 2
MM_MODEM_STATE_DISABLED = 3
MM_MODEM_STATE_DISABLING = 4
MM_MODEM_STATE_ENABLING = 5
MM_MODEM_STATE_ENABLED = 6
MM_MODEM_STATE_SEARCHING = 7
MM_MODEM_STATE_REGISTERED = 8
MM_MODEM_STATE_DISCONNECTING = 9
MM_MODEM_STATE_CONNECTING = 10
MM_MODEM_STATE_CONNECTED = 11

# State Change Reasons
MM_MODEM_STATE_CHANGE_REASON_UNKNOWN = 0
MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED = 1
MM_MODEM_STATE_CHANGE_REASON_SUSPEND = 2

# List of GSM Registration Status
MM_MODEM_3GPP_REGISTRATION_STATE_IDLE = 0
MM_MODEM_3GPP_REGISTRATION_STATE_HOME = 1
MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING = 2
MM_MODEM_3GPP_REGISTRATION_STATE_DENIED = 3
MM_MODEM_3GPP_REGISTRATION_STATE_UNKNOWN = 4
MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING = 5

# Property Names
MM_MODEM_PROPERTY_STATE = 'State'
MM_MODEM3GPP_PROPERTY_REGISTRATION_STATE = 'RegistrationState'


class ConnectionUnknownError(DBusException):
    _dbus_error_name = MODEM_MANAGER_INTERFACE + '.Connection.Unknown'
    include_traceback = False


class ServiceOptionNotSubscribedError(DBusException):
    _dbus_error_name = (
        MODEM_MANAGER_INTERFACE +
        '.MobileEquipment.Connect.Gprs.ServiceOptionNotSubscribed')
    include_traceback = False


class NoNetworkError(DBusException):
    _dbus_error_name = (
        MODEM_MANAGER_INTERFACE + '.MobileEquipment.Connect.NoNetwork')
    include_traceback = False


class CoreUnsupportedError(DBusException):
    _dbus_error_name = MODEM_MANAGER_INTERFACE + '.Core.Unsupported'
    include_traceback = False
