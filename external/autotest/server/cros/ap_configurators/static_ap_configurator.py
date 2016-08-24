# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import pprint
import re
import xmlrpclib

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.ap_configurators import ap_configurator
from autotest_lib.server.cros.ap_configurators import ap_spec

CartridgeCmd = collections.namedtuple('CartridgeCmd', ['method', 'args'])

# DHCP delayed devices.  Some APs need additional time for the DHCP
# server to come on-line.  These are AP based, so the BSS is used
# since that is unique.
DHCP_DELAY_DEVICES=['44:94:fc:71:88:9b', # Netgear wndr4300
                    '10:0d:7f:4d:68:3c', # Netgear wndr3700v4
                    '14:35:8b:0b:6c:80', # Medialink mwn_wapr150nv2
                    '20:4e:7f:49:86:8f'] # Netgear wpn824n

class StaticAPConfigurator(ap_configurator.APConfiguratorAbstract):
    """Derived class to supply AP configuration information."""


    def __init__(self, ap_config):
        """
        Initialize instance

        @param ap_config: AP object to configure this instance

        """
        super(StaticAPConfigurator, self).__init__()
        self._command_list = list()

        # This allows the ability to build a generic configurator
        # which can be used to get access to the members above.
        self.class_name = ap_config.get_class()
        self._short_name = ap_config.get_model()
        self.mac_address = ap_config.get_wan_mac()
        self.host_name = ap_config.get_wan_host()
        # Get corresponding PDU from host name.
        self.pdu = re.sub('host\d+', 'rpm1', self.host_name) + '.cros'
        self.channel = ap_config.get_channel()
        self.band = ap_config.get_band()
        self.current_band = ap_config.get_band()
        self.security = ap_config.get_security()
        if self.security == ap_spec.SECURITY_TYPE_MIXED:
           self.security = [ap_spec.SECURITY_TYPE_WPA2PSK,
                            ap_spec.SECURITY_TYPE_WPAPSK]
        else:
           self.security = [self.security]
        self.psk = ap_config.get_psk()
        self._ssid = ap_config.get_ssid()
        self.rpm_managed = ap_config.get_rpm_managed()

        self._configuration_success = ap_constants.CONFIG_SUCCESS
        self.config_data = ap_config

        name_dict = {'Router name': self._short_name,
                     'Controller class': self.class_name,
                     '2.4 GHz MAC Address': ap_config.get_bss(),
                     '5 GHz MAC Address': ap_config.get_bss5(),
                     'Hostname': ap_config.get_wan_host()}

        self._name = pprint.pformat(name_dict)

        # Check if a delay needs to be added for this AP.
        if (ap_config.get_bss() in DHCP_DELAY_DEVICES or
            ap_config.get_bss5() in DHCP_DELAY_DEVICES):
            self._dhcp_delay = 60

        if self.rpm_managed:
            rpm_frontend_server = global_config.global_config.get_config_value(
                    'CROS', 'rpm_frontend_uri')
            self.rpm_client = xmlrpclib.ServerProxy(
                    rpm_frontend_server, verbose=False)


    def __str__(self):
        """Prettier display of the object"""
        return('AP Name: %s\n'
               'BSS: %s\n'
               'SSID: %s\n'
               'Short name: %s' % (self.name,
                   self.config_data.get_bss(), self._ssid,
                   self.short_name))


    @property
    def ssid(self):
        """Returns the SSID."""
        return self._ssid


    def power_down_router(self):
        """power down via rpm"""
        if self.rpm_managed:
            self._command_list.append(CartridgeCmd(
                    self.rpm_client.queue_request,
                    [self.host_name, 'OFF']))


    def power_up_router(self):
        """power up via rpm"""
        if self.rpm_managed:
            self._command_list.append(CartridgeCmd(
                    self.rpm_client.queue_request,
                    [self.host_name, 'ON']))


    def set_using_ap_spec(self, set_ap_spec, power_up=True):
        """
        Sets all configurator options.

        Note: for StaticAPs there is no config required, so the only action
        here is to power up if needed

        @param set_ap_spec: APSpec object

        """
        if power_up and self.rpm_managed:
            self.power_up_router()


    def apply_settings(self):
        """Allow cartridge to run commands in _command_list"""
        self.check_pdu_status()
        for command in self._command_list:
            command.method(*command.args)


    def reset_command_list(self):
        """Resets all internal command state."""
        self._command_list = list()


    @property
    def name(self):
        """Returns a string to describe the router."""
        return self._name


    @property
    def short_name(self):
        """Returns a short string to describe the router."""
        return self._short_name


    def get_supported_bands(self):
        """Returns a list of dictionaries describing the supported bands.

        Example: returned is a dictionary of band and a list of channels. The
                 band object returned must be one of those defined in the
                 __init___ of this class.

        supported_bands = [{'band' : self.band_2GHz,
                            'channels' : [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                           {'band' : self.band_5ghz,
                            'channels' : [26, 40, 44, 48, 149, 153, 165]}]

        @return a list of dictionaries as described above

        """
        supported_bands = [{'band' : self.band,
                            'channels' : [self.channel]}]

        return supported_bands


    def get_supported_modes(self):
        """
        Returns a list of dictionaries describing the supported modes.

        Example: returned is a dictionary of band and a list of modes. The band
                 and modes objects returned must be one of those defined in the
                 __init___ of this class.

        supported_modes = [{'band' : ap_spec.BAND_2GHZ,
                            'modes' : [mode_b, mode_b | mode_g]},
                           {'band' : ap_spec.BAND_5GHZ,
                            'modes' : [mode_a, mode_n, mode_a | mode_n]}]

        @return a list of dictionaries as described above

        """
        supported_modes = [{'band' : self.band,
                            'modes' : [ap_spec.DEFAULT_5GHZ_MODE
                    if self.channel in ap_spec.VALID_5GHZ_CHANNELS
                    else ap_spec.DEFAULT_2GHZ_MODE]}]

        return supported_modes


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return False

        """
        return False


    def is_band_and_channel_supported(self, band, channel):
        """
        Returns if a given band and channel are supported.

        @param band: the band to check if supported
        @param channel: the channel to check if supported

        @return True if combination is supported; False otherwise.

        """
        bands = self.get_supported_bands()
        for current_band in bands:
            if (current_band['band'] == band and
                channel in current_band['channels']):
                return True
        return False


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one of the following modes:
                         self.security_disabled,
                         self.security_wep,
                         self.security_wpapsk,
                         self.security_wpa2psk

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in self.security


    def get_association_parameters(self):
        """
        Creates an AssociationParameters from the configured AP.

        @returns AssociationParameters for the configured AP.

        """
        security_config = None
        if (ap_spec.SECURITY_TYPE_WPAPSK in self.security or
            ap_spec.SECURITY_TYPE_WPA2PSK in self.security):
            # Not all of this is required but doing it just in case.
            security_config = xmlrpc_security_types.WPAConfig(
                    psk=self.psk,
                    wpa_mode=xmlrpc_security_types.WPAConfig.MODE_MIXED_WPA,
                    wpa_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP,
                                 xmlrpc_security_types.WPAConfig.CIPHER_TKIP],
                    wpa2_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
        # TODO(jabele) Allow StaticAPs configured as hidden
        #              by way of the ap_config file
        return xmlrpc_datatypes.AssociationParameters(
                ssid=self._ssid, security_config=security_config,
                discovery_timeout=45, association_timeout=30,
                configuration_timeout=30, is_hidden=False)
