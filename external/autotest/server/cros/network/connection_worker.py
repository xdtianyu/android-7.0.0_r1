# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import hosts
from autotest_lib.server.cros.network import wifi_client
from autotest_lib.server.cros.network import netperf_runner

WORK_CLIENT_CONNECTION_RETRIES = 3
WAIT_FOR_CONNECTION = 10

class ConnectionWorker(object):
    """ ConnectionWorker is a thin layer of interfaces for worker classes """

    @property
    def name(self):
        """@return a string: representing name of the worker class"""
        raise NotImplementedError('Missing subclass implementation')


    @classmethod
    def create_from_parent(cls, parent_obj, **init_args):
        """Creates a derived ConnectionWorker object from the provided parent
        object.

        @param cls: derived class object which we're trying to create.
        @param obj: existing parent class object.
        @param init_args: Args to be passed to the derived class constructor.

        @returns Instance of cls with the required fields copied from parent.
        """
        obj = cls(**init_args)
        obj.work_client = parent_obj.work_client
        obj.host = parent_obj.host
        return obj


    def prepare_work_client(self, work_client_machine):
        """Prepare the SSHHost object into WiFiClient object

        @param work_client_machine: a SSHHost object to be wrapped

        """
        work_client_host = hosts.create_host(work_client_machine.hostname)
        # All packet captures in chaos lab have dual NICs. Let us use phy1 to
        # be a radio dedicated for work client
        iw = iw_runner.IwRunner(remote_host=work_client_host)
        phys = iw.list_phys()
        devs = iw.list_interfaces(desired_if_type='managed')
        if len(devs) > 0:
            logging.debug('Removing interfaces in work host machine %s', devs)
            for i in range(len(devs)):
                iw.remove_interface(devs[i].if_name)
        if len(phys) > 1:
            logging.debug('Adding interfaces in work host machine')
            iw.add_interface('phy1', 'work0', 'managed')
            logging.debug('Interfaces in work client %s', iw.list_interfaces())
        elif len(phys) == 1:
            raise error.TestError('Not enough phys available to create a'
                                  'work client interface %s.' %
                                   work_client_host.hostname)
        self.work_client = wifi_client.WiFiClient(
                work_client_host, './debug', False)
        # Make the host object easily accessible
        self.host = self.work_client.host


    def connect_work_client(self, assoc_params):
        """
        Connect client to the AP.

        Tries to connect the work client to AP in WORK_CLIENT_CONNECTION_RETRIES
        tries. If we fail to connect in all tries then we would return False
        otherwise returns True on successful connection to the AP.

        @param assoc_params: an AssociationParameters object.
        @return a boolean: True if work client is successfully connected to AP
                or False on failing to connect to the AP

        """
        if not self.work_client.shill.init_test_network_state():
            logging.error('Failed to set up isolated test context profile for '
                          'work client.')
            return False

        success = False
        for i in range(WORK_CLIENT_CONNECTION_RETRIES):
            logging.info('Connecting work client to AP')
            assoc_result = xmlrpc_datatypes.deserialize(
                           self.work_client.shill.connect_wifi(assoc_params))
            success = assoc_result.success
            if not success:
                logging.error('Connection attempt of work client failed, try %d'
                              ' reason: %s', (i+1), assoc_result.failure_reason)
            else:
                logging.info('Work client connected to the AP')
                self.ssid = assoc_params.ssid
                break
        return success


    def cleanup(self):
        """Teardown work_client"""
        self.work_client.shill.disconnect(self.ssid)
        self.work_client.shill.clean_profiles()


    def run(self, client):
        """Executes the connection worker

        @param client: WiFiClient object representing the DUT

        """
        raise NotImplementedError('Missing subclass implementation')


class ConnectionDuration(ConnectionWorker):
    """This test is to check the liveliness of the connection to the AP. """

    def __init__(self, duration_sec=30):
        """
        Holds WiFi connection open with periodic pings

        @param duration_sec: amount of time to hold connection in seconds

        """

        self.duration_sec = duration_sec


    @property
    def name(self):
        """@return a string: representing name of this class"""
        return 'duration'


    def run(self, client):
        """Periodically pings work client to check liveliness of the connection

        @param client: WiFiClient object representing the DUT

        """
        ping_config = ping_runner.PingConfig(self.work_client.wifi_ip, count=10)
        logging.info('Pinging work client ip: %s', self.work_client.wifi_ip)
        start_time = time.time()
        while time.time() - start_time < self.duration_sec:
            time.sleep(10)
            ping_result = client.ping(ping_config)
            logging.info('Connection liveness %r', ping_result)


class ConnectionSuspend(ConnectionWorker):
    """
    This test is to check the liveliness of the connection to the AP with
    suspend resume cycle involved.

    """

    def __init__(self, suspend_sec=30):
        """
        Construct a ConnectionSuspend.

        @param suspend_sec: amount of time to suspend in seconds

        """

        self._suspend_sec = suspend_sec


    @property
    def name(self):
        """@return a string: representing name of this class"""
        return 'suspend'


    def run(self, client):
        """
        Check the liveliness of the connection to the AP by pinging the work
        client before and after a suspend resume.

        @param client: WiFiClient object representing the DUT

        """
        ping_config = ping_runner.PingConfig(self.work_client.wifi_ip, count=10)
        # pinging work client to ensure we have a connection
        logging.info('work client ip: %s', self.work_client.wifi_ip)
        ping_result = client.ping(ping_config)
        logging.info('before suspend:%r', ping_result)
        client.do_suspend(self._suspend_sec)
        # When going to suspend, DUTs using ath9k devices do not disassociate
        # from the AP. On resume, DUTs would re-use the association from prior
        # to suspend. However, this leads to some confused state for some APs
        # (see crbug.com/346417) where the AP responds to actions frames like
        # NullFunc but not to any data frames like DHCP/ARP packets from the
        # DUT.  Let us sleep for:
        #       + 2 seconds for linkmonitor to detect failure if any
        #       + 10 seconds for ReconnectTimer timeout
        #       + 5 seconds to reconnect to the AP
        #       + 3 seconds let us not have a very strict timeline.
        # 20 seconds before we start to query shill about the connection state.
        # TODO (krisr): add board detection code in wifi_client and adjust the
        # sleep time here based on the wireless chipset
        time.sleep(20)

        # Wait for WAIT_FOR_CONNECTION time before trying to ping.
        success, state, elapsed_time = client.wait_for_service_states(
                self.ssid, ('ready', 'portal', 'online'), WAIT_FOR_CONNECTION)
        if not success:
            raise error.TestFail('DUT failed to connect to AP (%s state) after'
                                 'resume in %d seconds' %
                                 (state, WAIT_FOR_CONNECTION))
        else:
            logging.info('DUT entered %s state after %s seconds',
                         state, elapsed_time)
            # ping work client to ensure we have connection after resume.
            ping_result = client.ping(ping_config)
            logging.info('after resume:%r', ping_result)


class ConnectionNetperf(ConnectionWorker):
    """
    This ConnectionWorker is used to run a sustained data transfer between the
    DUT and the work_client through an AP.

    """

    # Minimum expected throughput for netperf streaming tests
    NETPERF_MIN_THROUGHPUT = 2.0 # Mbps

    def __init__(self, netperf_config):
        """
        Construct a ConnectionNetperf object.

        @param netperf_config: NetperfConfig object to define transfer test.

        """
        self._config = netperf_config


    @property
    def name(self):
        """@return a string: representing name of this class"""
        return 'netperf_%s' % self._config.human_readable_tag


    def run(self, client):
        """
        Create a NetperfRunner, run netperf between DUT and work_client.

        @param client: WiFiClient object representing the DUT

        """
        with netperf_runner.NetperfRunner(
                client, self.work_client, self._config) as netperf:
            ping_config = ping_runner.PingConfig(
                    self.work_client.wifi_ip, count=10)
            # pinging work client to ensure we have a connection
            logging.info('work client ip: %s', self.work_client.wifi_ip)
            ping_result = client.ping(ping_config)

            result = netperf.run(self._config)
            logging.info('Netperf Result: %s', result)

        if result is None:
            raise error.TestError('Failed to create NetperfResult')

        if result.duration_seconds < self._config.test_time:
            raise error.TestFail(
                    'Netperf duration too short: %0.2f < %0.2f' %
                    (result.duration_seconds, self._config.test_time))

        # TODO: Convert this limit to a perf metric crbug.com/348780
        if result.throughput <self.NETPERF_MIN_THROUGHPUT:
            raise error.TestFail(
                    'Netperf throughput too low: %0.2f < %0.2f' %
                    (result.throughput, self.NETPERF_MIN_THROUGHPUT))
