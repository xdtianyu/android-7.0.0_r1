# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import avahi_utils
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import netblock
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.client.common_lib.cros.tendo import peerd_config
from autotest_lib.client.common_lib.cros.tendo import buffet_config
from autotest_lib.client.common_lib.cros.tendo import privet_helper
from autotest_lib.server import site_linux_router
from autotest_lib.server import test
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_client


PASSPHRASE = 'chromeos'

PRIVET_AP_STARTUP_TIMEOUT_SECONDS = 30
PRIVET_MDNS_RECORD_TIMEOUT_SECONDS = 10
PRIVET_CONNECT_TIMEOUT_SECONDS = 30

POLLING_PERIOD = 0.5


class buffet_PrivetSetupFlow(test.test):
    """This test validates the privet pairing/authentication/setup flow."""
    version = 1

    def warmup(self, host, router_hostname=None):
        self._router = None
        self._shill_xmlrpc_proxy = None
        config = buffet_config.BuffetConfig(
                log_verbosity=3,
                enable_ping=True,
                disable_pairing_security=True,
                device_whitelist='any',
                options={'wifi_bootstrap_mode': 'automatic'})
        config.restart_with_config(host=host)
        self._router = site_linux_router.build_router_proxy(
                test_name=self.__class__.__name__,
                client_hostname=host.hostname,
                router_addr=router_hostname,
                enable_avahi=True)
        self._shill_xmlrpc_proxy = wifi_client.get_xmlrpc_proxy(host)
        # Cleans up profiles, wifi credentials, sandboxes our new credentials.
        self._shill_xmlrpc_proxy.init_test_network_state()
        peerd_config.PeerdConfig(verbosity_level=3).restart_with_config(
                host=host)


    def cleanup(self, host):
        if self._shill_xmlrpc_proxy is not None:
            self._shill_xmlrpc_proxy.clean_profiles()
        if self._router is not None:
            self._router.close()
        buffet_config.naive_restart(host=host)


    def run_once(self, host):
        helper = privet_helper.PrivetHelper(host=host)
        logging.info('Looking for privet bootstrapping network from DUT.')
        scan_interface = self._router.get_wlanif(2437, 'managed')
        self._router.host.run('%s link set %s up' %
                              (self._router.cmd_ip, scan_interface))
        start_time = time.time()
        privet_bss = None
        while time.time() - start_time < PRIVET_AP_STARTUP_TIMEOUT_SECONDS:
            bss_list = self._router.iw_runner.scan(scan_interface)
            for bss in bss_list or []:
                if helper.is_softap_ssid(bss.ssid):
                    privet_bss = bss
        if privet_bss is None:
            raise error.TestFail('Device did not start soft AP in time.')
        self._router.release_interface(scan_interface)

        # Get the netblock of the interface running the AP.
        dut_iw_runner = iw_runner.IwRunner(remote_host=host)
        devs = dut_iw_runner.list_interfaces(desired_if_type='AP')
        if not devs:
            raise error.TestFail('No AP devices on DUT?')
        ap_interface = interface.Interface(devs[0].if_name, host=host)
        ap_netblock = netblock.from_addr(ap_interface.ipv4_address_and_prefix)

        # Set up an AP on the router in the 5Ghz range with WPA2 security.
        wpa_config = xmlrpc_security_types.WPAConfig(
                psk=PASSPHRASE,
                wpa_mode=xmlrpc_security_types.WPAConfig.MODE_PURE_WPA2,
                wpa2_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
        router_conf = hostap_config.HostapConfig(
                frequency=5240, security_config=wpa_config,
                mode=hostap_config.HostapConfig.MODE_11N_PURE)
        self._router.hostap_configure(router_conf)

        # Connect the other interface on the router to the AP on the client
        # at a hardcoded IP address.
        self._router.configure_managed_station(
                privet_bss.ssid, privet_bss.frequency,
                ap_netblock.get_addr_in_block(200))
        station_interface = self._router.get_station_interface(instance=0)
        logging.debug('Set up station on %s', station_interface)
        self._router.ping(ping_runner.PingConfig(ap_netblock.addr, count=3))

        logging.info('Looking for privet webserver in mDNS records.')
        start_time = time.time()
        while time.time() - start_time < PRIVET_MDNS_RECORD_TIMEOUT_SECONDS:
            all_records = avahi_utils.avahi_browse(host=self._router.host)
            records = [record for record in all_records
                       if (record.interface == station_interface and
                           record.record_type == '_privet._tcp')]
            if records:
                break
            time.sleep(POLLING_PERIOD)
        if not records:
            raise error.TestFail('Did not find privet mDNS records in time.')
        if len(records) > 1:
            raise error.TestFail('Should not see multiple privet records.')
        privet_record = records[0]
        # TODO(wiley) pull the HTTPs port number out of the /info API.
        helper = privet_helper.PrivetdHelper(
                host=self._router.host,
                hostname=privet_record.address,
                http_port=int(privet_record.port))
        helper.ping_server()

        # Now configure the client with WiFi credentials.
        auth_token = helper.privet_auth()
        ssid = self._router.get_ssid()
        data = helper.setup_add_wifi_credentials(ssid, PASSPHRASE)
        helper.setup_start(data, auth_token)

        logging.info('Waiting for DUT to connect to router network.')
        start_time = time.time()
        # Wait for the DUT to take down the AP.
        while time.time() - start_time < PRIVET_CONNECT_TIMEOUT_SECONDS:
            if not dut_iw_runner.list_interfaces(desired_if_type='AP'):
                break
            time.sleep(POLLING_PERIOD)
        else:
            raise error.TestFail('Timeout waiting for DUT to take down AP.')

        # But we should be able to ping the client from the router's AP.
        while time.time() - start_time < PRIVET_CONNECT_TIMEOUT_SECONDS:
            if dut_iw_runner.list_interfaces(desired_if_type='managed'):
                break
            time.sleep(POLLING_PERIOD)
        else:
            raise error.TestFail('Timeout waiting for DUT managerd interface.')

        while time.time() - start_time < PRIVET_CONNECT_TIMEOUT_SECONDS:
            devs = dut_iw_runner.list_interfaces(desired_if_type='managed')
            if devs:
                managed_interface = interface.Interface(devs[0].if_name,
                                                        host=host)
                # Check if we have an IP yet.
                if managed_interface.ipv4_address_and_prefix:
                    break
            time.sleep(POLLING_PERIOD)
        else:
            raise error.TestFail('Timeout waiting for DUT managerd interface.')

        managed_netblock = netblock.from_addr(
                managed_interface.ipv4_address_and_prefix)
        while time.time() - start_time < PRIVET_CONNECT_TIMEOUT_SECONDS:
            PING_COUNT = 3
            result = self._router.ping(
                    ping_runner.PingConfig(managed_netblock.addr,
                                           ignore_result=True,
                                           count=PING_COUNT))
            if result.received == PING_COUNT:
                break
            time.sleep(POLLING_PERIOD)
        else:
            raise error.TestFail('Timeout before ping was successful.')

        # And buffet should think it is online as well.
        helper = privet_helper.PrivetdHelper(
                host=host, hostname=managed_netblock.addr,
                http_port=int(privet_record.port))
        helper.ping_server()
        if not helper.wifi_setup_was_successful(ssid, auth_token):
            raise error.TestFail('Device claims to be offline, but is online.')
