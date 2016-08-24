# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.types
import logging

import modem
import pm_constants
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class ModemCdma(modem.Modem):
    """
    Pseudomodem implementation of the
    org.freedesktop.ModemManager1.Modem.ModemCdma and
    org.freedesktop.ModemManager1.Modem.Simple interfaces. This class provides
    access to specific actions that may be performed in modems with CDMA
    capabilities.

    """

    class CdmaNetwork(object):
        """
        Stores carrier specific information needed for a CDMA network.

        """
        def __init__(self,
                     sid=99998,
                     nid=0,
                     activated=True,
                     mdn='5555555555',
                     standard='evdo'):
            self.sid = sid
            self.nid = nid
            self.standard = standard
            self.activated = activated
            self._mdn = mdn


        @property
        def mdn(self):
            """
            @returns: The 'Mobile Directory Number' assigned to this modem by
                    the carrier. If not activated, the first 6 digits will
                    contain '0'.

            """
            if self.activated:
                return self._mdn
            return '000000' + self._mdn[6:]


    def __init__(self,
                 state_machine_factory=None,
                 home_network=CdmaNetwork(),
                 bus=None,
                 device='pseudomodem0',
                 roaming_networks=None,
                 config=None):
        self.home_network = home_network
        self.cdma_activate_step = None
        modem.Modem.__init__(self,
                             state_machine_factory,
                             bus=bus,
                             device=device,
                             roaming_networks=roaming_networks,
                             config=config)


    def _InitializeProperties(self):
        ip = modem.Modem._InitializeProperties(self)
        if self.home_network and self.home_network.activated:
            activation_state = \
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED
        else:
            activation_state = \
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED
        ip[mm1_constants.I_MODEM_CDMA] = {
            'Meid' : 'A100000DCE2CA0',
            'Esn' : 'EDD1EDD1',
            'Sid' : dbus.types.UInt32(0),
            'Nid' : dbus.types.UInt32(0),
            'Cdma1xRegistrationState' : (
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)),
            'EvdoRegistrationState' : (
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)),
            'ActivationState' : dbus.types.UInt32(activation_state)
        }
        props = ip[mm1_constants.I_MODEM]
        props['SupportedCapabilities'] = [
            dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_CDMA_EVDO)
        ]
        props['CurrentCapabilities'] = (
            dbus.types.UInt32(mm1_constants.MM_MODEM_CAPABILITY_CDMA_EVDO))
        props['MaxBearers'] = dbus.types.UInt32(1)
        props['MaxActiveBearers'] = dbus.types.UInt32(1)
        props['EquipmentIdentifier'] = ip[mm1_constants.I_MODEM_CDMA]['Meid']
        props['AccessTechnologies'] = (
            dbus.types.UInt32(mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_EVDO0))
        props['SupportedModes'] = [
            dbus.types.Struct(
                [dbus.types.UInt32(mm1_constants.MM_MODEM_MODE_3G),
                 dbus.types.UInt32(mm1_constants.MM_MODEM_MODE_4G)],
                signature='uu')
        ]
        props['CurrentModes'] = props['SupportedModes'][0]
        props['SupportedBands'] = [
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_BAND_CDMA_BC0_CELLULAR_800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC1_PCS_1900),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC2_TACS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC3_JTACS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC4_KOREAN_PCS),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC5_NMT450),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC6_IMT2000),
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_BAND_CDMA_BC7_CELLULAR_700),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC8_1800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC9_900),
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_BAND_CDMA_BC10_SECONDARY_800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC11_PAMR_400)
        ]
        props['CurrentBands'] = [
            dbus.types.UInt32(
                mm1_constants.MM_MODEM_BAND_CDMA_BC0_CELLULAR_800),
            dbus.types.UInt32(mm1_constants.MM_MODEM_BAND_CDMA_BC1_PCS_1900),
        ]
        if self.home_network:
            props['OwnNumbers'] = [self.home_network.mdn]
        else:
            props['OwnNumbers'] = []

        return ip


    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    @dbus.service.method(mm1_constants.I_MODEM_CDMA, in_signature='s',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Activate(self, carrier, return_cb, raise_cb):
        """
        Provisions the modem for use with a given carrier using the modem's
        OTA activation functionality, if any.

        @param carrier: Automatic activation code.
        @param return_cb: Asynchronous success callback.
        @param raise_cb: Asynchronous failure callback. Has to take an instance
                         of Exception or Error.
        Emits:
            ActivationStateChanged

        """
        logging.info('ModemCdma.Activate')
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_CDMA_ACTIVATE,
                self,
                return_cb,
                raise_cb)
        machine.Start()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_MODEM_CDMA, in_signature='a{sv}')
    def ActivateManual(self, properties):
        """
        Sets the modem provisioning data directly, without contacting the
        carrier over the air. Some modems will reboot after this call is made.

        @param properties: A dictionary of properties to set on the modem,
                           including "mdn" and "min".
        Emits:
            ActivationStateChanged

        """
        raise NotImplementedError()


    @dbus.service.signal(mm1_constants.I_MODEM_CDMA, signature='uua{sv}')
    def ActivationStateChanged(
            self,
            activation_state,
            activation_error,
            status_changes):
        """
        The device activation state changed.

        @param activation_state: Current activation state, given as a
                MMModemCdmaActivationState.
        @param activation_error: Carrier-specific error code, given as a
                MMCdmaActivationError.
        @param status_changes: Properties that have changed as a result of this
                activation state chage, including "mdn" and "min".

        """
        logging.info('ModemCdma: activation state changed: state: %u, error: '
                     '%u, status_changes: %s',
                     activation_state,
                     activation_error,
                     str(status_changes))


    def IsPendingActivation(self):
        """
        @returns: True, if a CdmaActivationMachine is currently active.

        """
        return self.cdma_activate_step and \
            not self.cdma_activate_step.cancelled


    def ChangeActivationState(self, state, error):
        """
        Changes the activation state of this modem to the one provided.

        If the requested state is MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED and
        a cdma_activation_machine.CdmaActivationMachine associated with this
        modem is currently active, then this method won't update the DBus
        properties until after the modem has reset.

        @param state: Requested activation state, given as a
                MMModemCdmaActivationState.
        @param error: Carrier-specific error code, given as a
                MMCdmaActivationError.

        """
        status_changes = {}
        if state == mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED:
            self.home_network.activated = True
            status_changes['mdn'] = [self.home_network.mdn]
            self.Set(mm1_constants.I_MODEM, 'OwnNumbers', status_changes['mdn'])

            if self.IsPendingActivation():
                logging.info("A CdmaActivationMachine is currently active. "
                             "Deferring setting the 'ActivationState' property"
                             " to ACTIVATED until after the reset is "
                             "complete.")
                return

        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'ActivationState', state)
        self.ActivationStateChanged(state, error, status_changes)


    def GetHomeNetwork(self):
        """
        @returns: A instance of CdmaNetwork that represents the
                current home network that is assigned to this modem.

        """
        return self.home_network


    def SetRegistered(self, network):
        """
        Sets the modem to be registered on the given network. Configures the
        'ActivationState', 'Sid', and 'Nid' properties accordingly.

        @param network: An instance of CdmaNetwork.

        """
        logging.info('ModemCdma.SetRegistered')
        if network:
            state = mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_HOME
            sid = network.sid
            nid = network.nid
            if network.activated:
                activation_state = \
                    mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED
            else:
                activation_state = \
                    mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED
        else:
            state = mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN
            sid = 0
            nid = 0
            activation_state = \
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED
        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'ActivationState',
                       activation_state)
        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'Sid', sid)
        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'Nid', nid)
        self.SetRegistrationState(state)


    def SetRegistrationState(self, state):
        """
        Sets the CDMA1x and EVDO registration states to the provided value.

        @param state: A MMModemCdmaRegistrationState value.

        """
        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'Cdma1xRegistrationState',
                       state)
        self.SetUInt32(mm1_constants.I_MODEM_CDMA, 'EvdoRegistrationState',
                       state)


    # Inherited from modem.Modem.
    def RegisterWithNetwork(
            self, operator_id="", return_cb=None, raise_cb=None):
        """ Overridden from superclass.

        @param operator_id: See superclass.
        @param return_cb: See superclass.
        @param raise_cb: See superclass.

        """
        logging.info('ModemCdma.RegisterWithNetwork')
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_REGISTER_CDMA,
                self,
                operator_id,
                return_cb,
                raise_cb)
        machine.Start()


    def UnregisterWithNetwork(self):
        """ Overridden from superclass. """
        logging.info('ModemCdma.UnregisterWithNetwork')
        if self.Get(mm1_constants.I_MODEM, 'State') != \
            mm1_constants.MM_MODEM_STATE_REGISTERED:
            logging.info('Currently not registered. Nothing to do.')
            return
        logging.info('Setting state to ENABLED.')
        self.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED,
            mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED)
        logging.info('Unregistering.')
        self.SetRegistered(None)


    # Inherited from modem_simple.ModemSimple.
    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    def Connect(self, properties, return_cb, raise_cb):
        """
        Overridden from superclass.

        @param properties
        @param return_cb
        @param raise_cb

        """
        logging.info('ModemCdma.Connect')
        machine = self._state_machine_factory.CreateMachine(
                pm_constants.STATE_MACHINE_CONNECT_CDMA,
                self,
                properties,
                return_cb,
                raise_cb)
        machine.Start()


    # Inherited from modem_simple.ModemSimple.
    @utils.log_dbus_method(return_cb_arg='return_cb', raise_cb_arg='raise_cb')
    def Disconnect(self, bearer_path, return_cb, raise_cb, *return_cb_args):
        """
        Overridden from superclass.

        @param bearer_path
        @param return_cb
        @param raise_cb
        @param return_cb_args

        """
        logging.info('ModemCdma.Disconnect: %s', bearer_path)
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
        """ Overridden from superclass. """
        modem_props = self.GetAll(mm1_constants.I_MODEM)
        cdma_props = self.GetAll(mm1_constants.I_MODEM_CDMA)
        retval = {}
        retval['state'] = modem_props['State']
        if retval['state'] >= mm1_constants.MM_MODEM_STATE_REGISTERED:
            retval['signal-quality'] = modem_props['SignalQuality'][0]
            retval['bands'] = modem_props['CurrentBands']
            retval['cdma-cdma1x-registration-state'] = \
                cdma_props['Cdma1xRegistrationState']
            retval['cdma-evdo-registration-state'] = \
                cdma_props['EvdoRegistrationState']
            retval['cdma-sid'] = cdma_props['Sid']
            retval['cdma-nid'] = cdma_props['Nid']
        return retval
