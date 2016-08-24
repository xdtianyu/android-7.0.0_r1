# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import os
import pprint
import time
import re

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.server import hosts
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros import host_lock_manager
from autotest_lib.server.cros.ap_configurators import ap_batch_locker
from autotest_lib.server.cros.network import chaos_clique_utils as utils
from autotest_lib.server.cros.network import connection_worker
from autotest_lib.server.cros.clique_lib import clique_dut_locker
from autotest_lib.server.cros.clique_lib import clique_dut_log_collector
from autotest_lib.server.cros.clique_lib import clique_dut_updater


class CliqueRunner(object):
    """Object to run a network_WiFi_CliqueXXX test."""

    def __init__(self, test, dut_pool_spec, ap_specs):
        """Initializes and runs test.

        @param test: a string, test name.
        @param dut_pool_spec: a list of pool sets. Each set contains a list of
                              board: <board_name> labels to chose the required
                              DUT's.
        @param ap_specs: a list of APSpec objects corresponding to the APs
                         needed for the test.
        """
        self._test = test
        self._ap_specs = ap_specs
        self._dut_pool_spec = dut_pool_spec
        self._dut_pool = []
        # Log server and DUT times
        dt = datetime.datetime.now()
        logging.info('Server time: %s', dt.strftime('%a %b %d %H:%M:%S %Y'))

    def _allocate_dut_pool(self, dut_locker):
        """Allocate the required DUT's from the spec for the test.
        The DUT objects are stored in a list of sets in |_dut_pool| attribute.

        @param dut_locker: DUTBatchLocker object used to allocate the DUTs
                           for the test pool.

        @return: Returns a list of DUTObjects allocated.
        """
        self._dut_pool  = dut_locker.get_dut_pool()
        # Flatten the list of DUT objects into a single list.
        dut_objects = sum(self._dut_pool, [])
        return dut_objects

    @staticmethod
    def _update_dut_pool(dut_objects, release_version):
        """Allocate the required DUT's from the spec for the test.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        @param release_version: A chromeOS release version.

        @return: True if all the DUT's successfully upgraded, False otherwise.
        """
        dut_updater = clique_dut_updater.CliqueDUTUpdater()
        return dut_updater.update_dut_pool(dut_objects, release_version)

    @staticmethod
    def _collect_dut_pool_logs(dut_objects, job):
        """Allocate the required DUT's from the spec for the test.
        The DUT objects are stored in a list of sets in |_dut_pool| attribute.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        @param job: Autotest job object to be used for log collection.

        @return: Returns a list of DUTObjects allocated.
        """
        log_collector = clique_dut_log_collector.CliqueDUTLogCollector()
        log_collector.collect_logs(dut_objects, job)

    @staticmethod
    def _are_all_duts_healthy(dut_objects, ap):
        """Returns if iw scan is not working on any of the DUTs.

        Sometimes iw scan will die, especially on the Atheros chips.
        This works around that bug.  See crbug.com/358716.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        @param ap: ap_configurator object

        @returns True if all the DUTs are healthy, False otherwise.
        """
        healthy = True
        for dut in dut_objects:
            if not utils.is_dut_healthy(dut.wifi_client, ap):
                logging.error('DUT %s not healthy.', dut.host.hostname)
                healthy = False
        return healthy

    @staticmethod
    def _sanitize_all_duts(dut_objects):
        """Clean up logs and reboot all the DUTs.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        """
        for dut in dut_objects:
            utils.sanitize_client(dut.host)

    @staticmethod
    def _sync_time_on_all_duts(dut_objects):
        """Syncs time on all the DUTs in the pool to the time on the host.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        """
        # Let's get the timestamp once on the host and then set it on all
        # the duts.
        epoch_seconds = time.time()
        logging.info('Syncing epoch time on DUTs to %d seconds.', epoch_seconds)
        for dut in dut_objects:
            dut.wifi_client.shill.sync_time_to(epoch_seconds)

    @staticmethod
    def _get_debug_string(dut_objects, aps):
        """Gets the debug info for all the DUT's and APs in the pool.

        This is printed in the logs at the end of each test scenario for
        debugging.
        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        @param aps: A list of APConfigurator for all APs allocated for
                    the test.

        @returns a string with the list of information for each DUT and AP
                 in the pool.
        """
        debug_string = ""
        for dut in dut_objects:
            kernel_ver = dut.host.get_kernel_ver()
            firmware_ver = utils.get_firmware_ver(dut.host)
            if not firmware_ver:
                firmware_ver = "Unknown"
            debug_dict = {'host_name': dut.host.hostname,
                          'kernel_versions': kernel_ver,
                          'wifi_firmware_versions': firmware_ver}
            debug_string += pprint.pformat(debug_dict)
        for ap in aps:
            debug_string += pprint.pformat({'ap_name': ap.name})
        return debug_string

    @staticmethod
    def _are_all_conn_workers_healthy(workers, aps, assoc_params_list, job):
        """Returns if all the connection workers are working properly.

        From time to time the connection worker will fail to establish a
        connection to the APs.

        @param workers: a list of conn_worker objects.
        @param aps: a list of an ap_configurator objects.
        @param assoc_params_list: list of connection association parameters.
        @param job: the Autotest job object.

        @returns True if all the workers are healthy, False otherwise.
        """
        healthy = True
        for worker, ap, assoc_params in zip(workers, aps, assoc_params_list):
            if not utils.is_conn_worker_healthy(worker, ap, assoc_params, job):
                logging.error('Connection worker %s not healthy.',
                              worker.host.hostname)
                healthy = False
        return healthy

    def _cleanup(self, dut_objects, dut_locker, ap_locker, capturer,
                 conn_workers):
        """Cleans up after the test is complete.

        @param dut_objects: A list of DUTObjects for all DUTs allocated for the
                            test.
        @param dut_locker: DUTBatchLocker object used to allocate the DUTs
                           for the test pool.
        @param ap_locker: the AP batch locker object.
        @param capturer: a packet capture device.
        @param conn_workers: a list of conn_worker objects.
        """
        self._collect_dut_pool_logs(dut_objects)
        for worker in conn_workers:
            if worker: worker.cleanup()
        capturer.close()
        ap_locker.unlock_aps()
        dut_locker.unlock_and_close_duts()

    def run(self, job, tries=10, capturer_hostname=None,
            conn_worker_hostnames=[], release_version="",
            disabled_sysinfo=False):
        """Executes Clique test.

        @param job: an Autotest job object.
        @param tries: an integer, number of iterations to run per AP.
        @param capturer_hostname: a string or None, hostname or IP of capturer.
        @param conn_worker_hostnames: a list of string, hostname of
                                      connection workers.
        @param release_version: the DUT cros image version to use for testing.
        @param disabled_sysinfo: a bool, disable collection of logs from DUT.
        """
        lock_manager = host_lock_manager.HostLockManager()
        with host_lock_manager.HostsLockedBy(lock_manager):
            dut_locker = clique_dut_locker.CliqueDUTBatchLocker(
                    lock_manager, self._dut_pool_spec)
            dut_objects = self._allocate_dut_pool(dut_locker)
            if not dut_objects:
                raise error.TestError('No DUTs allocated for test.')
            update_status = self._update_dut_pool(dut_objects, release_version)
            if not update_status:
                raise error.TestError('DUT pool update failed. Bailing!')

            capture_host = utils.allocate_packet_capturer(
                    lock_manager, hostname=capturer_hostname)
            capturer = site_linux_system.LinuxSystem(
                    capture_host, {}, 'packet_capturer')

            conn_workers = []
            for hostname in conn_worker_hostnames:
                conn_worker_host = utils.allocate_packet_capturer(
                        lock_manager, hostname=hostname)
                # Let's create generic connection workers and make them connect
                # to the corresponding AP. The DUT role will recast each of
                # these connection workers based on the role we want them to
                # perform.
                conn_worker = connection_worker.ConnectionWorker()
                conn_worker.prepare_work_client(conn_worker_host)
                conn_workers.append(conn_worker)

            aps = []
            for ap_spec in self._ap_specs:
                ap_locker = ap_batch_locker.ApBatchLocker(
                        lock_manager, ap_spec,
                        ap_test_type=ap_constants.AP_TEST_TYPE_CLIQUE)
                ap = ap_locker.get_ap_batch(batch_size=1)
                if not ap:
                    raise error.TestError('AP matching spec not found.')
                aps.append(ap)

            # Reset all the DUTs before the test starts and configure all the
            # APs.
            self._sanitize_all_duts(dut_objects)
            utils.configure_aps(aps, self._ap_specs)

            # This is a list of association parameters for the test for all the
            # APs in the test.
            assoc_params_list = []
            # Check if all our APs, DUTs and connection workers are in good
            # state before we proceed.
            for ap, ap_spec in zip(aps, self._ap_specs):
                if ap.ssid == None:
                    self._cleanup(dut_objects, dut_locker, ap_locker,
                                  capturer, conn_workers)
                    raise error.TestError('SSID not set for the AP: %s.' %
                                          ap.configurator.host_name)
                networks = utils.return_available_networks(
                        ap, ap_spec, capturer, job)
                if ((networks is None) or (networks == list())):
                    self._cleanup(dut_objects, dut_locker, ap_locker,
                                  capturer, conn_workers)
                    raise error.TestError('Scanning error on the AP %s.' %
                                          ap.configurator.host_name)

                assoc_params = ap.get_association_parameters()
                assoc_params_list.append(assoc_params)

            if not self._are_all_duts_healthy(dut_objects, ap):
                self._cleanup(dut_objects, dut_locker, ap_locker,
                              capturer, conn_workers)
                raise error.TestError('Not all DUTs healthy.')

            if not self._are_all_conn_workers_healthy(
                    conn_workers, aps, assoc_params_list, job):
                self._cleanup(dut_objects, dut_locker, ap_locker,
                              capturer, conn_workers)
                raise error.TestError('Not all connection workers healthy.')

            debug_string = self._get_debug_string(dut_objects, aps)
            self._sync_time_on_all_duts(dut_objects)

            result = job.run_test(
                    self._test,
                    capturer=capturer,
                    capturer_frequency=networks[0].frequency,
                    capturer_ht_type=networks[0].ht,
                    dut_pool=self._dut_pool,
                    assoc_params_list=assoc_params_list,
                    tries=tries,
                    debug_info=debug_string,
                    conn_workers=conn_workers,
                    # Copy all logs from the system
                    disabled_sysinfo=disabled_sysinfo)

            # Reclaim all the APs, DUTs and capturers used in the test and
            # collect the required logs.
            self._cleanup(dut_objects, dut_locker, ap_locker,
                          capturer, conn_workers)
