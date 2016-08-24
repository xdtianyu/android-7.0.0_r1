# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import logging
import pprint
import sys

from autotest_lib.client.common_lib.cros import xmlrpc_types
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types


def deserialize(serialized):
    """Deserialize an argument to the XmlRpc proxy.

    @param serialized dict representing a serialized object.
    @return the corresponding deserialized object.

    """
    return xmlrpc_types.deserialize(serialized, module=sys.modules[__name__])


class AssociationParameters(xmlrpc_types.XmlRpcStruct):
    """Describes parameters used in WiFi connection attempts."""

    DEFAULT_DISCOVERY_TIMEOUT = 15
    DEFAULT_ASSOCIATION_TIMEOUT = 15
    DEFAULT_CONFIGURATION_TIMEOUT = 15
    # Mode for most routers and access points.
    STATION_TYPE_MANAGED = 'managed'
    # Mode for certain kinds of p2p networks like old Android phone hotspots.
    STATION_TYPE_IBSS = 'ibss'

    @property
    def security(self):
        """@return string security type for this network."""
        return self.security_config.security


    @property
    def security_parameters(self):
        """@return dict of service property/value pairs related to security."""
        return self.security_config.get_shill_service_properties()


    def __init__(self, ssid=None, security_config=None,
                 discovery_timeout=DEFAULT_DISCOVERY_TIMEOUT,
                 association_timeout=DEFAULT_ASSOCIATION_TIMEOUT,
                 configuration_timeout=DEFAULT_CONFIGURATION_TIMEOUT,
                 is_hidden=False, save_credentials=False, station_type=None,
                 expect_failure=False, guid=None, autoconnect=None,
                 bgscan_config=None):
        """Construct an AssociationParameters.

        @param ssid string the network to connect to (e.g. 'GoogleGuest').
        @param security_config SecurityConfig object or serialized version.
        @param discovery_timeout int timeout for discovery in seconds.
        @param association_timeout int timeout for association in seconds.
        @param configuration_timeout int timeout for configuration in seconds.
        @param is_hidden bool True iff this is a hidden service.
        @param save_credentials True iff the credentials should be saved for
                this service.
        @param station_type string station type to connect with.  Usually
                left unfilled unless we're attempting to connect to a
                non-managed BSS.  One of STATION_TYPE_* above.
        @param expect_failure bool True if we expect this connection attempt to
                fail.
        @param guid string unique identifier of this service.
        @param autoconnect: bool or None.  None indicates that this should not
                be set one way or the other, while a boolean indicates a desired
                value.

        """
        super(AssociationParameters, self).__init__()
        self.ssid = ssid
        # The security config is a little tricky.  When we're being deserialized
        # this is passed to us in the form of a dictionary which also needs
        # to be deserialized into a real object.
        if isinstance(security_config, dict):
            self.security_config = xmlrpc_security_types.deserialize(
                    security_config)
        elif security_config is not None:
            self.security_config = copy.copy(security_config)
        else:
            self.security_config = xmlrpc_security_types.SecurityConfig()

        # The bgscan configuration is similar to the security configuration.
        if isinstance(bgscan_config, dict):
            self.bgscan_config = deserialize(bgscan_config)
        elif bgscan_config is not None:
            self.bgscan_config = copy.copy(bgscan_config)
        else:
            self.bgscan_config = BgscanConfiguration()
        self.discovery_timeout = discovery_timeout
        self.association_timeout = association_timeout
        self.configuration_timeout = configuration_timeout
        self.is_hidden = is_hidden
        self.save_credentials = save_credentials
        self.station_type = station_type
        self.expect_failure = expect_failure
        self.guid = guid
        self.autoconnect = autoconnect


    def __str__(self):
        """Returns a formatted string of member parameters"""
        return pprint.pformat(self.__dict__)


class AssociationResult(xmlrpc_types.XmlRpcStruct):
    """Describes the result of an association attempt."""

    def __init__(self, success=False, discovery_time=-1.0,
                 association_time=-1.0, configuration_time=-1.0,
                 failure_reason='unknown'):
        """Construct an AssociationResult.

        @param success bool True iff we were successful in connecting to
                this WiFi network.
        @param discovery_time int number of seconds it took to find and call
                connect on a network from the time the proxy is told to connect.
                This includes scanning time.
        @param association_time int number of seconds it takes from the moment
                that we call connect to the moment we're fully associated with
                the BSS.  This includes wpa handshakes.
        @param configuration_time int number of seconds it takes from
                association till we have an IP address and mark the network as
                being either online or portalled.
        @param failure_reason int holds a descriptive reason for why the
                negotiation failed when |successs| is False.  Undefined
                otherwise.

        """
        super(AssociationResult, self).__init__()
        self.success = success
        self.discovery_time = discovery_time
        self.association_time = association_time
        self.configuration_time = configuration_time
        self.failure_reason = failure_reason


    @staticmethod
    def from_dbus_proxy_output(raw):
        """Factory for AssociationResult.

        The object which knows how to talk over DBus to shill is not part of
        autotest and as a result can't return a AssociationResult.  Instead,
        it returns a similar looing tuple, which we'll parse.

        @param raw tuple from ShillProxy.
        @return AssociationResult parsed output from ShillProxy.

        """
        return AssociationResult(success=raw[0],
                                 discovery_time=raw[1],
                                 association_time=raw[2],
                                 configuration_time=raw[3],
                                 failure_reason=raw[4])


class BgscanConfiguration(xmlrpc_types.XmlRpcStruct):
    """Describes how to configure wpa_supplicant on a DUT."""

    # Clears shill's bgscan method property on the WiFi device.
    # This causes shill to choose between simple and no bgscan
    # depending on the number of visible BSS's for a network.
    SCAN_METHOD_DEFAULT = 'default'
    # Disables background scan entirely.
    SCAN_METHOD_NONE = 'none'
    # A periodic background scan based on signal strength.
    SCAN_METHOD_SIMPLE = 'simple'

    # These three parameters come out shill's wifi.cc.
    # and refer to inputs to the simple scanning method.
    DEFAULT_SHORT_INTERVAL_SECONDS = 30
    DEFAULT_LONG_INTERVAL_SECONDS = 180
    DEFAULT_SIGNAL_THRESHOLD = -50

    def __init__(self, interface=None, signal=DEFAULT_SIGNAL_THRESHOLD,
                 short_interval=DEFAULT_SHORT_INTERVAL_SECONDS,
                 long_interval=DEFAULT_LONG_INTERVAL_SECONDS,
                 method=SCAN_METHOD_DEFAULT):
        """Construct a BgscanConfiguration.

        @param interface string interface to configure (e.g. wlan0).
        @param signal int signal threshold to scan below.
        @param short_interval int wpa_supplicant short scanning interval.
        @param long_interval int wpa_supplicant normal scanning interval.
        @param method string a valid wpa_supplicant scanning algorithm (e.g.
                any of SCAN_METHOD_* above).

        """
        super(BgscanConfiguration, self).__init__()
        self.interface = interface
        self.signal = signal
        self.short_interval = short_interval
        self.long_interval = long_interval
        self.method = method


    def set_auto_signal(self, signal_average, signal_offset=None,
                        signal_noise=None):
        """Set the signal threshold automatically from observed parameters.

        @param signal_average int average signal level.
        @param signal_offset int amount to adjust the average by.
        @param signal_noise int amount of background noise observed.

        """
        signal = signal_average
        if signal_offset:
            signal += signal_offset
        if signal_noise:
            # Compensate for real noise vs standard estimate
            signal -= 95 + signal_noise
        logging.debug('Setting signal via auto configuration: '
                      'avg=%d, offset=%r, noise=%r => signal=%d.',
                      signal_average, signal_offset, signal_noise, signal)
        self.signal = signal


class ConfigureServiceParameters(xmlrpc_types.XmlRpcStruct):
    """Describes a group of optional settings for use with ConfigureService.

    The Manager in shill has a method ConfigureService which takes a dictionary
    of parameters, and uses some of them to look up a service, and sets the
    remainder of the properties on the service.  This struct represents
    some of the optional parameters that can be set in this way.  Current
    consumers of this interface look up the service by GUID.

    """

    def __init__(self, guid, passphrase=None, autoconnect=None):
        """Construct a ConfigureServiceParameters.

        @param guid string GUID of the service to configure.
        @param passphrase string optional psk passphrase.
        @param autoconnect: bool or None.  None indicates that this should not
                be set one way or the other, while a boolean indicates a desired
                value.

        """
        super(ConfigureServiceParameters, self).__init__()
        self.guid = guid
        self.passphrase = passphrase
        self.autoconnect = autoconnect
