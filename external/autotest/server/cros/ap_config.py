# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ConfigParser
import logging
import os
import time

from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.site_utils.rpm_control_system import rpm_client
from autotest_lib.server.cros.ap_configurators import ap_spec

AP_CONFIG_FILES = { ap_constants.AP_TEST_TYPE_CHAOS:
                    ('chaos_dynamic_ap_list.conf', 'chaos_shadow_ap_list.conf'),
                    ap_constants.AP_TEST_TYPE_CLIQUE:
                    ('clique_ap_list.conf',)}

TIMEOUT = 100

def get_ap_list(ap_test_type):
    """
    Returns the list of AP's from the corresponding configuration file.

    @param ap_test_type: Used to determine which type of test we're
                         currently running (Chaos vs Clique).
    @returns a list of AP objects.

    """
    aps = []
    ap_config_files = AP_CONFIG_FILES.get(ap_test_type, None)
    for filename in ap_config_files:
        ap_config = ConfigParser.RawConfigParser(
                {AP.CONF_RPM_MANAGED: 'False'})
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            filename)
        if not os.path.exists(path):
            logging.warning('Skipping missing config: "%s"', path)
            continue

        logging.debug('Reading config from: "%s"', path)
        ap_config.read(path)
        for bss in ap_config.sections():
            aps.append(AP(bss, ap_config))
    return aps


class APPowerException(Exception):
    """ Exception raised when AP fails to power on. """
    pass

class APSectionError(Exception):
    """ Exception raised when AP instance does not exist in the config. """
    pass

class AP(object):
    """ An instance of an ap defined in the chaos config file.

    This object is a wrapper that can be used to retrieve information
    about an AP in the chaos lab, and control its power.

    """


    # Keys used in the config file.
    CONF_SSID = 'ssid'
    CONF_BRAND = 'brand'
    CONF_MODEL = 'model'
    CONF_WAN_MAC = 'wan mac'
    CONF_WAN_HOST = 'wan_hostname'
    CONF_RPM_MANAGED = 'rpm_managed'
    CONF_BSS = 'bss'
    CONF_BSS5 = 'bss5'
    CONF_BANDWIDTH = 'bandwidth'
    CONF_SECURITY = 'security'
    CONF_PSK = 'psk'
    CONF_FREQUENCY = 'frequency'
    CONF_BAND = 'band'
    CONF_CHANNEL = 'channel'
    CONF_CLASS = 'class_name'
    CONF_ADMIN = 'admin_url'


    def __init__(self, bss, config):
        """
        Intialize object

        @param bss: string containing bssid
        @param config: ConfigParser read from file

        """
        if not config.has_section(bss):
            raise APSectionError('BSS (%s) not defined.' % bss)
        self.bss = bss
        self.ap_config = config


    def get_ssid(self):
        """@return string ssid for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_SSID)


    def get_brand(self):
        """@return string brand for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_BRAND)


    def get_model(self):
        """@return string model for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_MODEL)


    def get_wan_mac(self):
        """@return string mac for WAN port of AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_WAN_MAC)


    def get_wan_host(self):
        """@return string host for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_WAN_HOST)


    def get_rpm_managed(self):
        """@return bool for AP power via rpm from config file"""
        return self.ap_config.getboolean(self.bss, self.CONF_RPM_MANAGED)


    def get_bss(self):
        """@return string bss for AP from config file"""
        try:
            bss = self.ap_config.get(self.bss, self.CONF_BSS)
        except ConfigParser.NoOptionError as e:
            bss = 'N/A'
        return bss


    def get_bss5(self):
        """@return string bss5 for AP from config file"""
        try:
            bss5 = self.ap_config.get(self.bss, self.CONF_BSS5)
        except ConfigParser.NoOptionError as e:
            bss5 = 'N/A'
        return bss5

    def get_bandwidth(self):
        """@return string bandwidth for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_BANDWIDTH)


    def get_security(self):
        """@return string security for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_SECURITY)


    def get_psk(self):
        """@return string psk for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_PSK)


    def get_frequency(self):
        """@return int frequency for AP from config file"""
        return int(self.ap_config.get(self.bss, self.CONF_FREQUENCY))

    def get_channel(self):
        """@return int channel for AP from config file"""
        return ap_spec.CHANNEL_TABLE[self.get_frequency()]


    def get_band(self):
        """@return string band for AP from config file"""
        if self.get_frequency() < 4915:
            return ap_spec.BAND_2GHZ
        else:
            return ap_spec.BAND_5GHZ


    def get_class(self):
        """@return string class for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_CLASS)


    def get_admin(self):
        """@return string admin for AP from config file"""
        return self.ap_config.get(self.bss, self.CONF_ADMIN)


    def power_off(self):
        """call rpm_client to power off AP"""
        rpm_client.set_power(self.get_wan_host(), 'OFF')


    def power_on(self):
        """call rpm_client to power on AP"""
        rpm_client.set_power(self.get_wan_host(), 'ON')

        # Hard coded timer for now to wait for the AP to come alive
        # before trying to use it.  We need scanning code
        # to scan until the AP becomes available (crosbug.com/36710).
        time.sleep(TIMEOUT)


    def __str__(self):
        """@return string description of AP"""
        ap_info = {
            'brand': self.get_brand(),
            'model': self.get_model(),
            'ssid' : self.get_ssid(),
            'bss'  : self.get_bss(),
            'hostname': self.get_wan_host(),
        }
        return ('AP Info:\n'
                '  Name:      %(brand)s %(model)s\n'
                '  SSID:      %(ssid)s\n'
                '  BSS:       %(bss)s\n'
                '  Hostname:  %(hostname)s\n' % ap_info)
