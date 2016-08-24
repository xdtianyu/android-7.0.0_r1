# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import subprocess
import time

from autotest_lib.client.cros.networking import shill_proxy


class WifiProxy(shill_proxy.ShillProxy):
    """Wrapper around shill dbus interface used by wifi tests."""


    def set_logging_for_wifi_test(self):
        """Set the logging in shill for a test of wifi technology.

        Set the log level to |ShillProxy.LOG_LEVEL_FOR_TEST| and the log scopes
        to the ones defined in |ShillProxy.LOG_SCOPES_FOR_TEST| for
        |ShillProxy.TECHNOLOGY_WIFI|.

        """
        self.set_logging_for_test(self.TECHNOLOGY_WIFI)


    def remove_all_wifi_entries(self):
        """Iterate over all pushed profiles and remove WiFi entries."""
        profiles = self.get_profiles()
        for profile in profiles:
            profile_properties = profile.GetProperties(utf8_strings=True)
            entries = profile_properties[self.PROFILE_PROPERTY_ENTRIES]
            for entry_id in entries:
                try:
                    entry = profile.GetEntry(entry_id)
                except dbus.exceptions.DBusException as e:
                    logging.error('Unable to retrieve entry %s', entry_id)
                    continue
                if entry[self.ENTRY_FIELD_TYPE] == 'wifi':
                    profile.DeleteEntry(entry_id)


    def configure_wifi_service(self, ssid, security, security_parameters={},
                               save_credentials=True, station_type=None,
                               hidden_network=False, guid=None,
                               autoconnect=None):
        """Configure a WiFi service.

        @param ssid string name of network to connect to.
        @param security string type of security used in network (e.g. psk)
        @param security_parameters dict of service property/value pairs that
            make up the credentials and settings for the given security
            type (e.g. the passphrase for psk security).
        @param save_credentials bool True if we should save EAP credentials.
        @param station_type string one of SUPPORTED_WIFI_STATION_TYPES.
        @param hidden_network bool True when the SSID is not broadcasted.
        @param guid string unique identifier for network.
        @param autoconnect bool or None.  None indicates that this should not
            be set one way or the other, while a boolean indicates a desired
            value.

        """
        # |mode| is derived from the station type we're attempting to join.  It
        # does not refer to the 802.11x (802.11a/b/g/n) type.  It refers to a
        # shill connection mode.
        mode = self.SUPPORTED_WIFI_STATION_TYPES[station_type]
        config_params = {self.SERVICE_PROPERTY_TYPE: 'wifi',
                         self.SERVICE_PROPERTY_HIDDEN: hidden_network,
                         self.SERVICE_PROPERTY_SSID: ssid,
                         self.SERVICE_PROPERTY_SECURITY_CLASS: security,
                         self.SERVICE_PROPERTY_MODE: mode}
        if autoconnect is not None:
            config_params[self.SERVICE_PROPERTY_AUTOCONNECT] = autoconnect
        config_params.update(security_parameters)
        if guid is not None:
            config_params[self.SERVICE_PROPERTY_GUID] = guid
        try:
            self.configure_service(config_params)
        except dbus.exceptions.DBusException as e:
            logging.error('Caught an error while configuring a WiFi '
                          'service: %r', e)
            return False

        logging.info('Configured service: %s', ssid)
        return True


    def connect_to_wifi_network(self,
                                ssid,
                                security,
                                security_parameters,
                                save_credentials,
                                station_type=None,
                                hidden_network=False,
                                guid=None,
                                autoconnect=None,
                                discovery_timeout_seconds=15,
                                association_timeout_seconds=15,
                                configuration_timeout_seconds=15):
        """
        Connect to a WiFi network with the given association parameters.

        @param ssid string name of network to connect to.
        @param security string type of security used in network (e.g. psk)
        @param security_parameters dict of service property/value pairs that
                make up the credentials and settings for the given security
                type (e.g. the passphrase for psk security).
        @param save_credentials bool True if we should save EAP credentials.
        @param station_type string one of SUPPORTED_WIFI_STATION_TYPES.
        @param hidden_network bool True when the SSID is not broadcasted.
        @param guid string unique identifier for network.
        @param discovery_timeout_seconds float timeout for service discovery.
        @param association_timeout_seconds float timeout for service
            association.
        @param configuration_timeout_seconds float timeout for DHCP
            negotiations.
        @param autoconnect: bool or None.  None indicates that this should not
            be set one way or the other, while a boolean indicates a desired
            value.
        @return (successful, discovery_time, association_time,
                 configuration_time, reason)
            where successful is True iff the operation succeeded, *_time is
            the time spent waiting for each transition, and reason is a string
            which may contain a meaningful description of failures.

        """
        logging.info('Attempting to connect to %s', ssid)
        service_proxy = None
        start_time = time.time()
        discovery_time = -1.0
        association_time = -1.0
        configuration_time = -1.0
        if station_type not in self.SUPPORTED_WIFI_STATION_TYPES:
            return (False, discovery_time, association_time,
                    configuration_time,
                    'FAIL(Invalid station type specified.)')

        # |mode| is derived from the station type we're attempting to join.  It
        # does not refer to the 802.11x (802.11a/b/g/n) type.  It refers to a
        # shill connection mode.
        mode = self.SUPPORTED_WIFI_STATION_TYPES[station_type]

        if hidden_network:
            logging.info('Configuring %s as a hidden network.', ssid)
            if not self.configure_wifi_service(
                    ssid, security, save_credentials=save_credentials,
                    station_type=station_type, hidden_network=True,
                    autoconnect=autoconnect):
                return (False, discovery_time, association_time,
                        configuration_time,
                        'FAIL(Failed to configure hidden SSID)')

            logging.info('Configured hidden service: %s', ssid)


        logging.info('Discovering...')
        discovery_params = {self.SERVICE_PROPERTY_TYPE: 'wifi',
                            self.SERVICE_PROPERTY_NAME: ssid,
                            self.SERVICE_PROPERTY_SECURITY_CLASS: security,
                            self.SERVICE_PROPERTY_MODE: mode}
        while time.time() - start_time < discovery_timeout_seconds:
            discovery_time = time.time() - start_time
            service_object = self.find_matching_service(discovery_params)
            if service_object:
                try:
                    service_properties = service_object.GetProperties(
                            utf8_strings=True)
                except dbus.exceptions.DBusException:
                    # This usually means the service handle has become invalid.
                    # Which is sort of like not getting a handle back from
                    # find_matching_service in the first place.
                    continue
                strength = self.dbus2primitive(
                        service_properties[self.SERVICE_PROPERTY_STRENGTH])
                if strength > 0:
                    logging.info('Discovered service: %s. Strength: %r.',
                                 ssid, strength)
                    break

            # This is spammy, but shill handles that for us.
            self.manager.RequestScan('wifi')
            time.sleep(self.POLLING_INTERVAL_SECONDS)
        else:
            return (False, discovery_time, association_time,
                    configuration_time, 'FAIL(Discovery timed out)')

        # At this point, we know |service| is in the service list.  Attempt
        # to connect it, and watch the states roll by.
        logging.info('Connecting...')
        try:
            for service_property, value in security_parameters.iteritems():
                service_object.SetProperty(service_property, value)
            if guid is not None:
                service_object.SetProperty(self.SERVICE_PROPERTY_GUID, guid)
            if autoconnect is not None:
                service_object.SetProperty(self.SERVICE_PROPERTY_AUTOCONNECT,
                                           autoconnect)
            service_object.Connect()
            logging.info('Called connect on service')
        except dbus.exceptions.DBusException, e:
            logging.error('Caught an error while trying to connect: %s',
                          e.get_dbus_message())
            return (False, discovery_time, association_time,
                    configuration_time, 'FAIL(Failed to call connect)')

        logging.info('Associating...')
        result = self.wait_for_property_in(
                service_object,
                self.SERVICE_PROPERTY_STATE,
                ('configuration', 'ready', 'portal', 'online'),
                association_timeout_seconds)
        (successful, _, association_time) = result
        if not successful:
            return (False, discovery_time, association_time,
                    configuration_time, 'FAIL(Association timed out)')

        logging.info('Associated with service: %s', ssid)

        logging.info('Configuring...')
        result = self.wait_for_property_in(
                service_object,
                self.SERVICE_PROPERTY_STATE,
                ('ready', 'portal', 'online'),
                configuration_timeout_seconds)
        (successful, _, configuration_time) = result
        if not successful:
            return (False, discovery_time, association_time,
                    configuration_time, 'FAIL(Configuration timed out)')

        logging.info('Configured service: %s', ssid)

        # Great success!
        logging.info('Connected to WiFi service.')
        return (True, discovery_time, association_time, configuration_time,
                'SUCCESS(Connection successful)')


    def disconnect_from_wifi_network(self, ssid, timeout=None):
        """Disconnect from the specified WiFi network.

        Method will succeed if it observes the specified network in the idle
        state after calling Disconnect.

        @param ssid string name of network to disconnect.
        @param timeout float number of seconds to wait for idle.
        @return tuple(success, duration, reason) where:
            success is a bool (True on success).
            duration is a float number of seconds the operation took.
            reason is a string containing an informative error on failure.

        """
        if timeout is None:
            timeout = self.SERVICE_DISCONNECT_TIMEOUT
        service_description = {self.SERVICE_PROPERTY_TYPE: 'wifi',
                               self.SERVICE_PROPERTY_NAME: ssid}
        service = self.find_matching_service(service_description)
        if service is None:
            return (False,
                    0.0,
                    'Failed to disconnect from %s, service not found.' % ssid)

        service.Disconnect()
        result = self.wait_for_property_in(service,
                                           self.SERVICE_PROPERTY_STATE,
                                           ('idle',),
                                           timeout)
        (successful, final_state, duration) = result
        message = 'Success.'
        if not successful:
            message = ('Failed to disconnect from %s, '
                       'timed out in state: %s.' % (ssid, final_state))
        return (successful, duration, message)


    def configure_bgscan(self, interface, method=None, short_interval=None,
                         long_interval=None, signal=None):
        """Configures bgscan parameters for wpa_supplicant.

        @param interface string name of interface to configure (e.g. 'mlan0').
        @param method string bgscan method (e.g. 'none').
        @param short_interval int short scanning interval.
        @param long_interval int normal scanning interval.
        @param signal int signal threshold.

        """
        device = self.find_object('Device', {'Name': interface})
        if device is None:
            logging.error('No device found with name: %s', interface)
            return False

        attributes = {'ScanInterval': (dbus.UInt16, long_interval),
                      'BgscanMethod': (dbus.String, method),
                      'BgscanShortInterval': (dbus.UInt16, short_interval),
                      'BgscanSignalThreshold': (dbus.Int32, signal)}
        for k, (type_cast, value) in attributes.iteritems():
            if value is None:
                continue

            # 'default' is defined in:
            # client/common_lib/cros/network/xmlrpc_datatypes.py
            # but we don't have access to that file here.
            if value == 'default':
                device.ClearProperty(k)
            else:
                device.SetProperty(k, type_cast(value))
        return True


    def get_active_wifi_SSIDs(self):
        """@return list of string SSIDs with at least one BSS we've scanned."""
        properties = self.manager.GetProperties(utf8_strings=True)
        services = [self.get_dbus_object(self.DBUS_TYPE_SERVICE, path)
                    for path in properties[self.MANAGER_PROPERTY_SERVICES]]
        wifi_services = []
        for service in services:
            try:
                service_properties = self.dbus2primitive(service.GetProperties(
                        utf8_strings=True))
            except dbus.exceptions.DBusException as e:
                pass  # Probably the service disappeared before GetProperties().
            logging.debug('Considering service with properties: %r',
                          service_properties)
            service_type = service_properties[self.SERVICE_PROPERTY_TYPE]
            strength = service_properties[self.SERVICE_PROPERTY_STRENGTH]
            if service_type == 'wifi' and strength > 0:
                # Note that this may cause terrible things if the SSID
                # is not a valid ASCII string.
                ssid = service_properties[self.SERVICE_PROPERTY_HEX_SSID]
                logging.info('Found active WiFi service: %s', ssid)
                wifi_services.append(ssid.decode('hex'))
        return wifi_services


    def wait_for_service_states(self, ssid, states, timeout_seconds):
        """Wait for a service (ssid) to achieve one of a number of states.

        @param ssid string name of network for whose state we're waiting.
        @param states tuple states for which to wait.
        @param timeout_seconds seconds to wait for property to be achieved
        @return tuple(successful, final_value, duration)
            where successful is True iff we saw one of |states|, final_value
            is the final state we saw, and duration is how long we waited to
            see that value.

        """
        discovery_params = {self.SERVICE_PROPERTY_TYPE: 'wifi',
                            self.SERVICE_PROPERTY_NAME: ssid}
        start_time = time.time()
        service_object = None
        while time.time() - start_time < timeout_seconds:
            service_object = self.find_matching_service(discovery_params)
            if service_object:
                break

            time.sleep(self.POLLING_INTERVAL_SECONDS)
        else:
            logging.error('Timed out waiting for %s states', ssid)
            return False, 'unknown', timeout_seconds

        return self.wait_for_property_in(
                service_object,
                self.SERVICE_PROPERTY_STATE,
                states,
                timeout_seconds - (time.time() - start_time))
