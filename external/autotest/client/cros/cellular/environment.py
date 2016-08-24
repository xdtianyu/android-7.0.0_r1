# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Encapsulates interactions with the cellular testbed."""

import contextlib, logging, urllib2

# We'd really prefer not to depend on autotest proper here
from autotest_lib.client.bin import utils

from autotest_lib.client.cros import backchannel, flimflam_test_path
from autotest_lib.client.cros.cellular import cellular, cell_tools
from autotest_lib.client.cros.cellular import emulator_config

import flimflam


TIMEOUT = 60

class Error(Exception):
    pass


class Environment(object):
    """Dispatch class:  reads config and returns appropriate concrete type."""
    def __new__(cls, config, *args, **kwargs):
        return EmulatedEnvironment(config, *args, **kwargs)


class EmulatedEnvironment(object):
    def __init__(self, config):
        self.config = config
        self.flim = None
        self.emulator = None

    def __enter__(self):
        self.flim = flimflam.FlimFlam()
        return self

    def __exit__(self, exception, value, traceback):
        if self.emulator:
            self.emulator.Close()
        return False

    def StartDefault(self, technology):
        (self.emulator, self.verifier) = emulator_config.StartDefault(
            self.config, technology)

    def CheckHttpConnectivity(self):
        """Check that the device can fetch HTTP pages."""
        http_config = self.config.cell['http_connectivity']
        response = urllib2.urlopen(http_config['url'], timeout=TIMEOUT).read()

        if ('url_required_contents' in http_config and
            http_config['url_required_contents'] not in response):
            logging.error('Could not find %s in \n\t%s\n',
                          http_config['url_required_contents'], response)
            raise Error('Content downloaded, but it was incorrect')

    def CheckedConnectToCellular(self, timeout=TIMEOUT):
        """Connect to cellular, check if we are connected, return a service"""
        (service, _) = cell_tools.ConnectToCellular(self.flim, timeout=timeout)
        self.verifier.AssertDataStatusIn([
            cellular.UeGenericDataStatus.CONNECTED])
        return service

    def CheckedDisconnectFromCellular(self, service):
        """Disconnect from cellular and check that we're disconnected."""

        self.flim.DisconnectService(service)

        def _ModemIsFullyDisconnected():
            return self.verifier.IsDataStatusIn([
                cellular.UeGenericDataStatus.REGISTERED,
                cellular.UeGenericDataStatus.NONE,])

        utils.poll_for_condition(
            _ModemIsFullyDisconnected,
            timeout=20,
            exception=Error('modem not disconnected from base station'))


class DefaultCellularTestContext(object):
    """Wraps useful contexts for a cellular test in a single context."""
    def __init__(self, config):
        self._nested = contextlib.nested(
            backchannel.Backchannel(),
            cell_tools.OtherDeviceShutdownContext('cellular'),
            Environment(config))

    def __enter__(self):
        (self.backchannel,
         self.other_device_shutdown_context,
         self.env) = self._nested.__enter__()
        return self

    def __exit__(self, exception, value, traceback):
        return self._nested.__exit__(exception, value, traceback)
