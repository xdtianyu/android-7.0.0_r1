# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import apmanager_constants
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.networking import apmanager_proxy

class apmanager_CheckAPProcesses(test.test):
    """Test that required processes are created/terminated when AP service
    is started/terminated.
    """
    version = 1


    POLLING_INTERVAL_SECONDS = 0.2
    # These services interact with the apmanager in undesirable ways.
    # For instance, buffet has a bad habit of starting up APs, which
    # prevents the test from doing likewise.
    RELATED_SERVICES = ['buffet']


    def _verify_process(self,
                        process_name,
                        expected_running,
                        timeout_seconds=10):
        """Verify the given process |process_name| is running |running|.

        @param process_name string name of the process
        @param expected_running bool flag indicating expected process state
        @param timeout_seconds float number of seconds to wait for the process
               to transition to the expected state.

        """
        endtime = time.time() + timeout_seconds
        while endtime > time.time():
            out = utils.system_output('pgrep %s 2>&1' % process_name,
                                      retain_output=True,
                                      ignore_status=True).strip()
            actual_running = (out != '')
            if expected_running == actual_running:
                break
            time.sleep(self.POLLING_INTERVAL_SECONDS)
        else:
            raise error.TestFail(
                    'Process %s running status expected %r actual %r'
                    % (process_name, expected_running, actual_running))


    def run_once(self):
        """Test body."""
        with service_stopper.ServiceStopper(
                services_to_stop=self.RELATED_SERVICES):
            # AP configuration parameters, only configuring SSID.
            ap_config = {apmanager_constants.CONFIG_SSID: 'testap'}
            manager = apmanager_proxy.ApmanagerProxy()
            service = manager.start_service(ap_config)
            self._verify_process('hostapd', True)
            self._verify_process('dnsmasq', True)
            manager.terminate_service(service)
            self._verify_process('hostapd', False)
            self._verify_process('dnsmasq', False)
