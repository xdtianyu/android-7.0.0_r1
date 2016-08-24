# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros import debugd_dev_tools


class debugd_DevTools(test.test):
    """
    Debugd dev tools test. See control file for details.
    """
    version = 1


    def create_tools(self, host):
        """
        Creates and initializes the tools needed for the test.

        Saves a RootfsVerificationTool to self.rootfs_tool and the rest
        to self.tools. The RootfsVerificationTool is handled separately
        because it can't be disabled and is required first for the
        other tools to function properly.

        @param host: Host device.

        @throw error.TestNAError: Dev tools are unavailable.
        """
        if not debugd_dev_tools.are_dev_tools_available(host):
            raise error.TestNAError('Cannot access dev tools. Make sure the '
                                    'device is in dev mode with no owner and '
                                    'the boot lockbox is not finalized.')

        logging.debug('Creating dev tools.')
        self.rootfs_tool = debugd_dev_tools.RootfsVerificationTool()
        self.tools = (debugd_dev_tools.BootFromUsbTool(),
                      debugd_dev_tools.SshServerTool(),
                      debugd_dev_tools.SystemPasswordTool())

        logging.debug('Initializing dev tools.')
        self.rootfs_tool.initialize(host)
        for tool in self.tools:
            tool.initialize(host, save_initial_state=True)


    def cleanup_tools(self, host):
        """
        Performs cleanup to return the device to its initial state.

        Any tools that fail to clean up will print a warning but will
        not register a test failure.

        @param host: Host device.
        """
        logging.debug('Cleaning up tools.')
        for tool in self.tools:
            try:
                tool.restore_state()
            except debugd_dev_tools.FeatureUnavailableError as e:
                logging.warning('Could not restore %s - device state may be '
                                'altered by test (%s).', tool, e)
        debugd_dev_tools.remove_temp_files(host)


    def test_tool(self, tool):
        """
        Tests an individual tool.

        Functionality is tested by disabling, enabling, then disabling
        again. Certain tools may be unavailable on a board (e.g. USB
        boot on Mario), which will log a warning but not register a
        test failure.

        @param tool: Tool object to test.

        @throw debugd_dev_tools.AccessError: Dev tool access failed.
        @throw error.TestFail: A tool failed to affect device state.
        """
        # Start by disabling the tool. If disable fails we may still be
        # able to test enabling the tool.
        logging.debug('Disabling %s.', tool)
        try:
            tool.disable()
        except debugd_dev_tools.FeatureUnavailableError as e:
            # If the tool can't be disabled and is already enabled there's no
            # way to test if our enable function is working or not.
            if tool.is_enabled():
                logging.warning('Skipping %s - cannot disable (%s).', tool, e)
                return
        if tool.is_enabled():
            raise error.TestFail('%s did not disable correctly.' % tool)

        # Now enable the tool and make sure it worked.
        logging.debug('Enabling %s.', tool)
        try:
            tool.enable()
        except debugd_dev_tools.FeatureUnavailableError as e:
            logging.warning('Skipping %s - cannot enable (%s).', tool, e)
            return
        if not tool.is_enabled():
            raise error.TestFail('%s did not enable correctly.' % tool)

        # Disable one more time to confirm our disable routine works.
        logging.debug('Disabling %s.', tool)
        try:
            tool.disable()
        except debugd_dev_tools.FeatureUnavailableError:
            return
        if tool.is_enabled():
            raise error.TestFail('%s did not disable correctly.' % tool)


    def run_once(self, host=None):
        self.create_tools(host)
        try:
            # First remove rootfs verification if it's not already.
            if not self.rootfs_tool.is_enabled():
                logging.debug('Removing rootfs verification.')
                self.rootfs_tool.enable()

            for tool in self.tools:
                self.test_tool(tool)
        finally:
            self.cleanup_tools(host)
