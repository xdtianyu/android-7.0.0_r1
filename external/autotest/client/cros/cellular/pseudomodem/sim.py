# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service
import logging

import dbus_std_ifaces
import pm_constants
import pm_errors
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class IncorrectPasswordError(pm_errors.MMMobileEquipmentError):
    """ Wrapper around MM_MOBILE_EQUIPMENT_ERROR_INCORRECT_PASSWORD. """

    def __init__(self):
        pm_errors.MMMobileEquipmentError.__init__(
                self, pm_errors.MMMobileEquipmentError.INCORRECT_PASSWORD,
                'Incorrect password')

class SimPukError(pm_errors.MMMobileEquipmentError):
    """ Wrapper around MM_MOBILE_EQUIPMENT_ERROR_SIM_PUK. """

    def __init__(self):
        pm_errors.MMMobileEquipmentError.__init__(
                self, pm_errors.MMMobileEquipmentError.SIM_PUK,
                'SIM PUK required')

class SimFailureError(pm_errors.MMMobileEquipmentError):
    """ Wrapper around MM_MOBILE_EQUIPMENT_ERROR_SIM_FAILURE. """

    def __init__(self):
        pm_errors.MMMobileEquipmentError.__init__(
                self, pm_errors.MMMobileEquipmentError.SIM_FAILURE,
                'SIM failure')

class SIM(dbus_std_ifaces.DBusProperties):
    """
    Pseudomodem implementation of the org.freedesktop.ModemManager1.Sim
    interface.

    Broadband modems usually need a SIM card to operate. Each Modem object will
    therefore expose up to one SIM object, which allows SIM-specific actions
    such as PIN unlocking.

    The SIM interface handles communication with SIM, USIM, and RUIM (CDMA SIM)
    cards.

    """

    # Multiple object paths needs to be supported so that the SIM can be
    # "reset". This allows the object to reappear on a new path as if it has
    # been reset.
    SUPPORTS_MULTIPLE_OBJECT_PATHS = True

    DEFAULT_MSIN = '1234567890'
    DEFAULT_IMSI = '888999111'
    DEFAULT_PIN = '1111'
    DEFAULT_PUK = '12345678'
    DEFAULT_PIN_RETRIES = 3
    DEFAULT_PUK_RETRIES = 10

    class Carrier:
        """
        Represents a 3GPP carrier that can be stored by a SIM object.

        """
        MCC_LIST = {
            'test' : '001',
            'us': '310',
            'de': '262',
            'es': '214',
            'fr': '208',
            'gb': '234',
            'it': '222',
            'nl': '204'
        }

        CARRIER_LIST = {
            'test' : ('test', '000', pm_constants.DEFAULT_TEST_NETWORK_PREFIX),
            'banana' : ('us', '001', 'Banana-Comm'),
            'att': ('us', '090', 'AT&T'),
            'tmobile': ('us', '026', 'T-Mobile'),
            'simyo': ('de', '03', 'simyo'),
            'movistar': ('es', '07', 'Movistar'),
            'sfr': ('fr', '10', 'SFR'),
            'three': ('gb', '20', '3'),
            'threeita': ('it', '99', '3ITA'),
            'kpn': ('nl', '08', 'KPN')
        }

        def __init__(self, carrier='test'):
           carrier = self.CARRIER_LIST.get(carrier, self.CARRIER_LIST['test'])

           self.mcc = self.MCC_LIST[carrier[0]]
           self.mnc = carrier[1]
           self.operator_name = carrier[2]
           if self.operator_name != 'Banana-Comm':
              self.operator_name = self.operator_name + ' - Fake'
           self.operator_id = self.mcc + self.mnc


    def __init__(self,
                 carrier,
                 access_technology,
                 index=0,
                 pin=DEFAULT_PIN,
                 puk=DEFAULT_PUK,
                 pin_retries=DEFAULT_PIN_RETRIES,
                 puk_retries=DEFAULT_PUK_RETRIES,
                 locked=False,
                 msin=DEFAULT_MSIN,
                 imsi=DEFAULT_IMSI,
                 config=None):
        if not carrier:
            raise TypeError('A carrier is required.')
        path = mm1_constants.MM1 + '/SIM/' + str(index)
        self.msin = msin
        self._carrier = carrier
        self.imsi = carrier.operator_id + imsi
        self._index = 0
        self._total_pin_retries = pin_retries
        self._total_puk_retries = puk_retries
        self._lock_data = {
            mm1_constants.MM_MODEM_LOCK_SIM_PIN : {
                'code' : pin,
                'retries' : pin_retries
            },
            mm1_constants.MM_MODEM_LOCK_SIM_PUK : {
                'code' : puk,
                'retries' : puk_retries
            }
        }
        self._lock_enabled = locked
        self._show_retries = locked
        if locked:
            self._lock_type = mm1_constants.MM_MODEM_LOCK_SIM_PIN
        else:
            self._lock_type = mm1_constants.MM_MODEM_LOCK_NONE
        self._modem = None
        self.access_technology = access_technology
        dbus_std_ifaces.DBusProperties.__init__(self, path, None, config)


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
        self._index += 1
        path = mm1_constants.MM1 + '/SIM/' + str(self._index)
        logging.info('SIM coming back as: ' + path)
        self.SetPath(path)


    def Reset(self):
        """ Resets the SIM. This will lock the SIM if locks are enabled. """
        self.IncrementPath()
        if not self.locked and self._lock_enabled:
            self._lock_type = mm1_constants.MM_MODEM_LOCK_SIM_PIN


    @property
    def lock_type(self):
        """
        Returns the current lock type of the SIM. Can be used to determine
        whether or not the SIM is locked.

        @returns: The lock type, as a MMModemLock value.

        """
        return self._lock_type


    @property
    def unlock_retries(self):
        """
        Returns the number of unlock retries left.

        @returns: The number of unlock retries for each lock type the SIM
                supports as a dictionary.

        """
        retries = dbus.Dictionary(signature='uu')
        if not self._show_retries:
            return retries
        for k, v in self._lock_data.iteritems():
            retries[dbus.types.UInt32(k)] = dbus.types.UInt32(v['retries'])
        return retries


    @property
    def enabled_locks(self):
        """
        Returns the currently enabled facility locks.

        @returns: The currently enabled facility locks, as a MMModem3gppFacility
                value.

        """
        if self._lock_enabled:
            return mm1_constants.MM_MODEM_3GPP_FACILITY_SIM
        return mm1_constants.MM_MODEM_3GPP_FACILITY_NONE


    @property
    def locked(self):
        """ @returns: True, if the SIM is locked. False, otherwise. """
        return not (self._lock_type == mm1_constants.MM_MODEM_LOCK_NONE or
            self._lock_type == mm1_constants.MM_MODEM_LOCK_UNKNOWN)


    @property
    def modem(self):
        """
        @returns: the modem object that this SIM is currently plugged into.

        """
        return self._modem


    @modem.setter
    def modem(self, modem):
        """
        Assigns a modem object to this SIM, so that the modem knows about it.
        This should only be called directly by a modem object.

        @param modem: The modem to be associated with this SIM.

        """
        self._modem = modem


    @property
    def carrier(self):
        """
        @returns: An instance of SIM.Carrier that contains the carrier
                information assigned to this SIM.

        """
        return self._carrier


    def _DBusPropertiesDict(self):
        imsi = self.imsi
        if self.locked:
            msin = ''
            op_id = ''
            op_name = ''
        else:
            msin = self.msin
            op_id = self._carrier.operator_id
            op_name = self._carrier.operator_name
        return {
            'SimIdentifier' : msin,
            'Imsi' : imsi,
            'OperatorIdentifier' : op_id,
            'OperatorName' : op_name
        }


    def _InitializeProperties(self):
        return { mm1_constants.I_SIM : self._DBusPropertiesDict() }


    def _UpdateProperties(self):
        self.SetAll(mm1_constants.I_SIM, self._DBusPropertiesDict())


    def _CheckCode(self, code, lock_data, next_lock, error_to_raise):
        # Checks |code| against |lock_data['code']|. If the codes don't match:
        #
        #   - if the number of retries left for |lock_data| drops down to 0,
        #     the current lock type gets set to |next_lock| and
        #     |error_to_raise| is raised.
        #
        #   - otherwise, IncorrectPasswordError is raised.
        #
        # If the codes match, no error is raised.

        if code == lock_data['code']:
            # Codes match, nothing to do.
            return

        # Codes didn't match. Figure out which error to raise based on
        # remaining retries.
        lock_data['retries'] -= 1
        self._show_retries = True
        if lock_data['retries'] == 0:
            logging.info('Retries exceeded the allowed number.')
            if next_lock:
                self._lock_type = next_lock
                self._lock_enabled = True
        else:
            error_to_raise = IncorrectPasswordError()
        self._modem.UpdateLockStatus()
        raise error_to_raise


    def _ResetRetries(self, lock_type):
        if lock_type == mm1_constants.MM_MODEM_LOCK_SIM_PIN:
            value = self._total_pin_retries
        elif lock_type == mm1_constants.MM_MODEM_LOCK_SIM_PUK:
            value = self._total_puk_retries
        else:
            raise TypeError('Invalid SIM lock type')
        self._lock_data[lock_type]['retries'] = value


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SIM, in_signature='s')
    def SendPin(self, pin):
        """
        Sends the PIN to unlock the SIM card.

        @param pin: A string containing the PIN code.

        """
        if not self.locked:
            logging.info('SIM is not locked. Nothing to do.')
            return

        if self._lock_type == mm1_constants.MM_MODEM_LOCK_SIM_PUK:
            if self._lock_data[self._lock_type]['retries'] == 0:
                raise SimFailureError()
            else:
                raise SimPukError()

        lock_data = self._lock_data.get(self._lock_type, None)
        if not lock_data:
            raise pm_errors.MMCoreError(
                pm_errors.MMCoreError.FAILED,
                'Current lock type does not match the SIM lock capabilities.')

        self._CheckCode(pin, lock_data, mm1_constants.MM_MODEM_LOCK_SIM_PUK,
                        SimPukError())

        logging.info('Entered correct PIN.')
        self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PIN)
        self._lock_type = mm1_constants.MM_MODEM_LOCK_NONE
        self._modem.UpdateLockStatus()
        self._modem.Expose3GPPProperties()
        self._UpdateProperties()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SIM, in_signature='ss')
    def SendPuk(self, puk, pin):
        """
        Sends the PUK and a new PIN to unlock the SIM card.

        @param puk: A string containing the PUK code.
        @param pin: A string containing the PIN code.

        """
        if self._lock_type != mm1_constants.MM_MODEM_LOCK_SIM_PUK:
            logging.info('No PUK lock in place. Nothing to do.')
            return

        lock_data = self._lock_data.get(self._lock_type, None)
        if not lock_data:
            raise pm_errors.MMCoreError(
                    pm_errors.MMCoreError.FAILED,
                    'Current lock type does not match the SIM locks in place.')

        if lock_data['retries'] == 0:
            raise SimFailureError()

        self._CheckCode(puk, lock_data, None, SimFailureError())

        logging.info('Entered correct PUK.')
        self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PIN)
        self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PUK)
        self._lock_data[mm1_constants.MM_MODEM_LOCK_SIM_PIN]['code'] = pin
        self._lock_type = mm1_constants.MM_MODEM_LOCK_NONE
        self._modem.UpdateLockStatus()
        self._modem.Expose3GPPProperties()
        self._UpdateProperties()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SIM, in_signature='sb')
    def EnablePin(self, pin, enabled):
        """
        Enables or disables PIN checking.

        @param pin: A string containing the PIN code.
        @param enabled: True to enable PIN, False otherwise.

        """
        if enabled:
            self._EnablePin(pin)
        else:
            self._DisablePin(pin)


    def _EnablePin(self, pin):
        # Operation fails if the SIM is locked or PIN lock is already
        # enabled.
        if self.locked or self._lock_enabled:
            raise SimFailureError()

        lock_data = self._lock_data[mm1_constants.MM_MODEM_LOCK_SIM_PIN]
        self._CheckCode(pin, lock_data, mm1_constants.MM_MODEM_LOCK_SIM_PUK,
                        SimPukError())
        self._lock_enabled = True
        self._show_retries = True
        self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PIN)
        self._UpdateProperties()
        self.modem.UpdateLockStatus()


    def _DisablePin(self, pin):
        if not self._lock_enabled:
            raise SimFailureError()

        if self.locked:
            self.SendPin(pin)
        else:
            lock_data = self._lock_data[mm1_constants.MM_MODEM_LOCK_SIM_PIN]
            self._CheckCode(pin, lock_data,
                            mm1_constants.MM_MODEM_LOCK_SIM_PUK, SimPukError())
            self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PIN)
        self._lock_enabled = False
        self._UpdateProperties()
        self.modem.UpdateLockStatus()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SIM, in_signature='ss')
    def ChangePin(self, old_pin, new_pin):
        """
        Changes the PIN code.

        @param old_pin: A string containing the old PIN code.
        @param new_pin: A string containing the new PIN code.

        """
        if not self._lock_enabled or self.locked:
            raise SimFailureError()

        lock_data = self._lock_data[mm1_constants.MM_MODEM_LOCK_SIM_PIN]
        self._CheckCode(old_pin, lock_data,
                        mm1_constants.MM_MODEM_LOCK_SIM_PUK, SimPukError())
        self._ResetRetries(mm1_constants.MM_MODEM_LOCK_SIM_PIN)
        self._lock_data[mm1_constants.MM_MODEM_LOCK_SIM_PIN]['code'] = new_pin
        self._UpdateProperties()
        self.modem.UpdateLockStatus()
