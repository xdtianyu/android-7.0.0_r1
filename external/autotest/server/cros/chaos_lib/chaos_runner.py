# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import datetime
import logging
import pprint
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.server import hosts
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros import host_lock_manager
from autotest_lib.server.cros.ap_configurators import ap_batch_locker
from autotest_lib.server.cros.network import chaos_clique_utils as utils
from autotest_lib.server.cros.network import wifi_client

# Webdriver master hostname
MASTERNAME = 'chromeos3-chaosvmmaster.cros.corp.google.com'


class ChaosRunner(object):
    """Object to run a network_WiFi_ChaosXXX test."""


    def __init__(self, test, host, spec, broken_pdus=list()):
        """Initializes and runs test.

        @param test: a string, test name.
        @param host: an Autotest host object, device under test.
        @param spec: an APSpec object.
        @param broken_pdus: list of offline PDUs.

        """
        self._test = test
        self._host = host
        self._ap_spec = spec
        self._broken_pdus = broken_pdus
        # Log server and DUT times
        dt = datetime.datetime.now()
        logging.info('Server time: %s', dt.strftime('%a %b %d %H:%M:%S %Y'))
        logging.info('DUT time: %s', self._host.run('date').stdout.strip())


    def run(self, job, batch_size=7, tries=10, capturer_hostname=None,
            conn_worker=None, work_client_hostname=None,
            disabled_sysinfo=False):
        """Executes Chaos test.

        @param job: an Autotest job object.
        @param batch_size: an integer, max number of APs to lock in one batch.
        @param tries: an integer, number of iterations to run per AP.
        @param capturer_hostname: a string or None, hostname or IP of capturer.
        @param conn_worker: ConnectionWorkerAbstract or None, to run extra
                            work after successful connection.
        @param work_client_hostname: a string or None, hostname of work client
        @param disabled_sysinfo: a bool, disable collection of logs from DUT.


        @raises TestError: Issues locking VM webdriver instance
        """

        lock_manager = host_lock_manager.HostLockManager()
        webdriver_master = hosts.SSHHost(MASTERNAME, user='chaosvmmaster')
        with host_lock_manager.HostsLockedBy(lock_manager):
            capture_host = utils.allocate_packet_capturer(
                    lock_manager, hostname=capturer_hostname)
            # Cleanup and reboot packet capturer before the test.
            utils.sanitize_client(capture_host)
            capturer = site_linux_system.LinuxSystem(capture_host, {},
                                                     'packet_capturer')

            # Run iw scan and abort if more than allowed number of APs are up.
            iw_command = iw_runner.IwRunner(capture_host)
            start_time = time.time()
            logging.info('Performing a scan with a max timeout of 30 seconds.')
            while time.time() - start_time <= ap_constants.MAX_SCAN_TIMEOUT:
                networks = iw_command.scan('wlan0')
                if networks is None:
                    if (time.time() - start_time ==
                            ap_constants.MAX_SCAN_TIMEOUT):
                        raise error.TestError(
                            'Packet capturer is not responding to scans. Check'
                            'device and re-run test')
                    continue
                elif len(networks) < ap_constants.MAX_SSID_COUNT:
                    break
                elif len(networks) >= ap_constants.MAX_SSID_COUNT:
                    raise error.TestError(
                        'Probably someone is already running a'
                        'chaos test?!')

            if conn_worker is not None:
                work_client_machine = utils.allocate_packet_capturer(
                        lock_manager, hostname=work_client_hostname)
                conn_worker.prepare_work_client(work_client_machine)

            webdriver_instance = utils.allocate_webdriver_instance(lock_manager)
            self._ap_spec._webdriver_hostname = webdriver_instance

            # If a test is cancelled or aborted the VM may be left on.  Always
            # turn of the VM to return it to a clean state.
            try:
                logging.info('Always power off VM %s', webdriver_instance)
                utils.power_off_VM(webdriver_master, webdriver_instance)
            except:
                logging.debug('VM was already off, ignoring.')

            logging.info('Starting up VM %s', webdriver_instance)
            utils.power_on_VM(webdriver_master, webdriver_instance)

            batch_locker = ap_batch_locker.ApBatchLocker(
                    lock_manager, self._ap_spec,
                    ap_test_type=ap_constants.AP_TEST_TYPE_CHAOS)

            while batch_locker.has_more_aps():
                # Work around crbug.com/358716
                utils.sanitize_client(self._host)
                healthy_dut = True

                with contextlib.closing(wifi_client.WiFiClient(
                    hosts.create_host(self._host.hostname),
                    './debug',
                    False)) as client:

                    aps = batch_locker.get_ap_batch(batch_size=batch_size)
                    if not aps:
                        logging.info('No more APs to test.')
                        break

                    # Power down all of the APs because some can get grumpy
                    # if they are configured several times and remain on.
                    # User the cartridge to down group power downs and
                    # configurations.
                    utils.power_down_aps(aps, self._broken_pdus)
                    utils.configure_aps(aps, self._ap_spec, self._broken_pdus)

                    aps = utils.filter_quarantined_and_config_failed_aps(aps,
                            batch_locker, job, self._broken_pdus)

                    for ap in aps:
                        # http://crbug.com/306687
                        if ap.ssid == None:
                            logging.error('The SSID was not set for the AP:%s',
                                          ap)

                        healthy_dut = utils.is_dut_healthy(client, ap)

                        if not healthy_dut:
                            logging.error('DUT is not healthy, rebooting.')
                            batch_locker.unlock_and_reclaim_aps()
                            break

                        networks = utils.return_available_networks(
                                ap, capturer, job, self._ap_spec)

                        if networks is None:
                            # If scan returned no networks, iw scan failed.
                            # Reboot the packet capturer device and
                            # reconfigure the capturer.
                            batch_locker.unlock_and_reclaim_ap(ap.host_name)
                            logging.error('Packet capture is not healthy, '
                                          'rebooting.')
                            capturer.host.reboot()
                            capturer = site_linux_system.LinuxSystem(
                                           capture_host, {},'packet_capturer')
                            continue
                        if networks == list():
                           # Packet capturer did not find the SSID in scan or
                           # there was a security mismatch.
                           utils.release_ap(ap, batch_locker, self._broken_pdus)
                           continue

                        assoc_params = ap.get_association_parameters()

                        if not utils.is_conn_worker_healthy(
                                conn_worker, ap, assoc_params, job):
                            utils.release_ap(
                                    ap, batch_locker, self._broken_pdus)
                            continue

                        name = ap.name
                        kernel_ver = self._host.get_kernel_ver()
                        firmware_ver = utils.get_firmware_ver(self._host)
                        if not firmware_ver:
                            firmware_ver = "Unknown"

                        debug_dict = {'+++PARSE DATA+++': '+++PARSE DATA+++',
                                      'SSID': ap._ssid,
                                      'DUT': client.wifi_mac,
                                      'AP Info': ap.name,
                                      'kernel_version': kernel_ver,
                                      'wifi_firmware_version': firmware_ver}
                        debug_string = pprint.pformat(debug_dict)

                        logging.info('Waiting %d seconds for the AP dhcp '
                                     'server', ap.dhcp_delay)
                        time.sleep(ap.dhcp_delay)

                        result = job.run_test(self._test,
                                     capturer=capturer,
                                     capturer_frequency=networks[0].frequency,
                                     capturer_ht_type=networks[0].ht,
                                     host=self._host,
                                     assoc_params=assoc_params,
                                     client=client,
                                     tries=tries,
                                     debug_info=debug_string,
                                     # Copy all logs from the system
                                     disabled_sysinfo=disabled_sysinfo,
                                     conn_worker=conn_worker,
                                     tag=ap.ssid if conn_worker is None else
                                         '%s.%s' % (conn_worker.name, ap.ssid))

                        utils.release_ap(ap, batch_locker, self._broken_pdus)

                        if conn_worker is not None:
                            conn_worker.cleanup()

                    if not healthy_dut:
                        continue

                batch_locker.unlock_aps()
            capturer.close()
            logging.info('Powering off VM %s', webdriver_instance)
            utils.power_off_VM(webdriver_master, webdriver_instance)
            lock_manager.unlock(webdriver_instance)

            if self._broken_pdus:
                logging.info('PDU is down!!!\nThe following PDUs are down:\n')
                pprint.pprint(self._broken_pdus)
