# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Sonic host.

This host can perform actions either over ssh or by submitting requests to
an http server running on the client. Though the server provides flexibility
and allows us to test things at a modular level, there are times we must
resort to ssh (eg: to reboot into recovery). The server exposes the same stack
that the chromecast extension needs to communicate with the sonic device, so
any test involving an sonic host will fail if it cannot submit posts/gets
to the server. In cases where we can achieve the same action over ssh or
the rpc server, we choose the rpc server by default, because several existing
sonic tests do the same.
"""

import logging
import os

import common

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.common_lib import error
from autotest_lib.server import site_utils
from autotest_lib.server.cros import sonic_client_utils
from autotest_lib.server.hosts import abstract_ssh


class SonicHost(abstract_ssh.AbstractSSHHost):
    """This class represents a sonic host."""

    # Maximum time a reboot can take.
    REBOOT_TIME = 360

    COREDUMP_DIR = '/data/coredump'
    OTA_LOCATION = '/cache/ota.zip'
    RECOVERY_DIR = '/cache/recovery'
    COMMAND_FILE = os.path.join(RECOVERY_DIR, 'command')
    PLATFORM = 'sonic'
    LABELS = [sonic_client_utils.SONIC_BOARD_LABEL]


    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is a sonic host.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.

        @return: True if the host device is sonic.

        @raises AutoservRunError: If the command failed.
        @raises AutoservSSHTimeout: Ssh connection has timed out.
        """
        try:
            result = host.run('getprop ro.product.device', timeout=timeout)
        except (error.AutoservRunError, error.AutoservSSHTimeout,
                error.AutotestHostRunError):
            return False
        return 'anchovy' in result.stdout


    def _initialize(self, hostname, *args, **dargs):
        super(SonicHost, self)._initialize(hostname=hostname, *args, **dargs)

        # Sonic devices expose a server that can respond to json over http.
        self.client = sonic_client_utils.SonicProxy(hostname)


    def enable_test_extension(self):
        """Enable a chromecast test extension on the sonic host.

        Appends the extension id to the list of accepted cast
        extensions, without which the sonic device will fail to
        respond to any Dial requests submitted by the extension.

        @raises CmdExecutionError: If the expected files are not found
            on the sonic host.
        """
        extension_id = sonic_client_utils.get_extension_id()
        tempdir = autotemp.tempdir()
        local_dest = os.path.join(tempdir.name, 'content_shell.sh')
        remote_src = '/system/usr/bin/content_shell.sh'
        whitelist_flag = '--extra-cast-extension-ids'

        try:
            self.run('mount -o rw,remount /system')
            self.get_file(remote_src, local_dest)
            with open(local_dest) as f:
                content = f.read()
                if extension_id in content:
                    return
                if whitelist_flag in content:
                    append_str = ',%s' % extension_id
                else:
                    append_str = ' %s=%s' % (whitelist_flag, extension_id)

            with open(local_dest, 'a') as f:
                f.write(append_str)
            self.send_file(local_dest, remote_src)
            self.reboot()
        finally:
            tempdir.clean()


    def get_boot_id(self, timeout=60):
        """Get a unique ID associated with the current boot.

        @param timeout The number of seconds to wait before timing out, as
            taken by base_utils.run.

        @return A string unique to this boot or None if not available.
        """
        BOOT_ID_FILE = '/proc/sys/kernel/random/boot_id'
        cmd = 'cat %r' % (BOOT_ID_FILE)
        return self.run(cmd, timeout=timeout).stdout.strip()


    def get_platform(self):
        return self.PLATFORM


    def get_labels(self):
        return self.LABELS


    def ssh_ping(self, timeout=60, base_cmd=''):
        """Checks if we can ssh into the host and run getprop.

        Ssh ping is vital for connectivity checks and waiting on a reboot.
        A simple true check, or something like if [ 0 ], is not guaranteed
        to always exit with a successful return value.

        @param timeout: timeout in seconds to wait on the ssh_ping.
        @param base_cmd: The base command to use to confirm that a round
            trip ssh works.
        """
        super(SonicHost, self).ssh_ping(timeout=timeout,
                                         base_cmd="getprop>/dev/null")


    def verify_software(self):
        """Verified that the server on the client device is responding to gets.

        The server on the client device is crucial for the sonic device to
        communicate with the chromecast extension. Device verify on the whole
        consists of verify_(hardware, connectivity and software), ssh
        connectivity is verified in the base class' verify_connectivity.

        @raises: SonicProxyException if the server doesn't respond.
        """
        self.client.check_server()


    def get_build_number(self, timeout_mins=1):
        """
        Gets the build number on the sonic device.

        Since this method is usually called right after a reboot/install,
        it has retries built in.

        @param timeout_mins: The timeout in minutes.

        @return: The build number of the build on the host.

        @raises TimeoutError: If we're unable to get the build number within
            the specified timeout.
        @raises ValueError: If the build number returned isn't an integer.
        """
        cmd = 'getprop ro.build.version.incremental'
        timeout = timeout_mins * 60
        cmd_result = utils.poll_for_condition(
                        lambda: self.run(cmd, timeout=timeout/10),
                        timeout=timeout, sleep_interval=timeout/10)
        return int(cmd_result.stdout)


    def get_kernel_ver(self):
        """Returns the build number of the build on the device."""
        return self.get_build_number()


    def reboot(self, timeout=5):
        """Reboot the sonic device by submitting a post to the server."""

        # TODO(beeps): crbug.com/318306
        current_boot_id = self.get_boot_id()
        try:
            self.client.reboot()
        except sonic_client_utils.SonicProxyException as e:
            raise error.AutoservRebootError(
                    'Unable to reboot through the sonic proxy: %s' % e)

        self.wait_for_restart(timeout=timeout, old_boot_id=current_boot_id)


    def cleanup(self):
        """Cleanup state.

        If removing state information fails, do a hard reboot. This will hit
        our reboot method through the ssh host's cleanup.
        """
        try:
            self.run('rm -r /data/*')
            self.run('rm -f /cache/*')
        except (error.AutotestRunError, error.AutoservRunError) as e:
            logging.warning('Unable to remove /data and /cache %s', e)
            super(SonicHost, self).cleanup()


    def _remount_root(self, permissions):
        """Remount root partition.

        @param permissions: Permissions to use for the remount, eg: ro, rw.

        @raises error.AutoservRunError: If something goes wrong in executing
            the remount command.
        """
        self.run('mount -o %s,remount /' % permissions)


    def _setup_coredump_dirs(self):
        """Sets up the /data/coredump directory on the client.

        The device will write a memory dump to this directory on crash,
        if it exists. No crashdump will get written if it doesn't.
        """
        try:
            self.run('mkdir -p %s' % self.COREDUMP_DIR)
            self.run('chmod 4777 %s' % self.COREDUMP_DIR)
        except (error.AutotestRunError, error.AutoservRunError) as e:
            error.AutoservRunError('Unable to create coredump directories with '
                                   'the appropriate permissions: %s' % e)


    def _setup_for_recovery(self, update_url):
        """Sets up the /cache/recovery directory on the client.

        Copies over the OTA zipfile from the update_url to /cache, then
        sets up the recovery directory. Normal installs are achieved
        by rebooting into recovery mode.

        @param update_url: A url pointing to a staged ota zip file.

        @raises error.AutoservRunError: If something goes wrong while
            executing a command.
        """
        ssh_cmd = '%s %s' % (self.make_ssh_command(), self.hostname)
        site_utils.remote_wget(update_url, self.OTA_LOCATION, ssh_cmd)
        self.run('ls %s' % self.OTA_LOCATION)

        self.run('mkdir -p %s' % self.RECOVERY_DIR)

        # These 2 commands will always return a non-zero exit status
        # even if they complete successfully. This is a confirmed
        # non-issue, since the install will actually complete. If one
        # of the commands fails we can only detect it as a failure
        # to install the specified build.
        self.run('echo --update_package>%s' % self.COMMAND_FILE,
                 ignore_status=True)
        self.run('echo %s>>%s' % (self.OTA_LOCATION, self.COMMAND_FILE),
                 ignore_status=True)


    def machine_install(self, update_url):
        """Installs a build on the Sonic device.

        @returns String of the current build number.
        """
        old_build_number = self.get_build_number()
        self._remount_root(permissions='rw')
        self._setup_coredump_dirs()
        self._setup_for_recovery(update_url)

        current_boot_id = self.get_boot_id()
        self.run_background('reboot recovery')
        self.wait_for_restart(timeout=self.REBOOT_TIME,
                              old_boot_id=current_boot_id)
        new_build_number = self.get_build_number()

        # TODO(beeps): crbug.com/318278
        if new_build_number ==  old_build_number:
            raise error.AutoservRunError('Build number did not change on: '
                                         '%s after update with %s' %
                                         (self.hostname, update_url()))

        return str(new_build_number)
