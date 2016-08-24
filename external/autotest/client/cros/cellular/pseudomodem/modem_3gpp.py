# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.types
import logging

import modem
import pm_constants
import pm_errors
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class Modem3gpp(modem.Modem):
    """
    Pseudomodem implementation of the
    org.freedesktop.ModemManager1.Modem.Modem3gpp and
    org.freedesktop.ModemManager1.Modem.Simple interfaces. This class provides
    access to specific actions that may be performed in modems with 3GPP
    capabilities.

    """

    IMEI = '00112342342123'

    class GsmNetwork(object):
        """
        GsmNetwork stores the properties of a 3GPP network that can be
        discovered during a network scan.

        """
        def __init__(self,
                     operator_long,
                     operator_short,
                     operator_code,
                     status,
                     access_technology):
            self.status = status
            self.operator_long = operator_long
            self.operator_short = operator_short
            self.operator_code = operator_code
            self.access_technology = access_technology


        def ToScanDictionary(self):
            """
            @returns: Dictionary containing operator data as defined by
                    org.freedesktop.ModemManager1.Modem.Modem3gpp.Scan.

            """
            return {
              'status': dbus.types.UInt32(self.status),
              'operator-long': self.operator_long,
              'operator-short': self.operator_short,
              'operator-code': self.operator_code,
              'access-technology': dbus.types.UInt32(self.access_technology),
            }


    def __init__(self,
                 state_machine_factory=None,
                 bus=None,
                 device='pseudomodem0',
                 index=0,
                 roaming_networks=None,
                 config=None):
        modem.Modem.__init__(self,
                             state_machine_factory,
                             bus=bus,
                             device=device,
                             roaming_networks=roaming_networks,
                             config=config)

        self._scanned_networks = {}
        self._cached_pco_value = ''
        self._cached_unregistered_subscription_state = (
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN)
        self._cached_registered_subscription_state = (
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_PROVISIONED)


    def _InitializeProperties(self):
        ip = modem.Modem._InitializeProperties(self)
        props = ip[mm1_constants.I_MODEM]
        props3gpp = self._GetDefault3GPPProperties()
        if props3gpp:
            ip[mm1_constants.I_MODEM_3GPP] = props3gpp
        props['SupportedCapabilities'] = [
                dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_GSM_UMTS),
                dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_LTE),
                dbus.types.UInt32(
                        mm1_constants.MM_MODEM_CAPABILITY_GSM_UMTS |
                        mm1_constants.MM_MODEM_CAPABILITY_LTE)
        ]
        props['CurrentCapabilities'] = dbus.types.UInt32(
                mm1_constants.MM_MODEM_CAPABILITY_GSM_UMTS |
                mm1_constants.MM_MODEM_CAPABILITY_LTE)
        props['MaxBearers'] = dbus.types.UInt32(3)
        props['MaxActiveBearers'] = dbus.types.UInt32(2)
        props['EquipmentIdentifier'] = self.IMEI
        props['AccessTechnologies'] = dbus.types.UInt32((
                mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_GSM |
                mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_UMTS))
        props['SupportedModes'] = [
                dbus.types.Struct(
                        [dbus.types.UInt32(mm1_constants.MM_MODEM_MODE_3G |
                                           mm1_constants.MM_MODEM_MODE_4G),
                         dbus.types.UInt32(mm1_constants.MM_MODEM_MODE_4G)],
                        signature='uu')
        ]
        props['CurrentModes'] = props['SupportedModes'][0]
        props['SupportedBands'] = [
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_EGSM),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_DCS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_PCS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_G850),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U2100),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U1800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U17IV),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U850)
        ]
        props['CurrentBands'] = [
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_EGSM),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_DCS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_PCS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_G850),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U2100),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_U850)
        ]
        return ip


    def _GetDefault3GPPProperties(self):
        if not self.sim or self.sim.locked:
            return None
        return {
            'Imei' : self.IMEI,
            'RegistrationState' : (
                    dbus.types.UInt32(
                        mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)),
            'OperatorCode' : '',
            'OperatorName' : '',
            'EnabledFacilityLocks' : (
                    dbus.types.UInt32(self.sim.enabled_locks)),
            'SubscriptionState' : dbus.types.UInt32(
                    mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN),
            'VendorPcoInfo': ''
        }


    def SyncScan(self):
        """ The synchronous implementation of |Scan| for this class. """
        state = self.Get(mm1_constants.I_MODEM, 'State')
        if state < mm1_constants.MM_MODEM_STATE_ENABLED:
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.WRONG_STATE,
                    'Modem not enabled, cannot scan for networks.')

        sim_path = self.Get(mm1_constants.I_MODEM, 'Sim')
        if not self.sim:
            assert sim_path == mm1_constants.ROOT_PATH
            raise pm_errors.MMMobileEquipmentError(
                pm_errors.MMMobileEquipmentError.SIM_NOT_INSERTED,
                'Cannot scan for networks because no SIM is inserted.')
        assert sim_path != mm1_constants.ROOT_PATH

        # TODO(armansito): check here for SIM lock?

        scanned = [network.ToScanDictionary()
                   for network in self.roaming_networks]

        # get home network
        sim_props = self.sim.GetAll(mm1_constants.I_SIM)
        scanned.append({
            'status': dbus.types.UInt32(
                    mm1_constants.MM_MODEM_3GPP_NETWORK_AVAILABILITY_AVAILABLE),
            'operator-long': sim_props['OperatorName'],
            'operator-short': sim_props['OperatorName'],
            'operator-code': sim_props['OperatorIdentifier'],
            'access-technology': dbus.types.UInt32(self.sim.access_technology)
        })

        self._scanned_networks = (
                {network['operator-code']: network for network in scanned})
        return scanned


    def AssignPcoValue(self, pco_value):
        """
        Stores the given value so that it is shown as the value of VendorPcoInfo
        when the modem is in a registered state.

        Always prefer this method over calling "Set" directly if the PCO value
        should be cached.

        Note: See testing.Testing.UpdatePcoInfo, which allows calling this
        method over D-Bus.

        @param pco_value: String containing the PCO value to remember.

        """
        self._cached_pco_value = pco_value
        self.UpdatePcoInfo()


    def UpdatePcoInfo(self):
        """
        Updates the current PCO value based on the registration state.

        """
        if not mm1_constants.I_MODEM_3GPP in self._properties:
            return
        state = self.Get(mm1_constants.I_MODEM_3GPP, 'RegistrationState')
        if (state == mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME or
            state == mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING):
            new_pco_value = self._cached_pco_value
        else:
            new_pco_value = ''
        self.Set(mm1_constants.I_MODEM_3GPP, 'VendorPcoInfo', new_pco_value)


    def AssignSubscriptionState(self,
                                unregistered_subscription_state,
                                registered_subscription_state):
        """
        Caches the given subscription states and updates the actual
        |SubscriptionState| property depending on the |RegistrationState|.

        @param unregistered_subscription_state: This subscription state is
                returned when the modem is not registered on a network.
        @param registered_subscription_state: This subscription state is
                returned when the modem is registered on a network.

        """
        self._cached_unregistered_subscription_state = (
                unregistered_subscription_state)
        self._cached_registered_subscription_state = (
                registered_subscription_state)
        self.UpdateSubscriptionState()


    def UpdateSubscriptionState(self):
        """
        Updates the current |SubscriptionState| property depending on the
        |RegistrationState|.

        """
        if not mm1_constants.I_MODEM_3GPP in self._properties:
            return
        registration_state = self.Get(mm1_constants.I_MODEM_3GPP,
                                      'RegistrationState')
        if (registration_state ==
            mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME or
            registration_state ==
            mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING):
            new_subscription_state = self._cached_registered_subscription_state
        else:
            new_subscription_state = (
                    self._cached_unregistered_subscription_state)

        self.SetUInt32(mm1_constants.I_MODEM_3GPP,
                       'SubscriptionState',
                       new_subscription_state)


    def UpdateLockStatus(self):
        """
        Overloads superclass implementation. Also updates
        'EnabledFacilityLocks' if 3GPP properties are exposed.

        """
        modem.Modem.UpdateLockStatus(self)
        if mm1_constants.I_MODEM_3GPP in self._properties:
            self.SetUInt32(mm1_constants.I_MODEM_3GPP,
                     'EnabledFacilityLocks',
                     self.sim.enabled_locks)


    def SetSIM(self, sim):
        """
        Overrides modem.Modem.SetSIM. Once the SIM has been assigned, attempts
        to expose 3GPP properties if SIM readable.

        @param sim: An instance of sim.SIM
        Emits:
            PropertiesChanged

        """
        modem.Modem.SetSIM(self, sim)
        self.Expose3GPPProperties()


    def Expose3GPPProperties(self):
        """
        A call to this method will attempt to expose 3GPP properties if there
        is a current SIM and is unlocked.

        """
        props = self._GetDefault3GPPProperties()
        if props:
            self.SetAll(mm1_constants.I_MODEM_3GPP, props)


    def SetRegistrationState(self, state):
        """
        Sets the 'RegistrationState' property.

        @param state: An MMModem3gppRegistrationState value.
        Emits:
            PropertiesChanged

        """
        self.SetUInt32(mm1_constants.I_MODEM_3GPP, 'RegistrationState', state)
        self.UpdatePcoInfo()
        self.UpdateSubscriptionState()


    @property
    def scanned_networks(self):
        """
        @returns: Dictionary containing the result of the most recent network
                scan, where the keys are the operator code.

        """
        return self._scanned_networks


    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM_3GPP, in_signature='s',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Register(self, operator_id, return_cb=None, raise_cb=None):
        """
        Request registration with a given modem network.

        @param operator_id: The operator ID to register. An empty string can be
                used to register to the home network.
        @param return_cb: Async success callback.
        @param raise_cb: Async error callback.

        """
        logging.info('Modem3gpp.Register: %s', operator_id)

        # Check if we're already registered with the given network.
        if (self.Get(mm1_constants.I_MODEM_3GPP, 'OperatorCode') ==
            operator_id or
            ((not operator_id and self.Get(mm1_constants.I_MODEM, 'State') >=
                    mm1_constants.MM_MODEM_STATE_REGISTERED))):
            message = 'Already registered.'
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED, message)

        if (self.Get(mm1_constants.I_MODEM, 'State') <
            mm1_constants.MM_MODEM_STATE_ENABLED):
            message = 'Cannot register the modem if not enabled.'
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED, message)

        self.CancelAllStateMachines()

        def _Reregister():
            if (self.Get(mm1_constants.I_MODEM, 'State') ==
                mm1_constants.MM_MODEM_STATE_REGISTERED):
                self.UnregisterWithNetwork()
            self.RegisterWithNetwork(operator_id, return_cb, raise_cb)

        if (self.Get(mm1_constants.I_MODEM, 'State') ==
            mm1_constants.MM_MODEM_STATE_CONNECTED):
            self.Disconnect(mm1_constants.ROOT_PATH, _Reregister, raise_cb)
        else:
            _Reregister()


    def SetRegistered(self, operator_code, operator_name):
        """
        Sets the modem to be registered with the give network. Sets the Modem
        and Modem3gpp registration states.

        @param operator_code: The operator code that should be displayed by
                the modem.
        @param operator_name: The operator name that should be displayed by
                the modem.

        """
        if operator_code:
            assert self.sim
            assert (self.Get(mm1_constants.I_MODEM, 'Sim') !=
                    mm1_constants.ROOT_PATH)
            if (operator_code ==
                self.sim.Get(mm1_constants.I_SIM, 'OperatorIdentifier')):
                state = mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME
            else:
                state = mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING
        else:
            state = mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME

        logging.info('Modem3gpp.Register: Setting registration state to %s.',
            mm1_constants.RegistrationStateToString(state))
        self.SetRegistrationState(state)
        logging.info('Modem3gpp.Register: Setting state to REGISTERED.')
        self.ChangeState(mm1_constants.MM_MODEM_STATE_REGISTERED,
            mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED)
        self.Set(mm1_constants.I_MODEM_3GPP, 'OperatorCode', operator_code)
        self.Set(mm1_constants.I_MODEM_3GPP, 'OperatorName', operator_name)


    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM_3GPP, out_signature='aa{sv}',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Scan(self, return_cb, raise_cb):
        """
        Scan for available networks.

        @param return_cb: This function is called with the result.
        @param raise_cb: This function may be called with error.
        @returns: An array of dictionaries with each array element describing a
                mobile network found in the scan. See the ModemManager reference
                manual for the list of keys that may be included in the returned
                dictionary.

        """
        scan_result = self.SyncScan()
        return_cb(scan_result)


    def RegisterWithNetwork(
            self, operator_id="", return_cb=None, raise_cb=None):
        """
        Overridden from superclass.

        @param operator_id: See superclass documentation.
        @param return_cb: See superclass documentation.
        @param raise_cb: See superclass documentation.

        """
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_REGISTER,
                self,
                operator_id,
                return_cb,
                raise_cb)
        machine.Start()


    def UnregisterWithNetwork(self):
        """
        Overridden from superclass.

        """
        logging.info('Modem3gpp.UnregisterWithHomeNetwork')
        logging.info('Setting registration state to IDLE.')
        self.SetRegistrationState(
                mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)
        logging.info('Setting state to ENABLED.')
        self.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED,
            mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED)
        self.Set(mm1_constants.I_MODEM_3GPP, 'OperatorName', '')
        self.Set(mm1_constants.I_MODEM_3GPP, 'OperatorCode', '')


    # Inherited from modem_simple.ModemSimple.
    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    def Connect(self, properties, return_cb, raise_cb):
        """
        Overriden from superclass.

        @param properties
        @param return_cb
        @param raise_cb

        """
        logging.info('Connect')
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_CONNECT,
                self,
                properties,
                return_cb,
                raise_cb)
        machine.Start()


    # Inherited from modem_simple.ModemSimple.
    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    def Disconnect(self, bearer_path, return_cb, raise_cb, *return_cb_args):
        """
        Overriden from superclass.

        @param bearer_path
        @param return_cb
        @param raise_cb
        @param return_cb_args

        """
        logging.info('Disconnect: %s', bearer_path)
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_DISCONNECT,
                self,
                bearer_path,
                return_cb,
                raise_cb,
                return_cb_args)
        machine.Start()


    # Inherited from modem_simple.ModemSimple.
    @utils.log_dbus_method()
    def GetStatus(self):
        """
        Overriden from superclass.

        """
        modem_props = self.GetAll(mm1_constants.I_MODEM)
        m3gpp_props = self.GetAll(mm1_constants.I_MODEM_3GPP)
        retval = {}
        retval['state'] = modem_props['State']
        if retval['state'] >= mm1_constants.MM_MODEM_STATE_REGISTERED:
            retval['signal-quality'] = modem_props['SignalQuality'][0]
            retval['bands'] = modem_props['CurrentBands']
            retval['access-technology'] = self.sim.access_technology
            retval['m3gpp-registration-state'] = \
                m3gpp_props['RegistrationState']
            retval['m3gpp-operator-code'] = m3gpp_props['OperatorCode']
            retval['m3gpp-operator-name'] = m3gpp_props['OperatorName']
        return retval
    # TODO(armansito): implement
    # org.freedesktop.ModemManager1.Modem.Modem3gpp.Ussd, if needed
    # (in a separate class?)
