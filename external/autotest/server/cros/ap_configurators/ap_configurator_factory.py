# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""File containing class to build all available ap_configurators."""

import logging

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.server.cros import ap_config
from autotest_lib.server.cros.ap_configurators import ap_cartridge
from autotest_lib.server.cros.ap_configurators import ap_spec
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

CONFIG = global_config.global_config

_DEFAULT_AUTOTEST_INSTANCE = CONFIG.get_config_value('SERVER', 'hostname',
                                                     type=str)

class APConfiguratorFactory(object):
    """Class that instantiates all available APConfigurators.

    @attribute CONFIGURATOR_MAP: a dict of strings, mapping to model-specific
                                 APConfigurator objects.
    @attribute BANDS: a string, bands supported by an AP.
    @attribute MODES: a string, 802.11 modes supported by an AP.
    @attribute SECURITIES: a string, security methods supported by an AP.
    @attribute HOSTNAMES: a string, AP hostname.
    @attribute ap_list: a list of APConfigurator objects.
    @attribute ap_config: an APConfiguratorConfig object.
    """

    PREFIX='autotest_lib.server.cros.ap_configurators.'
    CONFIGURATOR_MAP = {
        'LinksysAPConfigurator':
            [PREFIX + 'linksys_ap_configurator',
                'LinksysAPConfigurator'],
        'LinksysAP15Configurator':
            [PREFIX + 'linksys_ap_15_configurator',
                'LinksysAP15Configurator'],
        'DLinkAPConfigurator':
            [PREFIX + 'dlink_ap_configurator',
                'DLinkAPConfigurator'],
        'TrendnetAPConfigurator':
            [PREFIX + 'trendnet_ap_configurator',
                'TrendnetAPConfigurator'],
        'Trendnet691grAPConfigurator':
            [PREFIX + 'trendnet691gr_ap_configurator',
                'Trendnet691grAPConfigurator'],
        'Trendnet731brAPConfigurator':
            [PREFIX + 'trendnet731br_ap_configurator',
                'Trendnet731brAPConfigurator'],
        'Trendnet432brpAPConfigurator':
            [PREFIX + 'trendnet432brp_ap_configurator',
                'Trendnet432brpAPConfigurator'],
        'Trendnet692grAPConfigurator':
            [PREFIX + 'trendnet692gr_ap_configurator',
                'Trendnet692grAPConfigurator'],
        'Trendnet654trAPConfigurator':
            [PREFIX + 'trendnet654tr_ap_configurator',
                'Trendnet654trAPConfigurator'],
        'Trendnet812druAPConfigurator':
            [PREFIX + 'trendnet812dru_ap_configurator',
                'Trendnet812druAPConfigurator'],
        'DLinkDIR655APConfigurator':
            [PREFIX + 'dlink_dir655_ap_configurator',
                'DLinkDIR655APConfigurator'],
        'DLinkDWL2100APConfigurator':
            [PREFIX + 'dlink_dwl2100_ap_configurator',
                'DLinkDWL2100APConfigurator'],
        'DLinkDIR300APConfigurator':
            [PREFIX + 'dlink_dir300_ap_configurator',
                'DLinkDIR300APConfigurator'],
        'DLinkDIR505lAPConfigurator':
            [PREFIX + 'dlink_dir505l_ap_configurator',
                'DLinkDIR505lAPConfigurator'],
        'BuffaloAPConfigurator':
            [PREFIX + 'buffalo_ap_configurator',
                'BuffaloAPConfigurator'],
        'BuffalowzrAPConfigurator':
            [PREFIX + 'buffalo_wzr_d1800h_ap_configurator',
                'BuffalowzrAPConfigurator'],
        'Buffaloag300hAPConfigurator':
            [PREFIX + 'buffaloag300h_ap_configurator',
                'Buffaloag300hAPConfigurator'],
        'AsusAPConfigurator':
            [PREFIX + 'asus_ap_configurator',
                'AsusAPConfigurator'],
        'AsusQISAPConfigurator':
            [PREFIX + 'asus_qis_ap_configurator',
                'AsusQISAPConfigurator'],
        'Asus66RAPConfigurator':
            [PREFIX + 'asus_ac66r_ap_configurator',
                'Asus66RAPConfigurator'],
        'Netgear3700APConfigurator':
            [PREFIX + 'netgear3700_ap_configurator',
                'Netgear3700APConfigurator'],
        'Netgear3400APConfigurator':
            [PREFIX + 'netgear3400_ap_configurator',
                'Netgear3400APConfigurator'],
        'NetgearR6200APConfigurator':
            [PREFIX + 'netgearR6200_ap_configurator',
                'NetgearR6200APConfigurator'],
        'Netgear1000APConfigurator':
            [PREFIX + 'netgear1000_ap_configurator',
                'Netgear1000APConfigurator'],
        'Netgear2000APConfigurator':
            [PREFIX + 'netgear2000_ap_configurator',
                'Netgear2000APConfigurator'],
        'Netgear4300APConfigurator':
            [PREFIX + 'netgear4300_ap_configurator',
                'Netgear4300APConfigurator'],
        'Netgear4500APConfigurator':
            [PREFIX + 'netgear4500_ap_configurator',
                'Netgear4500APConfigurator'],
        'LinksyseDualBandAPConfigurator':
            [PREFIX + 'linksyse_dual_band_configurator',
                'LinksyseDualBandAPConfigurator'],
        'Linksyse2000APConfigurator':
            [PREFIX + 'linksyse2000_ap_configurator',
                'Linksyse2000APConfigurator'],
        'LinksyseWRT320APConfigurator':
            [PREFIX + 'linksyswrt320_ap_configurator',
                'LinksysWRT320APConfigurator'],
        'Linksyse1500APConfigurator':
            [PREFIX + 'linksyse1500_ap_configurator',
                'Linksyse1500APConfigurator'],
        'LinksysWRT54GS2APConfigurator':
            [PREFIX + 'linksyswrt54gs2_ap_configurator',
                'LinksysWRT54GS2APConfigurator'],
        'LinksysWRT600APConfigurator':
            [PREFIX + 'linksyswrt600_ap_configurator',
                'LinksysWRT600APConfigurator'],
        'LinksysM10APConfigurator':
            [PREFIX + 'linksysm10_ap_configurator',
                'LinksysM10APConfigurator'],
        'LinksysWRT54GLAPConfigurator':
            [PREFIX + 'linksyswrt54gl_ap_configurator',
                'LinksysWRT54GLAPConfigurator'],
        'LinksysWRT610NAPConfigurator':
            [PREFIX + 'linksyswrt610n_ap_configurator',
                'LinksysWRT610NAPConfigurator'],
        'LinksysWRT120NAPConfigurator':
            [PREFIX + 'linksyswrt120n_ap_configurator',
                'LinksysWRT120NAPConfigurator'],
        'LevelOneAPConfigurator':
            [PREFIX + 'levelone_ap_configurator',
                'LevelOneAPConfigurator'],
        'NetgearDualBandAPConfigurator':
            [PREFIX + 'netgear_WNDR_dual_band_configurator',
                'NetgearDualBandAPConfigurator'],
        'BelkinAPConfigurator':
            [PREFIX + 'belkin_ap_configurator',
                'BelkinAPConfigurator'],
        'BelkinF5D7234APConfigurator':
            [PREFIX + 'belkinF5D7234_ap_configurator',
                'BelkinF5D7234APConfigurator'],
        'BelkinF5D8236APConfigurator':
            [PREFIX + 'belkinF5D8236_ap_configurator',
                'BelkinF5D8236APConfigurator'],
        'BelkinF6D4230APConfigurator':
            [PREFIX + 'belkinF6D4230_ap_configurator',
                'BelkinF6D4230APConfigurator'],
        'BelkinF7DAPConfigurator':
            [PREFIX + 'belkinF7D_ap_configurator',
                'BelkinF7DAPConfigurator'],
        'BelkinF9K1002v4APConfigurator':
            [PREFIX + 'belkinF9k1002v4_ap_configurator',
                'BelkinF9K1002v4APConfigurator'],
        'BelkinF7D1301APConfigurator':
            [PREFIX + 'belkinF7D1301_ap_configurator',
                'BelkinF7D1301APConfigurator'],
        'BelkinF9KAPConfigurator':
            [PREFIX + 'belkinF9K_ap_configurator',
                'BelkinF9KAPConfigurator'],
        'BelkinF9K1001APConfigurator':
            [PREFIX + 'belkinF9K1001_ap_configurator',
                'BelkinF9K1001APConfigurator'],
        'BelkinF9K1102APConfigurator':
            [PREFIX + 'belkinF9K1102_ap_configurator',
                'BelkinF9K1102APConfigurator'],
        'BelkinF9K1103APConfigurator':
            [PREFIX + 'belkinF9K1103_ap_configurator',
                'BelkinF9K1103APConfigurator'],
        'BelkinF9K1105APConfigurator':
            [PREFIX + 'belkinF9K1105_ap_configurator',
                'BelkinF9K1105APConfigurator'],
        'BelkinF7D5301APConfigurator':
            [PREFIX + 'belkinF7D5301_ap_configurator',
                'BelkinF7D5301APConfigurator'],
        'BelkinWRTRAPConfigurator':
            [PREFIX + 'belkinWRTR_ap_configurator',
                'BelkinWRTRAPConfigurator'],
        'MediaLinkAPConfigurator':
            [PREFIX + 'medialink_ap_configurator',
                'MediaLinkAPConfigurator'],
        'NetgearSingleBandAPConfigurator':
            [PREFIX + 'netgear_single_band_configurator',
                'NetgearSingleBandAPConfigurator'],
        'DLinkwbr1310APConfigurator':
            [PREFIX + 'dlinkwbr1310_ap_configurator',
                'DLinkwbr1310APConfigurator'],
        'Linksyse2100APConfigurator':
            [PREFIX + 'linksyse2100_ap_configurator',
                'Linksyse2100APConfigurator'],
        'LinksyseSingleBandAPConfigurator':
            [PREFIX + 'linksyse_single_band_configurator',
                'LinksyseSingleBandAPConfigurator'],
        'Linksyse2500APConfigurator':
            [PREFIX + 'linksyse2500_ap_configurator',
                'Linksyse2500APConfigurator'],
        'WesternDigitalN600APConfigurator':
            [PREFIX + 'westerndigitaln600_ap_configurator',
                'WesternDigitalN600APConfigurator'],
        'Linksyse1000APConfigurator':
            [PREFIX + 'linksyse1000_ap_configurator',
                'Linksyse1000APConfigurator'],
        'LinksysWRT160APConfigurator':
            [PREFIX + 'linksyswrt160_ap_configurator',
                'LinksysWRT160APConfigurator'],
        'Keeboxw150nrAPConfigurator':
            [PREFIX + 'keeboxw150nr_ap_configurator',
                'Keeboxw150nrAPConfigurator'],
        'EdimaxAPConfigurator':
            [PREFIX + 'edimax_ap_configurator',
                'EdimaxAPConfigurator'],
        'Edimax6475ndAPConfigurator':
            [PREFIX + 'edimax6475nd_ap_configurator',
                'Edimax6475ndAPConfigurator'],
        'Edimax6428nsAPConfigurator':
            [PREFIX + 'edimax6428ns_ap_configurator',
                'Edimax6428nsAPConfigurator'],
        'StaticAPConfigurator':
            [PREFIX + 'static_ap_configurator',
                'StaticAPConfigurator'],
    }

    BANDS = 'bands'
    MODES = 'modes'
    SECURITIES = 'securities'
    HOSTNAMES = 'hostnames'


    def __init__(self, ap_test_type, spec=None):
        webdriver_ready = False
        self.ap_list = []
        self.test_type = ap_test_type
        for ap in ap_config.get_ap_list(ap_test_type):
            module_name, configurator_class = \
                    self.CONFIGURATOR_MAP[ap.get_class()]
            module = __import__(module_name, fromlist=configurator_class)
            configurator = module.__dict__[configurator_class]
            self.ap_list.append(configurator(ap_config=ap))


    def _get_aps_by_visibility(self, visible=True):
        """Returns all configurators that support setting visibility.

        @param visibility = True if SSID should be visible; False otherwise.

        @returns aps: a set of APConfigurators"""
        if visible:
            return set(self.ap_list)

        return set(filter(lambda ap: ap.is_visibility_supported(),
                          self.ap_list))


    def _get_aps_by_mode(self, band, mode):
        """Returns all configurators that support a given 802.11 mode.

        @param band: an 802.11 band.
        @param mode: an 802.11 modes.

        @returns aps: a set of APConfigurators.
        """
        if not mode:
            return set(self.ap_list)

        aps = []
        for ap in self.ap_list:
            modes = ap.get_supported_modes()
            for d in modes:
                if d['band'] == band and mode in d['modes']:
                    aps.append(ap)
        return set(aps)


    def _get_aps_by_security(self, security):
        """Returns all configurators that support a given security mode.

        @param security: the security type

        @returns aps: a set of APConfigurators.
        """

        if not security:
            return set(self.ap_list)

        aps = []
        for ap in self.ap_list:
            if ap.is_security_mode_supported(security):
                aps.append(ap)
        return set(aps)


    def _get_aps_by_band(self, band, channel=None):
        """Returns all APs that support a given band.

        @param band: the band desired.

        @returns aps: a set of APConfigurators.
        """
        if not band:
            return set(self.ap_list)

        aps = []
        for ap in self.ap_list:
            bands_and_channels = ap.get_supported_bands()
            for d in bands_and_channels:
                if channel:
                    if d['band'] == band and channel in d['channels']:
                        aps.append(ap)
                elif d['band'] == band:
                    aps.append(ap)
        return set(aps)


    def get_aps_by_hostnames(self, hostnames, ap_list=None):
        """Returns specific APs by host name.

        @param hostnames: a list of strings, AP's wan_hostname defined in the AP
                          configuration file.
        @param ap_list: a list of APConfigurator objects.

        @return a list of APConfigurators.
        """
        if ap_list == None:
            ap_list = self.ap_list

        aps = []
        for ap in ap_list:
            if ap.host_name in hostnames:
                logging.info('Found AP by hostname %s', ap.host_name)
                aps.append(ap)

        return aps


    def _get_aps_by_configurator_type(self, configurator_type, ap_list):
        """Returns APs that match the given configurator type.

        @param configurator_type: the type of configurtor to return.

        @return a list of APConfigurators.
        """
        aps = []
        for ap in ap_list:
            if ap.configurator_type == configurator_type:
                aps.append(ap)

        return aps


    def _get_aps_by_lab_location(self, want_chamber_aps, ap_list):
        """Returns APs that are inside or outside of the chaos/clique lab.

        @param want_chamber_aps: True to select only APs in the chaos/clique
        chamber. False to select APs outside of the chaos/clique chamber.

        @return a list of APConfigurators
        """
        aps = []
        afe = frontend_wrappers.RetryingAFE(server=_DEFAULT_AUTOTEST_INSTANCE,
                                            timeout_min=10,
                                            delay_sec=5)
        if self.test_type == ap_constants.AP_TEST_TYPE_CHAOS:
            ap_label = 'chaos_ap'
            lab_label = 'chaos_chamber'
        else:
            ap_label = 'clique_ap'
            lab_label = 'clique_chamber'
        all_aps = set(afe.get_hostnames(label=ap_label))
        chamber_devices = set(afe.get_hostnames(label=lab_label))
        chamber_aps = all_aps.intersection(chamber_devices)
        for ap in ap_list:
            if want_chamber_aps and ap.host_name in chamber_aps:
                aps.append(ap)

            if not want_chamber_aps and ap.host_name not in chamber_aps:
                aps.append(ap)

        return aps


    def get_ap_configurators_by_spec(self, spec=None, pre_configure=False):
        """Returns available configurators meeting spec.

        @param spec: a validated ap_spec object
        @param pre_configure: boolean, True to set all of the configuration
                              options for the APConfigurator object using the
                              given ap_spec; False otherwise.  An ap_spec must
                              be passed for this to have any effect.
        @returns aps: a list of APConfigurator objects
        """
        if not spec:
            return self.ap_list

        # APSpec matching is exact.  With the exception of lab location, even
        # if a hostname is passed the capabilities of a given configurator
        # much match everything in the APSpec.  This helps to prevent failures
        # during the pre-scan phase.
        aps = self._get_aps_by_band(spec.band, channel=spec.channel)
        aps &= self._get_aps_by_mode(spec.band, spec.mode)
        aps &= self._get_aps_by_security(spec.security)
        aps &= self._get_aps_by_visibility(spec.visible)
        matching_aps = list(aps)
        # If APs hostnames are provided, assume the tester knows the location
        # of the AP and skip AFE calls.
        if spec.hostnames is None:
            matching_aps = self._get_aps_by_lab_location(spec.lab_ap,
                                                         matching_aps)

        if spec.configurator_type != ap_spec.CONFIGURATOR_ANY:
            matching_aps = self._get_aps_by_configurator_type(
                           spec.configurator_type, matching_aps)
        if spec.hostnames is not None:
            matching_aps = self.get_aps_by_hostnames(spec.hostnames,
                                                     ap_list=matching_aps)
        if pre_configure:
            for ap in matching_aps:
                ap.set_using_ap_spec(spec)
        return matching_aps


    def turn_off_all_routers(self):
        """Powers down all of the routers."""
        ap_power_cartridge = ap_cartridge.APCartridge()
        for ap in self.ap_list:
            ap.power_down_router()
            ap_power_cartridge.push_configurator(ap)
        ap_power_cartridge.run_configurators()
