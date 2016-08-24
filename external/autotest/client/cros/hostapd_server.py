# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import site_eap_certs

class HostapdServer(object):
    """Hostapd server instance wrapped in a context manager.

    Simple interface to starting and controlling a hsotapd instance.
    This can be combined with a virtual-ethernet setup to test 802.1x
    on a wired interface.

    Example usage:
        with hostapd_server.HostapdServer(interface='veth_master') as hostapd:
            hostapd.send_eap_packets()

    """
    CONFIG_TEMPLATE = """
interface=%(interface)s
driver=%(driver)s
logger_syslog=-1
logger_syslog_level=2
logger_stdout=-1
logger_stdout_level=2
dump_file=%(config_directory)s/hostapd.dump
ctrl_interface=%(config_directory)s/%(control_directory)s
ieee8021x=1
eapol_key_index_workaround=0
eap_server=1
eap_user_file=%(config_directory)s/%(user_file)s
ca_cert=%(config_directory)s/%(ca_cert)s
server_cert=%(config_directory)s/%(server_cert)s
private_key=%(config_directory)s/%(server_key)s
use_pae_group_addr=1
eap_reauth_period=10
"""
    CA_CERTIFICATE_FILE = 'ca.crt'
    CONFIG_FILE = 'hostapd.conf'
    CONTROL_DIRECTORY = 'hostapd.ctl'
    EAP_PASSWORD = 'password'
    EAP_PHASE2 = 'MSCHAPV2'
    EAP_TYPE = 'PEAP'
    EAP_USERNAME = 'test'
    HOSTAPD_EXECUTABLE = 'hostapd'
    HOSTAPD_CLIENT_EXECUTABLE = 'hostapd_cli'
    SERVER_CERTIFICATE_FILE = 'server.crt'
    SERVER_PRIVATE_KEY_FILE = 'server.key'
    USER_AUTHENTICATION_TEMPLATE = """* %(type)s
"%(username)s"\t%(phase2)s\t"%(password)s"\t[2]
"""
    USER_FILE = 'hostapd.eap_user'
    # This is the default group MAC address to which EAP challenges
    # are sent, absent any prior knowledge of a specific client on
    # the link.
    PAE_NEAREST_ADDRESS = '01:80:c2:00:00:03'

    def __init__(self,
                 interface=None,
                 driver='wired',
                 config_directory='/tmp/hostapd-test'):
        super(HostapdServer, self).__init__()
        self._interface = interface
        self._config_directory = config_directory
        self._control_directory = '%s/%s' % (self._config_directory,
                                             self.CONTROL_DIRECTORY)
        self._driver = driver
        self._process = None


    def __enter__(self):
        self.start()
        return self


    def __exit__(self, exception, value, traceback):
        self.stop()


    def write_config(self):
        """Write out a hostapd configuration file-set based on the caller
        supplied parameters.

        @return the file name of the top-level configuration file written.

        """
        if not os.path.exists(self._config_directory):
            os.mkdir(self._config_directory)
        config_params = {
            'ca_cert': self.CA_CERTIFICATE_FILE,
            'config_directory' : self._config_directory,
            'control_directory': self.CONTROL_DIRECTORY,
            'driver': self._driver,
            'interface': self._interface,
            'server_cert': self.SERVER_CERTIFICATE_FILE,
            'server_key': self.SERVER_PRIVATE_KEY_FILE,
            'user_file': self.USER_FILE
        }
        authentication_params = {
            'password': self.EAP_PASSWORD,
            'phase2': self.EAP_PHASE2,
            'username': self.EAP_USERNAME,
            'type': self.EAP_TYPE
        }
        for filename, contents in (
                ( self.CA_CERTIFICATE_FILE, site_eap_certs.ca_cert_1 ),
                ( self.CONFIG_FILE, self.CONFIG_TEMPLATE % config_params),
                ( self.SERVER_CERTIFICATE_FILE, site_eap_certs.server_cert_1 ),
                ( self.SERVER_PRIVATE_KEY_FILE,
                  site_eap_certs.server_private_key_1 ),
                ( self.USER_FILE,
                  self.USER_AUTHENTICATION_TEMPLATE % authentication_params )):
            config_file = '%s/%s' % (self._config_directory, filename)
            with open(config_file, 'w') as f:
                f.write(contents)
        return '%s/%s' % (self._config_directory, self.CONFIG_FILE)


    def start(self):
        """Start the hostap server."""
        config_file = self.write_config()
        self._process = subprocess.Popen(
                 [self.HOSTAPD_EXECUTABLE, '-dd', config_file])


    def stop(self):
        """Stop the hostapd server."""
        if self._process:
            self._process.terminate()
            self._process.wait()
            self._process = None


    def running(self):
        """Tests whether the hostapd process is still running.

        @return True if the hostapd process is still running, False otherwise.

        """
        if not self._process:
            return False

        if self._process.poll() != None:
            # We have essentially reaped the proces, and it is no more.
            self._process = None
            return False

        return True


    def send_eap_packets(self):
        """Start sending EAP packets to the nearest neighbor."""
        self.send_command('new_sta %s' % self.PAE_NEAREST_ADDRESS)


    def get_client_mib(self, client_mac_address):
        """Get a dict representing the MIB properties for |client_mac_address|.

        @param client_mac_address string MAC address of the client.
        @return dict containing mib properties.

        """
        # Expected output of "hostapd cli <client_mac_address>":
        #
        #     Selected interface 'veth_master'
        #     b6:f1:39:1d:ad:10
        #     dot1xPaePortNumber=0
        #     dot1xPaePortProtocolVersion=2
        #     [...]
        result = self.send_command('sta %s' % client_mac_address)
        client_mib = {}
        found_client = False
        for line in result.splitlines():
            if found_client:
                parts = line.split('=', 1)
                if len(parts) == 2:
                    client_mib[parts[0]] = parts[1]
            elif line == client_mac_address:
                found_client = True
        return client_mib


    def send_command(self, command):
        """Send a command to the hostapd instance.

        @param command string containing the command to run on hostapd.
        @return string output of the command.

        """
        return utils.system_output('%s -p %s %s' %
                                   (self.HOSTAPD_CLIENT_EXECUTABLE,
                                    self._control_directory, command))


    def client_has_authenticated(self, client_mac_address):
        """Return whether |client_mac_address| has successfully authenticated.

        @param client_mac_address string MAC address of the client.
        @return True if client is authenticated.

        """
        mib = self.get_client_mib(client_mac_address)
        return mib.get('dot1xAuthAuthSuccessesWhileAuthenticating', '') == '1'


    def client_has_logged_off(self, client_mac_address):
        """Return whether |client_mac_address| has logged-off.

        @param client_mac_address string MAC address of the client.
        @return True if client has logged off.

        """
        mib = self.get_client_mib(client_mac_address)
        return mib.get('dot1xAuthAuthEapLogoffWhileAuthenticated', '') == '1'
