# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service
import dbus.types
import gobject
import logging
import random

import bearer
import dbus_std_ifaces
import messaging
import modem_simple
import pm_constants
import pm_errors
import sms_handler
import state_machine_factory as smf
import utils

import common
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import net_interface

ALLOWED_BEARER_PROPERTIES = [
    'apn',
    'operator-id',
    'allowed-modes',
    'preferred-mode',
    'bands',
    'ip-type',
    'user',
    'password',
    'allow-roaming',
    'rm-protocol',
    'number'
]

class Modem(dbus_std_ifaces.DBusProperties,
            modem_simple.ModemSimple,
            messaging.Messaging):
    """
    Pseudomodem implementation of the org.freedesktop.ModemManager1.Modem
    interface. This class serves as the abstract base class of all fake modem
    implementations.

    """

    SUPPORTS_MULTIPLE_OBJECT_PATHS = True

    def __init__(self,
                 state_machine_factory=None,
                 bus=None,
                 device='pseudomodem0',
                 device_port_type=mm1_constants.MM_MODEM_PORT_TYPE_AT,
                 index=0,
                 roaming_networks=None,
                 config=None):
        """
        Initializes the fake modem object. kwargs can contain the optional
        argument |config|, which is a dictionary of property-value mappings.
        These properties will be added to the underlying property dictionary,
        and must be one of the properties listed in the ModemManager Reference
        Manual. See _InitializeProperties for all of the properties that belong
        to this interface. Possible values for each are enumerated in
        mm1_constants.py.

        """
        if state_machine_factory:
            self._state_machine_factory = state_machine_factory
        else:
            self._state_machine_factory = smf.StateMachineFactory()
        self.device = device
        self.device_port_type = device_port_type
        self.index = index
        self.sim = None

        # The superclass construct will call _InitializeProperties
        dbus_std_ifaces.DBusProperties.__init__(self,
            mm1_constants.MM1 + '/Modem/' + str(index), bus, config)

        if roaming_networks is None:
            roaming_networks = []
        self.roaming_networks = roaming_networks

        self.bearers = {}
        self.active_bearers = {}
        self.enable_step = None
        self.disable_step = None
        self.connect_step = None
        self.disconnect_step = None
        self.register_step = None

        self._modemmanager = None
        self.resetting = False

        self._sms_handler = sms_handler.SmsHandler(self, bus)


    def _InitializeProperties(self):
        """ Sets up the default values for the properties. """
        props = {
            'Manufacturer' : 'Banana Technologies', # be creative here
            'Model' : 'Banana Peel 3000', # yep
            'Revision' : '1.0',
            'DeviceIdentifier' : 'Banana1234567890',
            'Device' : self.device,
            'Ports': [dbus.types.Struct(
                              [self.device,
                               dbus.types.UInt32(self.device_port_type)],
                              signature='su'),
                      dbus.types.Struct(
                              [net_interface.PseudoNetInterface.IFACE_NAME,
                               dbus.types.UInt32(
                                       mm1_constants.MM_MODEM_PORT_TYPE_NET)],
                              signature='su')],
            'Drivers' : ['FakeDriver'],
            'Plugin' : 'Banana Plugin',
            'UnlockRequired' :
                    dbus.types.UInt32(mm1_constants.MM_MODEM_LOCK_NONE),
            'UnlockRetries' : dbus.Dictionary(signature='uu'),
            'State' : dbus.types.Int32(mm1_constants.MM_MODEM_STATE_DISABLED),
            'SignalQuality' : dbus.types.Struct(
                                      [dbus.types.UInt32(100), True],
                                      signature='ub'),
            'OwnNumbers' : ['5555555555'],
            'PowerState' :
                    dbus.types.UInt32(mm1_constants.MM_MODEM_POWER_STATE_ON),
            'SupportedIpFamilies' :
                dbus.types.UInt32(mm1_constants.MM_BEARER_IP_FAMILY_ANY),
            'Bearers' : dbus.Array([], signature='o'),

            # specified by subclass:
            'SupportedCapabilities' :
                    [dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_NONE)],
            'CurrentCapabilities' :
                    dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_NONE),
            'MaxBearers' : dbus.types.UInt32(0),
            'MaxActiveBearers' : dbus.types.UInt32(0),
            'EquipmentIdentifier' : '',
            'AccessTechnologies' :
                    dbus.types.UInt32(
                            mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN),
            'SupportedModes' : [
                    dbus.types.Struct(
                            [dbus.types.UInt32(
                                    mm1_constants.MM_MODEM_MODE_NONE),
                             dbus.types.UInt32(
                                    mm1_constants.MM_MODEM_MODE_NONE)],
                            signature='uu')
            ],
            'CurrentModes' :
                    dbus.types.Struct(
                            [dbus.types.UInt32(
                                    mm1_constants.MM_MODEM_MODE_NONE),
                             dbus.types.UInt32(
                                    mm1_constants.MM_MODEM_MODE_NONE)],
                            signature='uu'),
            'SupportedBands' :
                    [dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_UNKNOWN)],
            'CurrentBands' :
                    [dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_UNKNOWN)],
            'Sim' : dbus.types.ObjectPath(mm1_constants.ROOT_PATH)
        }
        return {
            mm1_constants.I_MODEM : props,
            mm1_constants.I_MODEM_SIMPLE : {}
        }


    def IncrementPath(self):
        """
        Increments the current index at which this modem is exposed on DBus.
        E.g. if the current path is org/freedesktop/ModemManager/Modem/0, the
        path will change to org/freedesktop/ModemManager/Modem/1.

        Calling this method does not remove the object from its current path,
        which means that it will be available via both the old and the new
        paths. This is currently only used by Reset, in conjunction with
        dbus_std_ifaces.DBusObjectManager.[Add|Remove].

        """
        self.index += 1
        path = mm1_constants.MM1 + '/Modem/' + str(self.index)
        logging.info('Modem coming back as: ' + path)
        self.SetPath(path)


    @property
    def manager(self):
        """
        The current modemmanager.ModemManager instance that is managing this
        modem.

        @returns: A modemmanager.ModemManager object.

        """
        return self._modemmanager


    @manager.setter
    def manager(self, manager):
        """
        Sets the current modemmanager.ModemManager instance that is managing
        this modem.

        @param manager: A modemmanager.ModemManager object.

        """
        self._modemmanager = manager


    @property
    def sms_handler(self):
        """
        @returns: sms_handler.SmsHandler responsible for handling SMS.

        """
        return self._sms_handler


    def IsPendingEnable(self):
        """
        @returns: True, if a current enable state machine is active and hasn't
                been cancelled.

        """
        return self.enable_step and not self.enable_step.cancelled


    def IsPendingDisable(self):
        """
        @returns: True, if a current disable state machine is active and hasn't
                been cancelled.

        """
        return self.disable_step and not self.disable_step.cancelled


    def IsPendingConnect(self):
        """
        @returns: True, if a current connect state machine is active and hasn't
                been cancelled.

        """
        return self.connect_step and not self.connect_step.cancelled


    def IsPendingDisconnect(self):
        """
        @returns: True, if a current disconnect state machine is active and
                hasn't been cancelled.

        """
        return self.disconnect_step and not self.disconnect_step.cancelled


    def IsPendingRegister(self):
        """
        @returns: True, if a current register state machine is active and hasn't
                been cancelled.

        """
        return self.register_step and not self.register_step.cancelled


    def CancelAllStateMachines(self):
        """ Cancels all state machines that are active. """
        if self.IsPendingEnable():
            self.enable_step.Cancel()
        if self.IsPendingDisable():
            self.disable_step.Cancel()
        if self.IsPendingConnect():
            self.connect_step.Cancel()
        if self.IsPendingDisconnect():
            self.disconnect_step.Cancel()
        if self.IsPendingRegister():
            self.register_step.Cancel()


    def SetSignalQuality(self, quality):
        """
        Sets the 'SignalQuality' property to the given value.

        @param quality: An integer value in the range 0-100.
        Emits:
            PropertiesChanged

        """
        self.Set(mm1_constants.I_MODEM, 'SignalQuality', (dbus.types.Struct(
            [dbus.types.UInt32(quality), True], signature='ub')))


    def ChangeState(self, state, reason):
        """
        Changes the modem state and emits the StateChanged signal.

        @param state: A MMModemState value.
        @param reason: A MMModemStateChangeReason value.
        Emits:
            PropertiesChanged
            StateChanged

        """
        old_state = self.Get(mm1_constants.I_MODEM, 'State')
        self.SetInt32(mm1_constants.I_MODEM, 'State', state)
        self.StateChanged(old_state, state, dbus.types.UInt32(reason))


    def SetSIM(self, sim):
        """
        Assigns a SIM object to this Modem. It exposes the SIM object via DBus
        and sets 'Sim' property of this Modem to the path of the SIM.

        @param sim: An instance of sim.SIM.
        Emits:
            PropertiesChanged

        """
        self.sim = sim
        if not sim:
            val = mm1_constants.ROOT_PATH
        else:
            val = sim.path
            self.sim.SetBus(self.bus)
            self.sim.modem = self
            self.UpdateLockStatus()
        self.Set(mm1_constants.I_MODEM, 'Sim', dbus.types.ObjectPath(val))


    def SetBus(self, bus):
        """
        Overridden from dbus_std_ifaces.DBusProperties.

        @param bus

        """
        dbus_std_ifaces.DBusProperties.SetBus(self, bus)
        self._state_machine_factory.SetBus(bus)
        self._sms_handler.bus = bus


    def UpdateLockStatus(self):
        """
        Tells the modem to update the current lock status. This method will
        update the modem state and the relevant modem properties.

        """
        if not self.sim:
            logging.info('SIM lock is the only kind of lock that is currently '
                         'supported. No SIM present, nothing to do.')
            return
        self.SetUInt32(mm1_constants.I_MODEM, 'UnlockRequired',
                       self.sim.lock_type)
        self.Set(mm1_constants.I_MODEM, 'UnlockRetries',
                 self.sim.unlock_retries)
        if self.sim.locked:
            def _SetLocked():
                logging.info('There is a SIM lock in place. Setting state to '
                             'LOCKED')
                self.ChangeState(
                        mm1_constants.MM_MODEM_STATE_LOCKED,
                        mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)

            # If the modem is currently in an enabled state, disable it before
            # setting the modem state to LOCKED.
            if (self.Get(mm1_constants.I_MODEM, 'State') >=
                    mm1_constants.MM_MODEM_STATE_ENABLED):
                logging.info('SIM got locked. Disabling modem.')
                self.Enable(False, return_cb=_SetLocked)
            else:
                _SetLocked()
        elif (self.Get(mm1_constants.I_MODEM, 'State') ==
                mm1_constants.MM_MODEM_STATE_LOCKED):
            # Change the state to DISABLED. Shill will see the property change
            # and automatically attempt to enable the modem.
            logging.info('SIM became unlocked! Setting state to INITIALIZING.')
            self.ChangeState(mm1_constants.MM_MODEM_STATE_INITIALIZING,
                             mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
            logging.info('SIM became unlocked! Setting state to DISABLED.')
            self.ChangeState(mm1_constants.MM_MODEM_STATE_DISABLED,
                             mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)


    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM,
                         in_signature='b', async_callbacks=('return_cb',
                                                            'raise_cb'))
    def Enable(self, enable, return_cb=None, raise_cb=None):
        """
        Enables or disables the modem.

        When enabled, the modem's radio is powered on and data sessions, voice
        calls, location services, and Short Message Service may be available.

        When disabled, the modem enters low-power state and no network-related
        operations are available.

        @param enable: True to enable the modem and False to disable it.
        @param return_cb: The asynchronous callback to invoke on success.
        @param raise_cb: The asynchronous callback to invoke on failure. Has to
                take a python Exception or Error as its single argument.

        """
        if enable:
            logging.info('Modem enable')
            machine = self._state_machine_factory.CreateMachine(
                    pm_constants.STATE_MACHINE_ENABLE,
                    self,
                    return_cb,
                    raise_cb)
        else:
            logging.info('Modem disable')
            machine = self._state_machine_factory.CreateMachine(
                    pm_constants.STATE_MACHINE_DISABLE,
                    self,
                    return_cb,
                    raise_cb)
        machine.Start()


    def RegisterWithNetwork(
            self, operator_id="", return_cb=None, raise_cb=None):
        """
        Register with the network specified by the given |operator_id|.
        |operator_id| should be an MCCMNC value (for 3GPP) or an empty string.
        An implementation of this method must set the state to SEARCHING first,
        and eventually to REGISTERED, also setting technology specific
        registration state properties. Technology specific error cases need to
        be handled here (such as activation, the presence of a valid SIM card,
        etc).

        Must be implemented by a subclass.

        @param operator_id: String containing the operator code. This method
                will typically initiate a network scan, yielding a list of
                networks. If |operator_id| is non-empty, the modem will register
                with the network in the scanned list that matches |operator_id|.
                An empty |operator_id| means that registration should be
                "automatic". In this case the implementation would typically
                register with the home network. If a home network is not
                available than any network that is returned by a network scan
                can be registered with.

                Note: CDMA doesn't support a network scan. In this case, the
                only possible option is to register with the home network and
                ignore the value of |operator_id|.
        @param return_cb: Async success callback.
        @param raise_cb: Async failure callback.

        """
        raise NotImplementedError()


    def UnregisterWithNetwork(self):
        """
        Unregisters with the currently registered network. This should
        transition the modem to the ENABLED state.

        Must be implemented by a subclass.

        """
        raise NotImplementedError()


    def ValidateBearerProperties(self, properties):
        """
        The default implementation makes sure that all keys in properties are
        one of the allowed bearer properties. Subclasses can override this
        method to provide CDMA/3GPP specific checks.

        @param properties: The dictionary of properties and values to validate.
        @raises: MMCoreError, if one or more properties are invalid.

        """
        for key in properties.iterkeys():
            if key not in ALLOWED_BEARER_PROPERTIES:
                raise pm_errors.MMCoreError(
                        pm_errors.MMCoreError.INVALID_ARGS,
                        'Invalid property "%s", not creating bearer.' % key)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, out_signature='ao')
    def ListBearers(self):
        """
        Lists configured packet data bearers (EPS Bearers, PDP Contexts, or
        CDMA2000 Packet Data Sessions).

        @returns: A list of bearer object paths.

        """
        return self.Get(mm1_constants.I_MODEM, 'Bearers')


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='a{sv}',
                         out_signature='o')
    def CreateBearer(self, properties):
        """
        Creates a new packet data bearer using the given characteristics.

        This request may fail if the modem does not support additional bearers,
        if too many bearers are already defined, or if properties are invalid.

        @param properties: A dictionary containing the properties to assign to
                the bearer after creating it. The allowed property values are
                contained in modem.ALLOWED_PROPERTIES.
        @returns: On success, the object path of the newly created bearer.

        """
        logging.info('CreateBearer')
        maxbearers = self.Get(mm1_constants.I_MODEM, 'MaxBearers')
        if len(self.bearers) == maxbearers:
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.TOO_MANY,
                    ('Maximum number of bearers reached. Cannot create new '
                     'bearer.'))
        else:
            self.ValidateBearerProperties(properties)
            bearer_obj = bearer.Bearer(self.bus, properties)
            logging.info('Created bearer with path "%s".', bearer_obj.path)
            self.bearers[bearer_obj.path] = bearer_obj
            self._UpdateBearersProperty()
            return bearer_obj.path


    def ActivateBearer(self, bearer_path):
        """
        Activates a data bearer by setting its 'Connected' property to True.

        This request may fail if the modem does not support additional active
        bearers, if too many bearers are already active, if the requested
        bearer doesn't exist, or if the requested bearer is already active.

        @param bearer_path: DBus path of the bearer to activate.

        """
        logging.info('ActivateBearer: %s', bearer_path)
        bearer = self.bearers.get(bearer_path, None)
        if bearer is None:
            message = 'Could not find bearer with path "%s"' % bearer_path
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.NOT_FOUND,
                                        message)

        max_active_bearers = self.Get(mm1_constants.I_MODEM, 'MaxActiveBearers')
        if len(self.active_bearers) >= max_active_bearers:
            message = ('Cannot activate bearer: maximum active bearer count '
                       'reached.')
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.TOO_MANY, message)
        if bearer.IsActive():
            message = 'Bearer with path "%s" already active.', bearer_path
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.CONNECTED,
                                        message)

        self.active_bearers[bearer_path] = bearer
        bearer.Connect()


    def DeactivateBearer(self, bearer_path):
        """
        Deactivates data bearer by setting its 'Connected' property to False.

        This request may fail if the modem with the requested path doesn't
        exist, or if the bearer is not active.

        @param bearer_path: DBus path of the bearer to activate.

        """
        logging.info('DeactivateBearer: %s', bearer_path)
        bearer = self.bearers.get(bearer_path, None)
        if bearer is None:
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.NOT_FOUND,
                    'Could not find bearer with path "%s".' % bearer_path)
        if not bearer.IsActive():
            assert bearer_path not in self.active_bearers
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.WRONG_STATE,
                    'Bearer with path "%s" is not active.' % bearer_path)
        assert bearer_path in self.active_bearers
        bearer.Disconnect()
        self.active_bearers.pop(bearer_path)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='o')
    def DeleteBearer(self, bearer):
        """
        Deletes an existing packet data bearer.

        If the bearer is currently active, it will be deactivated.

        @param bearer: Object path of the bearer to delete.

        """
        logging.info('Modem.DeleteBearer: ' + str(bearer))
        if not bearer in self.bearers:
            logging.info('Unknown bearer. Nothing to do.')
            return
        bearer_object = self.bearers[bearer]
        bearer_object.remove_from_connection()
        self.bearers.pop(bearer)
        self._UpdateBearersProperty()
        if bearer in self.active_bearers:
            self.active_bearers.pop(bearer)


    def ClearBearers(self):
        """ Deletes all bearers that are managed by this modem. """
        for b in self.bearers.keys():
            self.DeleteBearer(b)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM)
    def Reset(self):
        """
        Clears non-persistent configuration and state, and returns the device to
        a newly-powered-on state.

        As a result of this operation, the modem will be removed from its
        current path and will be exposed on an incremented path. It will be
        enabled afterwards.

        """
        logging.info('Resetting modem.')

        if self.resetting:
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.IN_PROGRESS,
                                        'Reset already in progress.')

        self.resetting = True

        self.CancelAllStateMachines()

        def _ResetFunc():
            # Disappear.
            manager = self.manager
            if manager:
                manager.Remove(self)
                if self.sim:
                    manager.Remove(self.sim)

            self.ClearBearers()

            # Reappear.
            def _DelayedReappear():
                self.IncrementPath()

                # Reset to defaults.
                if self.sim:
                    self.sim.Reset()
                self._properties = self._InitializeProperties()
                if self.sim:
                    self.Set(mm1_constants.I_MODEM,
                             'Sim',
                             dbus.types.ObjectPath(self.sim.path))
                    self.UpdateLockStatus()

                if manager:
                    manager.Add(self)

                self.resetting = False

                def _DelayedEnable():
                    state = self.Get(mm1_constants.I_MODEM, 'State')
                    if not self.IsPendingEnable() and \
                            state == mm1_constants.MM_MODEM_STATE_DISABLED:
                        self.Enable(True)
                    return False

                gobject.timeout_add(1000, _DelayedEnable)
                return False

            gobject.timeout_add(2000, _DelayedReappear)

        def _ErrorCallback(error):
            raise error

        if (self.Get(mm1_constants.I_MODEM, 'State') ==
                mm1_constants.MM_MODEM_STATE_CONNECTED):
            self.Disconnect('/', _ResetFunc, _ErrorCallback)
        else:
            gobject.idle_add(_ResetFunc)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='s')
    def FactoryReset(self, code):
        """
        Clears the modem's configuration (including persistent configuration and
        state), and returns the device to a factory-default state.

        If not required by the modem, code may be ignored.

        This command may or may not power-cycle the device.

        @param code: Carrier specific activation code.

        """
        raise NotImplementedError()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='(uu)')
    def SetCurrentModes(self, modes):
        """
        Sets the access technologies (eg 2G/3G/4G preference) the device is
        currently allowed to use when connecting to a network.

        @param modes: Specifies all the modes allowed in the modem as a bitmask
                of MMModemModem values.
        @param preferred: Specific MMModemMode preferred among the ones allowed,
                if any.

        """
        allowed = self.Get(mm1_constants.I_MODEM, 'SupportedModes')
        if not modes in allowed:
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED,
                                        'Mode not supported: ' + repr(modes))
        self.Set(mm1_constants.I_MODEM, 'CurrentModes', modes)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='au')
    def SetCurrentBands(self, bands):
        """
        Sets the radio frequency and technology bands the device is currently
        allowed to use when connecting to a network.

        @param bands: Specifies the bands to be used as a list of MMModemBand
                values.

        """
        band_list = [dbus.types.UInt32(band) for band in bands]
        self.Set(mm1_constants.I_MODEM, 'CurrentBands', band_list)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='su',
                         out_signature='s')
    def Command(self, cmd, timeout):
        """
        Allows clients to send commands to the modem. By default, this method
        does nothing, but responds by telling the client's fortune to brighten
        the client's day.

        @param cmd: Command to send to the modem.
        @param timeout: The timeout interval for the command.
        @returns: A string containing the response from the modem.

        """
        messages = ['Bananas are tasty and fresh. Have one!',
                    'A soft voice may be awfully persuasive.',
                    'Be careful or you could fall for some tricks today.',
                    'Believe in yourself and others will too.',
                    'Carve your name on your heart and not on marble.']
        return random.choice(messages)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='u')
    def SetPowerState(self, power_state):
        """
        Sets the power state of the modem. This action can only be run when the
        modem is in the MM_MODEM_STATE_DISABLED state.

        @param power_state: Specifies the desired power state as a
                MMModemPowerState value.
        @raises: MMCoreError if state is not DISABLED.

        """
        if (self.Get(mm1_constants.I_MODEM, 'State') !=
                mm1_constants.MM_MODEM_STATE_DISABLED):
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.WRONG_STATE,
                    'Cannot set the power state if modem is not DISABLED.')
        self.SetUInt32(mm1_constants.I_MODEM, 'PowerState', power_state);


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM, in_signature='u')
    def SetCurrentCapabilities(self, capabilities):
        """
        Set the capabilities of the device. A restart of the modem may be
        required.

        @param capabilities: Bitmask of MMModemCapability values, to specify the
                capabilities to use.

        """
        supported = self.Get(mm1_constants.I_MODEM, 'SupportedCapabilities')
        if not capabilities in supported:
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.FAILED,
                    'Given capabilities not supported: ' + capabilities)
        self.SetUInt32(mm1_constants.I_MODEM, 'CurrentCapabilities',
                       capabilities)


    @dbus.service.signal(mm1_constants.I_MODEM, signature='iiu')
    def StateChanged(self, old, new, reason):
        """
        Signals that the modem's 'State' property has changed.

        @param old: Specifies the old state, as a MMModemState value.
        @param new: Specifies the new state, as a MMModemState value.
        @param reason: Specifies the reason for this state change as a
                MMModemStateChangeReason value.

        """
        logging.info('Modem state changed from %u to %u for reason %u',
                old, new, reason)


    # org.freedesktop.ModemManager1.Messaging

    def List(self):
        """
        Overriden from messaging.Messaging.

        """
        return self._sms_handler.list_messages()


    def Delete(self, path):
        """
        Overriden from messaging.Messaging.

        @param path

        """
        self._sms_handler.delete_message(path)


    @dbus.service.signal(mm1_constants.I_MODEM_MESSAGING, signature='ob')
    def Added(self, path, received):
        """
        Overriden from messaging.Messaging.

        @param path
        @param received

        """
        logging.info('New SMS added: path: ' + path + ' received: ' +
                     str(received))


    def _UpdateBearersProperty(self):
        """
        Update the 'Bearers' property on |I_MODEM| interface to match the
        internal list.

        """
        bearers = dbus.Array(
                [dbus.types.ObjectPath(key) for key in self.bearers.iterkeys()],
                signature='o')
        self.Set(mm1_constants.I_MODEM, 'Bearers', bearers)
