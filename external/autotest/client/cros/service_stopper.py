# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Provides utility class for stopping and restarting services

When using this class, one likely wishes to do the following:

    def initialize(self):
        self._services = service_stopper.ServiceStopper(['service'])
        self._services.stop_services()


    def cleanup(self):
        self._services.start_services()

As this ensures that the services will be off before the test code runs, and
the test framework will ensure that the services are restarted through any
code path out of the test.
"""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class ServiceStopper(object):
    """Class to manage CrOS services.
    Public attributes:
      services_to_stop: list of services that should be stopped

   Public constants:
      POWER_DRAW_SERVICES: list of services that influence power test in
    unpredictable/undesirable manners.

    Public methods:
      stop_sevices: stop running system services.
      restore_services: restore services that were previously stopped.

    Private attributes:
      _services_stopped: list of services that were successfully stopped
    """

    POWER_DRAW_SERVICES = ['powerd', 'update-engine', 'bluetoothd', 'vnc']

    def __init__(self, services_to_stop=[]):
        """Initialize instance of class.

        By Default sets an empty list of services.
        """
        self.services_to_stop = services_to_stop
        self._services_stopped = []


    def stop_services(self):
        """Turn off managed services."""

        for service in self.services_to_stop:
            cmd = 'status %s' % service
            out = utils.system_output(cmd, ignore_status=True)
            is_stopped = 'start/running' not in out
            if is_stopped:
                continue
            try:
                utils.system('stop %s' % service)
                self._services_stopped.append(service)
            except error.CmdError as e:
                logging.warning('Error stopping service %s. %s',
                                service, str(e))


    def restore_services(self):
        """Restore services that were stopped."""
        for service in reversed(self._services_stopped):
            utils.system('start %s' % service, ignore_status=True)
        self._services_stopped = []


    def __enter__(self):
        self.stop_services()
        return self


    def __exit__(self, exnval, exntype, exnstack):
        self.restore_services()


    def close(self):
        """Equivalent to restore_services."""
        self.restore_services()
