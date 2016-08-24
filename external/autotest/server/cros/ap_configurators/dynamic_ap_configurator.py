# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import binascii
import copy
import logging
import os
import pprint
import re
import time
import xmlrpclib
import json
import urllib2
import time

import ap_spec
import web_driver_core_helpers

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.ap_configurators import ap_configurator

try:
  from selenium import webdriver
except ImportError:
  raise ImportError('Could not locate the webdriver package. '
                    'Did you emerge it into your chroot?')


class DynamicAPConfigurator(web_driver_core_helpers.WebDriverCoreHelpers,
                            ap_configurator.APConfiguratorAbstract):
    """Base class for objects to configure access points using webdriver."""


    def __init__(self, ap_config):
        """Construct a DynamicAPConfigurator.

        @param ap_config: information from the configuration file
        @param set_ap_spec: APSpec object that when passed will set all
                            of the configuration options

        """
        super(DynamicAPConfigurator, self).__init__()
        rpm_frontend_server = global_config.global_config.get_config_value(
                'CROS', 'rpm_frontend_uri')
        self.rpm_client = xmlrpclib.ServerProxy(
                rpm_frontend_server, verbose=False)

        # Load the data for the config file
        self.admin_interface_url = ap_config.get_admin()
        self.class_name = ap_config.get_class()
        self._short_name = ap_config.get_model()
        self.mac_address = ap_config.get_wan_mac()
        self.host_name = ap_config.get_wan_host()
        # Get corresponding PDU from host name.
        self.pdu = re.sub('host\d+', 'rpm1', self.host_name) + '.cros'
        self.config_data = ap_config

        name_dict = {'Router name': self._short_name,
                     'Controller class': self.class_name,
                     '2.4 GHz MAC Address': ap_config.get_bss(),
                     '5 GHz MAC Address': ap_config.get_bss5(),
                     'Hostname': ap_config.get_wan_host()}

        self._name = str('%s' % pprint.pformat(name_dict))

        # Set a default band, this can be overriden by the subclasses
        self.current_band = ap_spec.BAND_2GHZ
        self._ssid = None

        # Diagnostic members
        self._command_list = []
        self._screenshot_list = []
        self._traceback = None

        self.driver_connection_established = False
        self.router_on = False
        self._configuration_success = ap_constants.CONFIG_SUCCESS
        self._webdriver_port = 9515

        self.ap_spec = None
        self.webdriver_hostname = None

    def __del__(self):
        """Cleanup webdriver connections"""
        try:
            self.driver.close()
        except:
            pass


    def __str__(self):
        """Prettier display of the object"""
        return('AP Name: %s\n'
               'BSS: %s\n'
               'SSID: %s\n'
               'Short name: %s' % (self.name, self.get_bss(),
               self._ssid, self.short_name))


    @property
    def configurator_type(self):
        """Returns the configurator type."""
        return ap_spec.CONFIGURATOR_DYNAMIC


    @property
    def ssid(self):
        """Returns the SSID."""
        return self._ssid


    def add_item_to_command_list(self, method, args, page, priority):
        """
        Adds commands to be executed against the AP web UI.

        @param method: the method to run
        @param args: the arguments for the method you want executed
        @param page: the page on the web ui where to run the method against
        @param priority: the priority of the method

        """
        self._command_list.append({'method': method,
                                   'args': copy.copy(args),
                                   'page': page,
                                   'priority': priority})


    def reset_command_list(self):
        """Resets all internal command state."""
        logging.error('Dumping command list %s', self._command_list)
        self._command_list = []
        self.destroy_driver_connection()


    def save_screenshot(self):
        """
        Stores and returns the screenshot as a base 64 encoded string.

        @returns the screenshot as a base 64 encoded string; if there was
        an error saving the screenshot None is returned.

        """
        screenshot = None
        if self.driver_connection_established:
            try:
                # driver.get_screenshot_as_base64 takes a screenshot that is
                # whatever the size of the window is.  That can be anything,
                # forcing a size that will get everything we care about.
                window_size = self.driver.get_window_size()
                self.driver.set_window_size(2000, 5000)
                screenshot = self.driver.get_screenshot_as_base64()
                self.driver.set_window_size(window_size['width'],
                                            window_size['height'])
            except Exception as e:
                # The messages differ based on the webdriver version
                logging.error('Getting the screenshot failed. %s', e)
                # TODO (krisr) this too can fail with an exception.
                self._check_for_alert_in_message(str(e),
                                                 self._handler(None))
                logging.error('Alert was handled.')
                screenshot = None
            if screenshot:
                self._screenshot_list.append(screenshot)
        return screenshot


    def get_all_screenshots(self):
        """Returns a list of screenshots."""
        return self._screenshot_list


    def clear_screenshot_list(self):
        """Clear the list of currently stored screenshots."""
        self._screenshot_list = []


    def _save_all_pages(self):
        """Iterate through AP pages, saving screenshots"""
        self.establish_driver_connection()
        if not self.driver_connection_established:
            logging.error('Unable to establish webdriver connection to '
                          'retrieve screenshots.')
            return
        for page in range(1, self.get_number_of_pages() + 1):
            self.navigate_to_page(page)
            self.save_screenshot()


    def _write_screenshots(self, filename, outputdir):
        """
        Writes screenshots to filename in outputdir

        @param filename: a string prefix for screenshot filenames
        @param outputdir: a string directory name to save screenshots

        """
        for (i, image) in enumerate(self.get_all_screenshots()):
            path = os.path.join(outputdir,
                                str('%s_%d.png' % (filename, (i + 1))))
            with open(path, 'wb') as f:
                f.write(image.decode('base64'))


    @property
    def traceback(self):
        """
        Returns the traceback of a configuration error as a string.

        Note that if configuration_success returns CONFIG_SUCCESS this will
        be none.

        """
        return self._traceback


    @traceback.setter
    def traceback(self, value):
        """
        Set the traceback.

        If the APConfigurator crashes use this to store what the traceback
        was as a string.  It can be used later to debug configurator errors.

        @param value: a string representation of the exception traceback

        """
        self._traceback = value


    def check_webdriver_ready(self, webdriver_hostname, webdriver_port):
        """Checks if webdriver binary is installed and running.

        @param webdriver_hostname: locked webdriver instance
        @param webdriver_port: port of the webdriver server

        @returns a string: the address of webdriver running on port.

        @raises TestError: Webdriver is not running.
        """
        address = webdriver_hostname + '.cros'
        url = 'http://%s:%d/session' % (address, webdriver_port)
        req = urllib2.Request(url, '{"desiredCapabilities":{}}')
        try:
            time.sleep(20)
            response = urllib2.urlopen(req)
            json_dict = json.loads(response.read())
            if json_dict['status'] == 0:
                # Connection was successful, close the session
                session_url = os.path.join(url, json_dict['sessionId'])
                req = urllib2.Request(session_url)
                req.get_method = lambda: 'DELETE'
                response = urllib2.urlopen(req)
                logging.info('Webdriver connection established to server %s',
                            address)
                return webdriver_hostname
        except:
            err = 'Could not establish connection: %s', webdriver_hostname
            raise error.TestError(err)


    @property
    def webdriver_port(self):
        """Returns the webdriver port."""
        return self._webdriver_port


    @webdriver_port.setter
    def webdriver_port(self, value):
        """
        Set the webdriver server port.

        @param value: the port number of the webdriver server

        """
        self._webdriver_port = value


    @property
    def name(self):
        """Returns a string to describe the router."""
        return self._name


    @property
    def short_name(self):
        """Returns a short string to describe the router."""
        return self._short_name


    def get_number_of_pages(self):
        """Returns the number of web pages used to configure the router.

        Note: This is used internally by apply_settings, and this method must be
              implemented by the derived class.

        Note: The derived class must implement this method.

        """
        raise NotImplementedError


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
        raise NotImplementedError


    def get_bss(self):
        """Returns the bss of the AP."""
        if self.current_band == ap_spec.BAND_2GHZ:
            return self.config_data.get_bss()
        else:
            return self.config_data.get_bss5()


    def _get_channel_popup_position(self, channel):
        """Internal method that converts a channel value to a popup position."""
        supported_bands = self.get_supported_bands()
        for band in supported_bands:
            if band['band'] == self.current_band:
                return band['channels'].index(channel)
        raise RuntimeError('The channel passed %d to the band %s is not '
                           'supported.' % (channel, band))


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
        raise NotImplementedError


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
        bands = self.get_supported_bands()
        for current_band in bands:
            if (current_band['band'] == band and
                channel in current_band['channels']):
                return True
        return False


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
        raise NotImplementedError


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        Note: The derived class must implement this method.

        @param page_number: page number of the page to load

        """
        raise NotImplementedError


    def power_cycle_router_up(self):
        """Queues the power cycle up command."""
        self.add_item_to_command_list(self._power_cycle_router_up, (), 1, 0)


    def _power_cycle_router_up(self):
        """Turns the ap off and then back on again."""
        self.rpm_client.queue_request(self.host_name, 'OFF')
        self.router_on = False
        self._power_up_router()


    def power_down_router(self):
        """Queues up the power down command."""
        self.add_item_to_command_list(self._power_down_router, (), 1, 999)


    def _power_down_router(self):
        """Turns off the power to the ap via the power strip."""
        self.check_pdu_status()
        self.rpm_client.queue_request(self.host_name, 'OFF')
        self.router_on = False


    def power_up_router(self):
        """Queues up the power up command."""
        self.add_item_to_command_list(self._power_up_router, (), 1, 0)


    def _power_up_router(self):
        """
        Turns on the power to the ap via the power strip.

        This method returns once it can navigate to a web page of the ap UI.

        """
        if self.router_on:
            return
        self.check_pdu_status()
        self.rpm_client.queue_request(self.host_name, 'ON')
        self.establish_driver_connection()
        # Depending on the response of the webserver for the AP, or lack
        # there of, the amount of time navigate_to_page and refresh take
        # is indeterminate.  Give the APs 5 minutes of real time and then
        # give up.
        timeout = time.time() + (5 * 60)
        half_way = time.time() + (2.5 * 60)
        performed_power_cycle = False
        while time.time() < timeout:
            try:
                logging.info('Attempting to load page')
                self.navigate_to_page(1)
                logging.debug('Page navigation complete')
                self.router_on = True
                return
            # Navigate to page may throw a Selemium error or its own
            # RuntimeError depending on the implementation.  Either way we are
            # bringing a router back from power off, we need to be patient.
            except:
                logging.info('Forcing a page refresh')
                self.driver.refresh()
                logging.info('Waiting for router %s to come back up.',
                             self.name)
                # Sometime the APs just don't come up right.
                if not performed_power_cycle and time.time() > half_way:
                    logging.info('Cannot connect to AP, forcing cycle')
                    self.rpm_client.queue_request(self.host_name, 'CYCLE')
                    performed_power_cycle = True
                    logging.info('Power cycle complete')
        raise RuntimeError('Unable to load admin page after powering on the '
                           'router: %s' % self.name)


    def save_page(self, page_number):
        """
        Saves the given page.

        Note: The derived class must implement this method.

        @param page_number: Page number of the page to save.

        """
        raise NotImplementedError


    def set_using_ap_spec(self, set_ap_spec, power_up=True):
        """
        Sets all configurator options.

        @param set_ap_spec: APSpec object

        """
        if power_up:
            self.power_up_router()
        if self.is_visibility_supported():
            self.set_visibility(set_ap_spec.visible)
        if (set_ap_spec.security == ap_spec.SECURITY_TYPE_WPAPSK or
            set_ap_spec.security == ap_spec.SECURITY_TYPE_WPA2PSK):
            self.set_security_wpapsk(set_ap_spec.security, set_ap_spec.password)
        else:
            self.set_security_disabled()
        self.set_band(set_ap_spec.band)
        self.set_mode(set_ap_spec.mode)
        self.set_channel(set_ap_spec.channel)

        # Update ssid
        raw_ssid = '%s_%s_ch%d_%s' % (
                self.short_name,
                ap_spec.mode_string_for_mode(set_ap_spec.mode),
                set_ap_spec.channel,
                set_ap_spec.security)
        self._ssid = raw_ssid.replace(' ', '_').replace('.', '_')[:32]
        self.set_ssid(self._ssid)
        self.ap_spec = set_ap_spec
        self.webdriver_hostname = set_ap_spec.webdriver_hostname

    def set_mode(self, mode, band=None):
        """
        Sets the mode.

        Note: The derived class must implement this method.

        @param mode: must be one of the modes listed in __init__()
        @param band: the band to select

        """
        raise NotImplementedError


    def set_radio(self, enabled=True):
        """
        Turns the radio on and off.

        Note: The derived class must implement this method.

        @param enabled: True to turn on the radio; False otherwise

        """
        raise NotImplementedError


    def set_ssid(self, ssid):
        """
        Sets the SSID of the wireless network.

        Note: The derived class must implement this method.

        @param ssid: name of the wireless network

        """
        raise NotImplementedError


    def set_channel(self, channel):
        """
        Sets the channel of the wireless network.

        Note: The derived class must implement this method.

        @param channel: integer value of the channel

        """
        raise NotImplementedError


    def set_band(self, band):
        """
        Sets the band of the wireless network.

        Currently there are only two possible values for band: 2kGHz and 5kGHz.
        Note: The derived class must implement this method.

        @param band: Constant describing the band type

        """
        raise NotImplementedError


    def set_security_disabled(self):
        """
        Disables the security of the wireless network.

        Note: The derived class must implement this method.

        """
        raise NotImplementedError


    def set_security_wep(self, key_value, authentication):
        """
        Enabled WEP security for the wireless network.

        Note: The derived class must implement this method.

        @param key_value: encryption key to use
        @param authentication: one of two supported WEP authentication types:
                               open or shared.
        """
        raise NotImplementedError


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        """Enabled WPA using a private security key for the wireless network.

        Note: The derived class must implement this method.

        @param security: Required security for AP configuration
        @param shared_key: shared encryption key to use
        @param update_interval: number of seconds to wait before updating

        """
        raise NotImplementedError

    def set_visibility(self, visible=True):
        """Set the visibility of the wireless network.

        Note: The derived class must implement this method.

        @param visible: True for visible; False otherwise

        """
        raise NotImplementedError


    def establish_driver_connection(self):
        """Makes a connection to the webdriver service."""
        if self.driver_connection_established:
            return
        # Load the Auth extension

        webdriver_hostname = self.ap_spec.webdriver_hostname
        webdriver_ready = self.check_webdriver_ready(webdriver_hostname,
                                                     self._webdriver_port)
        webdriver_server = webdriver_ready + '.cros'
        if webdriver_server is None:
            raise RuntimeError('Unable to connect to webdriver locally or '
                               'via the lab service.')
        extension_path = os.path.join(os.path.dirname(__file__),
                                      'basic_auth_extension.crx')
        f = open(extension_path, 'rb')
        base64_extensions = []
        base64_ext = (binascii.b2a_base64(f.read()).strip())
        base64_extensions.append(base64_ext)
        f.close()
        webdriver_url = ('http://%s:%d' % (webdriver_server,
                                           self._webdriver_port))
        capabilities = {'chromeOptions' : {'extensions' : base64_extensions}}
        self.driver = webdriver.Remote(webdriver_url, capabilities)
        self.driver_connection_established = True


    def destroy_driver_connection(self):
        """Breaks the connection to the webdriver service."""
        try:
            self.driver.close()
        except Exception, e:
            logging.debug('Webdriver is crashed, should be respawned %d',
                          time.time())
        finally:
            self.driver_connection_established = False


    def apply_settings(self):
        """Apply all settings to the access point.

        @param skip_success_validation: Boolean to track if method was
                                        executed successfully.

        """
        self.configuration_success = ap_constants.CONFIG_FAIL
        if len(self._command_list) == 0:
            return

        # If all we are doing is powering down the router, don't mess with
        # starting up webdriver.
        if (len(self._command_list) == 1 and
            self._command_list[0]['method'] == self._power_down_router):
            self._command_list[0]['method'](*self._command_list[0]['args'])
            self._command_list.pop()
            self.destroy_driver_connection()
            return
        self.establish_driver_connection()
        # Pull items by page and then sort
        if self.get_number_of_pages() == -1:
            self.fail(msg='Number of pages is not set.')
        page_range = range(1, self.get_number_of_pages() + 1)
        for i in page_range:
            page_commands = [x for x in self._command_list if x['page'] == i]
            sorted_page_commands = sorted(page_commands,
                                          key=lambda k: k['priority'])
            if sorted_page_commands:
                first_command = sorted_page_commands[0]['method']
                # If the first command is bringing the router up or down,
                # do that before navigating to a URL.
                if (first_command == self._power_up_router or
                    first_command == self._power_cycle_router_up or
                    first_command == self._power_down_router):
                    direction = 'up'
                    if first_command == self._power_down_router:
                        direction = 'down'
                    logging.info('Powering %s %s', direction, self.name)
                    first_command(*sorted_page_commands[0]['args'])
                    sorted_page_commands.pop(0)

                # If the router is off, no point in navigating
                if not self.router_on:
                    if len(sorted_page_commands) == 0:
                        # If all that was requested was to power off
                        # the router then abort here and do not set the
                        # configuration_success bit.  The reason is
                        # because if we failed on the configuration that
                        # failure should remain since all tests power
                        # down the AP when they are done.
                        return
                    break

                self.navigate_to_page(i)
                for command in sorted_page_commands:
                    command['method'](*command['args'])
                self.save_page(i)
        self._command_list = []
        self.configuration_success = ap_constants.CONFIG_SUCCESS
        self._traceback = None
        self.destroy_driver_connection()


    def get_association_parameters(self):
        """
        Creates an AssociationParameters from the configured AP.

        @returns AssociationParameters for the configured AP.

        """
        security_config = None
        if self.ap_spec.security in [ap_spec.SECURITY_TYPE_WPAPSK,
                                     ap_spec.SECURITY_TYPE_WPA2PSK]:
            # Not all of this is required but doing it just in case.
            security_config = xmlrpc_security_types.WPAConfig(
                    psk=self.ap_spec.password,
                    wpa_mode=xmlrpc_security_types.WPAConfig.MODE_MIXED_WPA,
                    wpa_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP,
                                 xmlrpc_security_types.WPAConfig.CIPHER_TKIP],
                    wpa2_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
        return xmlrpc_datatypes.AssociationParameters(
                ssid=self._ssid, security_config=security_config,
                discovery_timeout=45, association_timeout=30,
                configuration_timeout=30, is_hidden=not self.ap_spec.visible)


    def debug_last_failure(self, outputdir):
        """
        Write debug information for last AP_CONFIG_FAIL

        @param outputdir: a string directory path for debug files
        """
        logging.error('Traceback:\n %s', self.traceback)
        self._write_screenshots('config_failure', outputdir)
        self.clear_screenshot_list()


    def debug_full_state(self, outputdir):
        """
        Write debug information for full AP state

        @param outputdir: a string directory path for debug files
        """
        if self.configuration_success != ap_constants.PDU_FAIL:
            self._save_all_pages()
            self._write_screenshots('final_configuration', outputdir)
            self.clear_screenshot_list()
        self.reset_command_list()


    def store_config_failure(self, trace):
        """
        Store configuration failure for latter logging

        @param trace: a string traceback of config exception
        """
        self.save_screenshot()
        self._traceback = trace
