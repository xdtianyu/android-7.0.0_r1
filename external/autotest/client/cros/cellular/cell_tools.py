# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for cellular tests."""
import copy, dbus, os, tempfile

# TODO(thieule): Consider renaming mm.py, mm1.py, modem.py, etc to be more
# descriptive (crosbug.com/37060).
import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.cros.cellular import cellular_system_error
from autotest_lib.client.cros.cellular import mm
from autotest_lib.client.cros.cellular import modem

from autotest_lib.client.cros import flimflam_test_path
import flimflam


TIMEOUT = 30
SERVICE_TIMEOUT = 60

import cellular_logging

logger = cellular_logging.SetupCellularLogging('cell_tools')


def ConnectToCellular(flim, timeout=TIMEOUT):
    """Attempts to connect to a cell network using FlimFlam.

    Args:
        flim: A flimflam object
        timeout: Timeout (in seconds) before giving up on connect

    Returns:
        a tuple of the service and the service state

    Raises:
        Error if connection fails or times out
    """

    service = flim.FindCellularService(timeout=timeout)
    if not service:
        raise cellular_system_error.ConnectionFailure(
            'Could not find cell service')
    properties = service.GetProperties(utf8_strings=True)
    logger.error('Properties are: %s', properties)

    logger.info('Connecting to cell service: %s', service)

    states = ['portal', 'online', 'idle']
    state = flim.WaitForServiceState(service=service,
                                     expected_states=states,
                                     timeout=timeout,
                                     ignore_failure=True)[0]
    logger.debug('Cell connection state : %s ' % state)
    connected_states = ['portal', 'online']
    if state in connected_states:
        logger.debug('Looks good, skip ConnectService')
        return service, state
    else:
        logger.debug('Trying to ConnectService')

    success, status = flim.ConnectService(
        service=service,
        assoc_timeout=timeout,
        config_timeout=timeout)

    if not success:
        logger.error('Connect failed: %s' % status)
        # TODO(rochberg):  Turn off autoconnect
        if 'Error.AlreadyConnected' not in status['reason']:
            raise cellular_system_error.ConnectionFailure(
                'Could not connect: %s.' % status)

    state = flim.WaitForServiceState(service=service,
                                     expected_states=connected_states,
                                     timeout=timeout,
                                     ignore_failure=True)[0]
    if not state in connected_states:
        raise cellular_system_error.BadState(
            'Still in state %s, expecting one of: %s ' %
            (state, str(connected_states)))

    return service, state


def FindLastGoodAPN(service, default=None):
    if not service:
        return default
    props = service.GetProperties()
    if 'Cellular.LastGoodAPN' not in props:
        return default
    last_good_apn = props['Cellular.LastGoodAPN']
    return last_good_apn.get('apn', default)


def DisconnectFromCellularService(bs, flim, service):
    """Attempts to disconnect from the supplied cellular service.

    Args:
        bs:  A basestation object.  Pass None to skip basestation-side checks
        flim:  A flimflam object
        service:  A cellular service object
    """

    flim.DisconnectService(service)  # Waits for flimflam state to go to idle

    if bs:
        verifier = bs.GetAirStateVerifier()
        # This is racy: The modem is free to report itself as
        # disconnected before it actually finishes tearing down its RF
        # connection.
        verifier.AssertDataStatusIn([
            cellular.UeGenericDataStatus.DISCONNECTING,
            cellular.UeGenericDataStatus.REGISTERED,
            cellular.UeGenericDataStatus.NONE,])

        def _ModemIsFullyDisconnected():
            return verifier.IsDataStatusIn([
                cellular.UeGenericDataStatus.REGISTERED,
                cellular.UeGenericDataStatus.NONE,])

        utils.poll_for_condition(
            _ModemIsFullyDisconnected,
            timeout=20,
            exception=cellular_system_error.BadState(
                'modem not disconnected from base station'))


def _EnumerateModems(manager):
    """Get a set of modem paths."""
    return set([x[1] for x in mm.EnumerateDevices(manager)])


def _SawNewModem(manager, preexisting_modems, old_modem):
    current_modems = _EnumerateModems(manager)
    if old_modem in current_modems:
        return False
    # NB: This fails if an unrelated modem disappears.  Not fixing
    # until we support > 1 modem
    return preexisting_modems != current_modems


def _WaitForModemToReturn(manager, preexisting_modems_original, modem_path):
    preexisting_modems = copy.copy(preexisting_modems_original)
    preexisting_modems.remove(modem_path)

    utils.poll_for_condition(
        lambda: _SawNewModem(manager, preexisting_modems, modem_path),
        timeout=50,
        exception=cellular_system_error.BadState(
            'Modem did not come back after settings change'))

    current_modems = _EnumerateModems(manager)

    new_modems = [x for x in current_modems - preexisting_modems]
    if len(new_modems) != 1:
        raise cellular_system_error.BadState(
            'Unexpected modem list change: %s vs %s' %
            (current_modems, new_modems))

    logger.info('New modem: %s' % new_modems[0])
    return new_modems[0]


def SetFirmwareForTechnologyFamily(manager, modem_path, family):
    """Set the modem to firmware.  Return potentially-new modem path."""
    # todo(byronk): put this in a modem object?
    if family == cellular.TechnologyFamily.LTE:
        return  # nothing to set up on a Pixel. todo(byronk) how about others?
    logger.debug('SetFirmwareForTechnologyFamily : manager : %s ' % manager)
    logger.debug('SetFirmwareForTechnologyFamily : modem_path : %s ' %
                 modem_path)
    logger.debug('SetFirmwareForTechnologyFamily : family : %s ' % family)
    preexisting_modems = _EnumerateModems(manager)
    # We do not currently support any multi-family modems besides Gobi
    gobi = manager.GetModem(modem_path).GobiModem()
    if not gobi:
        raise cellular_system_error.BadScpiCommand(
            'Modem %s does not support %s, cannot change technologies' %
            modem_path, family)

    logger.info('Changing firmware to technology family %s' % family)

    FamilyToCarrierString = {
            cellular.TechnologyFamily.UMTS: 'Generic UMTS',
            cellular.TechnologyFamily.CDMA: 'Verizon Wireless',}

    gobi.SetCarrier(FamilyToCarrierString[family])
    return _WaitForModemToReturn(manager, preexisting_modems, modem_path)


# A test PRL that has an ID of 3333 and sets the device to aquire the
# default config of an 8960 with system_id 331.  Base64 encoding
# Generated with "base64 < prl"

TEST_PRL_3333 = (
    'ADENBQMAAMAAAYADAgmABgIKDQsEAYAKDUBAAQKWAAICQGAJApYAAgIw8BAAAQDhWA=='.
    decode('base64_codec'))


# A modem with this MDN will always report itself as activated
TESTING_MDN = dbus.String('1115551212', variant_level=1)


def _IsCdmaModemConfiguredCorrectly(manager, modem_path):
    """Returns true iff the CDMA modem at modem_path is configured correctly."""
    # We don't test for systemID because the PRL should take care of
    # that.

    status = manager.GetModem(modem_path).SimpleModem().GetStatus()

    required_settings = {'mdn': TESTING_MDN,
                         'min': TESTING_MDN,
                         'prl_version': 3333}
    configured_correctly = True

    for rk, rv in required_settings.iteritems():
        if rk not in status or rv != status[rk]:
            logger.error('_CheckCdmaModemStatus:  %s: expected %s, got %s' % (
                rk, rv, status.get(rk)))
            configured_correctly = False
    return configured_correctly


def PrepareCdmaModem(manager, modem_path):
    """Configure a CDMA device (including PRL, MIN, and MDN)."""

    if _IsCdmaModemConfiguredCorrectly(manager, modem_path):
        return modem_path

    logger.info('Updating modem settings')
    preexisting_modems = _EnumerateModems(manager)
    cdma = manager.GetModem(modem_path).CdmaModem()

    with tempfile.NamedTemporaryFile() as f:
        os.chmod(f.name, 0744)
        f.write(TEST_PRL_3333)
        f.flush()
        logger.info('Calling ActivateManual to change PRL')

        cdma.ActivateManual({
            'mdn': TESTING_MDN,
            'min': TESTING_MDN,
            'prlfile': dbus.String(f.name, variant_level=1),
            'system_id': dbus.UInt16(331, variant_level=1),  # Default 8960 SID
            'spc': dbus.String('000000'),})
        new_path = _WaitForModemToReturn(
            manager, preexisting_modems, modem_path)

    if not _IsCdmaModemConfiguredCorrectly(manager, new_path):
        raise cellular_system_error.BadState('Modem configuration failed')
    return new_path


def PrepareModemForTechnology(modem_path, target_technology):
    """Prepare modem for the technology: Sets things like firmware, PRL."""

    manager, modem_path = mm.PickOneModem(modem_path)

    logger.info('Found modem %s' % modem_path)


    # todo(byronk) : This returns TechnologyFamily:UMTS on a Pixel. ????
    current_family = manager.GetModem(modem_path).GetCurrentTechnologyFamily()
    target_family = cellular.TechnologyToFamily[target_technology]

    if current_family != target_family:
        logger.debug('Modem Current Family: %s ' % current_family)
        logger.debug('Modem Target Family : %s ' %target_family )
        modem_path = SetFirmwareForTechnologyFamily(
            manager, modem_path, target_family)

    if target_family == cellular.TechnologyFamily.CDMA:
        modem_path = PrepareCdmaModem(manager, modem_path)
        # Force the modem to report that is has been activated since we
        # use a custom PRL and have already manually activated it.
        manager.GetModem(modem_path).GobiModem().ForceModemActivatedStatus()

    # When testing EVDO, we need to force the modem to register with EVDO
    # directly (bypassing CDMA 1x RTT) else the modem will not register
    # properly because it looks for CDMA 1x RTT first but can't find it
    # because the call box can only emulate one technology at a time (EVDO).
    try:
        if target_technology == cellular.Technology.EVDO_1X:
            network_preference = modem.Modem.NETWORK_PREFERENCE_EVDO_1X
        else:
            network_preference = modem.Modem.NETWORK_PREFERENCE_AUTOMATIC
        gobi = manager.GetModem(modem_path).GobiModem()
        gobi.SetNetworkPreference(network_preference)
    except AttributeError:
        # Not a Gobi modem
        pass

    return modem_path


def FactoryResetModem(modem_pattern, spc='000000'):
    """Factory resets modem, returns DBus pathname of modem after reset."""
    manager, modem_path = mm.PickOneModem(modem_pattern)
    preexisting_modems = _EnumerateModems(manager)
    modem = manager.GetModem(modem_path).Modem()
    modem.FactoryReset(spc)
    return _WaitForModemToReturn(manager, preexisting_modems, modem_path)


class OtherDeviceShutdownContext(object):
    """Context manager that shuts down other devices.

    Usage:
        with cell_tools.OtherDeviceShutdownContext('cellular'):
            block

    TODO(rochberg):  Replace flimflam.DeviceManager with this
    """

    def __init__(self, device_type):
        self.device_type = device_type
        self.device_manager = None

    def __enter__(self):
        self.device_manager = flimflam.DeviceManager(flimflam.FlimFlam())
        self.device_manager.ShutdownAllExcept(self.device_type)
        return self

    def __exit__(self, exception, value, traceback):
        if self.device_manager:
            self.device_manager.RestoreDevices()
        return False


class AutoConnectContext(object):
    """Context manager which sets autoconnect to either true or false.

       Enable or Disable autoconnect for the cellular service.
       Restore it when done.

       Usage:
           with cell_tools.DisableAutoConnectContext(device, flim, autoconnect):
               block
    """

    def __init__(self, device, flim, autoconnect):
        self.device = device
        self.flim = flim
        self.autoconnect = autoconnect
        self.autoconnect_changed = False

    def PowerOnDevice(self, device):
        """Power on a flimflam device, ignoring in progress errors."""
        logger.info('powered = %s' % device.GetProperties()['Powered'])
        if device.GetProperties()['Powered']:
            return
        try:
            device.Enable()
        except dbus.exceptions.DBusException, e:
            if e._dbus_error_name != 'org.chromium.flimflam.Error.InProgress':
                raise e

    def __enter__(self):
        """Power up device, get the service and disable autoconnect."""
        changed = False
        self.PowerOnDevice(self.device)

        # Use SERVICE_TIMEOUT*2 here because it may take SERVICE_TIMEOUT
        # seconds for the modem to disconnect when the base emulator is taken
        # offline for reconfiguration and then another SERVICE_TIMEOUT
        # seconds for the modem to reconnect after the base emulator is
        # brought back online.
        #
        # TODO(jglasgow): generalize to use services associated with device
        service = self.flim.FindCellularService(timeout=SERVICE_TIMEOUT*2)
        if not service:
            raise error.TestFail('No cellular service available.')

        # Always set the AutoConnect property even if the requested value
        # is the same so that shill will retain the AutoConnect property, else
        # shill may override it.
        props = service.GetProperties()
        autoconnect = props['AutoConnect']
        logger.info('AutoConnect = %s' % autoconnect)
        logger.info('Setting AutoConnect = %s.', self.autoconnect)
        service.SetProperty('AutoConnect', dbus.Boolean(self.autoconnect))

        if autoconnect != self.autoconnect:
            props = service.GetProperties()
            autoconnect = props['AutoConnect']
            changed = True

        # Make sure the cellular service gets persisted by taking it out of
        # the ephemeral profile.
        if not props['Profile']:
            manager_props = self.flim.manager.GetProperties()
            active_profile = manager_props['ActiveProfile']
            logger.info("Setting cellular service profile to %s",
                        active_profile)
            service.SetProperty('Profile', active_profile)

        if autoconnect != self.autoconnect:
            raise error.TestFail('AutoConnect is %s, but we want it to be %s' %
                                 (autoconnect, self.autoconnect))

        self.autoconnect_changed = changed

        return self

    def __exit__(self, exception, value, traceback):
        """Restore autoconnect state if we changed it."""
        if not self.autoconnect_changed:
            return False

        try:
            self.PowerOnDevice(self.device)
        except Exception as e:
            if exception:
                logger.error(
                    'Exiting AutoConnectContext with one exception, but ' +
                    'PowerOnDevice raised another')
                logger.error(
                    'Swallowing PowerOnDevice exception %s' % e)
                return False
            else:
                raise e

        # TODO(jglasgow): generalize to use services associated with
        # device, and restore state only on changed services
        service = self.flim.FindCellularService()
        if not service:
            logger.error('Cannot find cellular service.  '
                          'Autoconnect state not restored.')
            return False
        service.SetProperty('AutoConnect', dbus.Boolean(not self.autoconnect))

        return False
