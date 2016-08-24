# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import shutil

from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import avahi_utils
from autotest_lib.client.cros import service_stopper, tcpdump


P2P_SHARE_PATH = '/var/cache/p2p'

# A path used to store the existing p2p files during the test and restore them
# once the test finishes.
P2P_SHARE_BACKUP_PATH = '/var/cache/p2p-backup'


def p2p_backup_files(backup_path=P2P_SHARE_BACKUP_PATH):
    """Backup the P2P shared files and create an empty shared directory.

    p2p-server shall not be running during backup or restore.

    @param backup_path: The path where the files will be moved to.
    @raise error.TestError
    """
    try:
        if os.path.exists(backup_path):
            shutil.rmtree(backup_path)
        if os.path.exists(P2P_SHARE_PATH):
            os.rename(P2P_SHARE_PATH, backup_path)
    except OSError, e:
        raise error.TestError("Error on P2P files backup: %s" % (e.message))


def p2p_restore_files(backup_path=P2P_SHARE_BACKUP_PATH):
    """Restore the P2P shared files from a backup and *delete* the backup.

    p2p-server shall not be running during backup or restore.

    @param backup_path: The path where the files will be moved from.
    """
    if os.path.exists(P2P_SHARE_PATH):
        shutil.rmtree(P2P_SHARE_PATH, ignore_errors=True)
    if os.path.exists(backup_path):
        os.rename(backup_path, P2P_SHARE_PATH)


class P2PServerOverTap(object):
    """Manage a p2p-server instance running over a TAP interface.

    This class manages a p2p-server instance configured to run over a TAP
    interface, useful for any test that needs to interact with the p2p-server
    (and its p2p-http-server instance) on a controled network environment.
    """
    def __init__(self, tap_ip='169.254.10.1', tap_mask=24, tap_name='faketap'):
        """Initialize the configuration.

        @param tap_ip: IPv4 address for the TAP interface on the DUT's end.
        @param tap_mask: Network mask fot the tap_ip address.
        @param tap_name: The name prefix for the TAP interface.
        """
        # The network 169.254/16 shouldn't clash with other real services and we
        # use a /24 subnet of it as the default safe value here.
        self._tap_ip = tap_ip
        self._tap_mask = tap_mask
        self._tap_name = tap_name
        self._services = None
        self.tap = None
        self._tcpdump = None


    def setup(self, dumpdir=None):
        """Initializes avahi daemon on a new tap interface.

        @param dumpdir: Directory where the traffic on the new tap interface
                        is recorded. A value of None disables traffic dumping.
        """
        try:
            from lansim import tuntap
        except ImportError:
            logging.exception('Failed to import lansim.')
            raise error.TestError('Error importing lansim. Did you setup_dep '
                                  'and install_pkg lansim on your test?')

        # Ensure p2p and avahi aren't running.
        self._services = service_stopper.ServiceStopper(['p2p', 'avahi'])
        self._services.stop_services()

        # Backup p2p files.
        p2p_backup_files()

        # Initialize the TAP interface.
        self.tap = tuntap.TunTap(tuntap.IFF_TAP, name=self._tap_name)
        self.tap.set_addr(self._tap_ip, self._tap_mask)
        self.tap.up()

        # Enable traffic dump.
        if not dumpdir is None:
            dumpfile = os.path.join(dumpdir, 'dump-%s.pcap' % self.tap.name)
            self._tcpdump = tcpdump.Tcpdump(self.tap.name, dumpfile)

        # Re-launch avahi-daemon on the TAP interface only.
        avahi_utils.avahi_start_on_iface(self.tap.name)
        utils.system("start p2p")


    def cleanup(self):
        """Restore the original environment as before the call to setup().

        This method makes a best-effort attempt to restore the environment and
        logs all the errors encountered but doesn't fail.
        """
        try:
            utils.system('stop p2p')
            avahi_utils.avahi_stop()
        except:
            logging.exception('Failed to stop tested services.')

        if self._tcpdump:
            self._tcpdump.stop()

        if self.tap:
            self.tap.down()

        # Restore p2p files.
        try:
            p2p_restore_files()
        except OSError:
            logging.exception('Failed to restore the P2P backup.')

        if self._services:
            self._services.restore_services()
