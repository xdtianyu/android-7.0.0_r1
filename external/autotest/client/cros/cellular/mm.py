# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import modem
from autotest_lib.client.cros.cellular import modem1


MMPROVIDERS = ['org.chromium', 'org.freedesktop']
SERVICE_UNKNOWN = 'org.freedesktop.DBus.Error.ServiceUnknown'


def GetManager():
    """Returns a ModemManager object.

    Attempts to connect to various modemmanagers, including
    ModemManager classic interfaces, ModemManager using the
    ModemManager1 interfaces and cromo and return the first
    ModemManager that is found.

    Returns:
        a ModemManager object.
    """
    for provider in MMPROVIDERS:
        try:
            return modem.ModemManager(provider)
        except dbus.exceptions.DBusException, e:
            if e._dbus_error_name != SERVICE_UNKNOWN:
                raise

    try:
        return modem1.ModemManager()
    except dbus.exceptions.DBusException, e:
        if e._dbus_error_name != SERVICE_UNKNOWN:
            raise

    return None


def EnumerateDevices(manager=None):
    """Enumerates all modems in the system.

    Args:
        manager: the specific manager to use, if None use the first valid
                 manager

    Returns:
        a list of (ModemManager object, modem dbus path)
    """
    if not manager:
        manager = GetManager()
    if not manager:
        raise error.TestError('Cannot connect to the modem manager, is '
                              'ModemManager/cromo/PseudoModemManager running?')

    result = []
    for path in manager.EnumerateDevices():
        result.append((manager, path))

    return result


def PickOneModem(modem_pattern, manager=None):
    """Pick a modem.

    If a machine has a single modem, managed by one of the MMPROVIDERS,
    return the dbus path and a ModemManager object for that modem.

    Args:
        modem_pattern: pattern that should match the modem path
        manager: the specific manager to use, if None check all known managers

    Returns:
        (ModemManager, Modem DBUS Path) tuple

    Raises:
        TestError: if there are no matching modems, or there are more
                   than one
    """
    devices = EnumerateDevices(manager)

    matches = [(m, path) for m, path in devices if modem_pattern in path]
    if not matches:
        raise error.TestError('No modems had substring: ' + modem_pattern)
    if len(matches) > 1:
        raise error.TestError('Expected only one modem, got: ' +
                              ', '.join([modem.path for modem in matches]))
    return matches[0]
