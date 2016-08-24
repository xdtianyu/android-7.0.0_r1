# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.server.cros.ap_configurators import ap_spec


class PduNotResponding(Exception):
    """PDU exception class."""
    def __init__(self, PDU):
        self.PDU = PDU
        logging.info('Caught PDU exception for %s', self.PDU)

    def __str__(self):
        return repr('Caught PDU exception for %s' % self.PDU)


class APConfiguratorAbstract(object):
    """Abstract Base class to find and control access points."""

    def __init__(self):
        super(APConfiguratorAbstract, self).__init__()
        # Some APs will turn on their beacon, but the DHCP server is not
        # running.  Each configurator can add this delay to have the test
        # wait before attempting to connect.
        self._dhcp_delay = 0


    @property
    def dhcp_delay(self):
        """Returns the DHCP delay."""
        return self._dhcp_delay


    @property
    def ssid(self):
        """Returns the SSID."""
        raise NotImplementedError('Missing subclass implementation')


    @property
    def name(self):
        """Returns a string to describe the router."""
        raise NotImplementedError('Missing subclass implementation')


    @property
    def configuration_success(self):
        """Returns configuration status as defined in ap_constants"""
        return self._configuration_success


    @configuration_success.setter
    def configuration_success(self, value):
        """
        Set the configuration status.

        @param value: the status of AP configuration.

        """
        self._configuration_success = value


    @property
    def configurator_type(self):
        """Returns the configurator type."""
        return ap_spec.CONFIGURATOR_STATIC


    @property
    def short_name(self):
        """Returns a short string to describe the router."""
        raise NotImplementedError('Missing subclass implementation')


    def check_pdu_status(self):
        """Check if the PDU is up before making any request."""
        ping_options = ping_runner.PingConfig(self.pdu, count=2,
                                              ignore_status=True,
                                              ignore_result=True)
        runner = ping_runner.PingRunner()
        logging.info('Pinging rpm %s', self.pdu)
        ping_result = runner.ping(ping_options)
        logging.info('ping result = %s', str(ping_result))
        # If all ping packets failed then mark PDU down.
        if ping_result.loss == 100:
            self.configuration_success = ap_constants.PDU_FAIL
            raise PduNotResponding(self.pdu)


    def get_supported_bands(self):
        """Returns a list of dictionaries describing the supported bands.

        Example: returned is a dictionary of band and a list of channels. The
                 band object returned must be one of those defined in the
                 __init___ of this class.

        supported_bands = [{'band' : self.band_2GHz,
                            'channels' : [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                           {'band' : ap_spec.BAND_5GHZ,
                            'channels' : [26, 40, 44, 48, 149, 153, 165]}]

        Note: The derived class must implement this method.

        @return a list of dictionaries as described above

        """
        raise NotImplementedError('Missing subclass implementation')


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

        Note: The derived class must implement this method.

        @return a list of dictionaries as described above

        """
        raise NotImplementedError('Missing subclass implementation')


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.

        """
        return True


    def is_band_and_channel_supported(self, band, channel):
        """
        Returns if a given band and channel are supported.

        @param band: the band to check if supported
        @param channel: the channel to check if supported

        @return True if combination is supported; False otherwise.

        """
        raise NotImplementedError('Missing subclass implementation')


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        Note: The derived class must implement this method.

        @param security_mode: one of the following modes:
                         self.security_disabled,
                         self.security_wep,
                         self.security_wpapsk,
                         self.security_wpa2psk

        @return True if the security mode is supported; False otherwise.

        """
        raise NotImplementedError('Missing subclass implementation')



    def set_using_ap_spec(self, set_ap_spec, power_up=True):
        """
        Sets all configurator options.
        Note: The derived class may override this method.

        @param set_ap_spec: APSpec object
        @param power_up: bool, enable power via rpm if applicable

        """
        logging.warning('%s.%s: Not Implemented',
                self.__class__.__name__,
                self.set_using_ap_spec.__name__)


    def apply_settings(self):
        """
        Apply all settings to the access point.
        Note: The derived class may override this method.

        """
        logging.warning('%s.%s: Not Implemented',
                self.__class__.__name__,
                self.apply_settings.__name__)


    def get_association_parameters(self):
        """
        Returns xmlrpc_datatypes.AssociationParameters for this AP

        Note: The derived class must implement this method.

        @return xmlrpc_datatypes.AssociationParameters

        """
        raise NotImplementedError('Missing subclass implementation')


    def debug_last_failure(self, outputdir):
        """
        Write debug information for last AP_CONFIG_FAIL

        @param outputdir: a string directory path for debug files
        """
        pass


    def debug_full_state(self, outputdir):
        """
        Write debug information for full AP state

        @param outputdir: a string directory path for debug files
        """
        pass


    def store_config_failure(self, trace):
        """
        Store configuration failure for latter logging

        @param trace: a string traceback of config exception
        """
        pass
