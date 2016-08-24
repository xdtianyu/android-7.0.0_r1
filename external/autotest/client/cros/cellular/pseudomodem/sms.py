# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

import dbus_std_ifaces

from autotest_lib.client.cros.cellular import mm1_constants

class SMSConfigException(Exception):
    """
    Raised when an error occurs while setting the SMS property template.

    """
    pass


class SMS(dbus_std_ifaces.DBusProperties):
    """
    Pseudomodem implementation of the org.freedesktop.ModemManager1.Sms
    interface.

    The SMS interface defines operations and properties of a single SMS
    message.

    Modems implementing the Messaging interface will export one SMS object for
    each SMS stored in the device.

    """

    _sms_index = 0
    _props_template = {}
    _settable_props = set([ 'SMSC', 'Validity', 'Class', 'Storage',
                            'DeliveryReportRequest' ])

    def __init__(self, bus, sender_number, content):
        self._sender_number = sender_number
        self._content = content
        dbus_std_ifaces.DBusProperties.__init__(
                self, self._get_next_sms_path(), bus)


    @classmethod
    def _get_next_sms_path(cls):
        path = mm1_constants.SMS_PATH + '/' + str(cls._sms_index)
        cls._sms_index += 1
        return path


    @classmethod
    def set_config(cls, params):
        """
        Sets the values that should be used for SMS properties when a new
        SMS is constructed.

        @param params: A dictionary containing properties and values to set.
                Only some properties are allowed to be set through this method,
                which are contained in |_settable_props|. A value of "default"
                can be used (which is a string) to use the default value for
                that dictionary when constructing the next SMS object.
        @raises: SMSConfigException, if params is malformed or contains a
                disallowed property.

        """
        if not isinstance(params, dict):
            raise SMSConfigException('sms.SMS.set_config only accepts '
                                     'dictionaries.')
        keyset = set(params)
        if not keyset.issubset(cls._settable_props):
            raise SMSConfigException(
                    'Properties: ' + repr(keyset.difference(params)) + ' are '
                    'not settable.')

        for key, value in params.iteritems():
            if value == 'default' and cls._props_template.has_key(key):
                cls._props_template.pop(key)
            else:
                cls._props_template[key] = value


    def _InitializeProperties(self):
        props = {}
        props['State'] = dbus.types.UInt32(mm1_constants.MM_SMS_STATE_UNKNOWN)
        props['PduType'] = dbus.types.UInt32(
                mm1_constants.MM_SMS_PDU_TYPE_UNKNOWN)
        props['Number'] = self._sender_number
        # For now, only support 'Text' and not 'Data'
        props['Text'] = self._content
        props['SMSC'] = self._props_template.get('SMSC', '1231212')
        props['Validity'] = self._props_template.get('Validity',
                dbus.types.Struct(
                        [dbus.types.UInt32(
                                mm1_constants.MM_SMS_VALIDITY_TYPE_UNKNOWN),
                         dbus.types.UInt32(0)],
                        signature='uv'))
        props['Class'] = self._props_template.get('Class', dbus.types.Int32(-1))
        props['DeliveryReportRequest'] = self._props_template.get(
                'DeliveryReportRequest',
                dbus.types.Boolean(False))
        props['Storage'] = self._props_template.get(
                'Storage',
                dbus.types.UInt32(mm1_constants.MM_SMS_STORAGE_UNKNOWN))
        # TODO(armansito): This may be useful for split SMS messages. Need to
        # study the SMS standard to figure out how to make use of this
        # property.
        props['MessageReference'] =  dbus.types.UInt32(0)

        # Timestamp, DischargeTimestamp, and DeliveryState won't be available
        # until an action (such as send, receive, status report) is take with
        # the SMS.
        props['Timestamp'] = ''
        props['DischargeTimestamp'] = ''
        return { mm1_constants.I_SMS: props }


    # Remember to decorate your concrete implementation with
    # @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SMS)
    def Send(self):
        """ If the message has not yet been sent, queue it for delivery. """
        raise NotImplementedError()


    # Remember to decorate your concrete implementation with
    # @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_SMS, in_signature='u')
    def Store(self, storage):
        """
        Stores the message in the device if not already done.

        @param storage: An MMSmsStorage value.

        """
        raise NotImplementedError()
