#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module used to sync and deploy infrastructure changes for the lab team.

This is a helper and used by lab_deploy which bootstraps and calls this utility
once a user has logged into the main autotest server. This can be called
directly if already on the autotest server.

Usage:
  lab_deploy_helper.py  (sync,restart,print) (devservers, drones, scheduler)+.
"""
import logging
import sys

import common
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import hosts

import common_util

CONFIG = global_config.global_config


def devserver_list():
    """Returns the list of devserver-type machines used by the infrastructure/
    """
    image_servers = dev_server.ImageServer.servers()
    crash_servers = dev_server.CrashServer.servers()
    return image_servers + crash_servers


def autotest_scheduler_drones():
    """Returns tuple containing the autotest scheduler and list of drones."""
    autotest_master = CONFIG.get_config_value('scheduler', 'host', type=str,
                                              default=None)
    autotest_drones = CONFIG.get_config_value('scheduler', 'drones',
                                              type=list, default=[])
    return autotest_master, autotest_drones


def devserver_restart(host):
    """SSH's in to |host| and restarts the devserver instance.

    This method uses puppet apply to restart the devserver instance on host.
    """
    logging.info('METHOD STUB called for restarting devserver on %s', host)
    # host.run('puppet apply devserver_start')


def devserver_sync(host):
    """SSH's in to |host| and syncs the devserver.

    This method uses puppet apply to sync the devserver instance on host.
    """
    logging.info('METHOD STUB called for syncing devserver on %s', host)
    # host.run('puppet apply devserver_sync')


def autotest_restart(host):
    """SSH's in to |host| and restarts autotest instance.

    This method uses puppet apply to restart autotest installed on the host.
    """
    logging.info('METHOD STUB called for restarting autotest on %s', host)
    # host.run('puppet apply autotest_start')


def autotest_sync(host):
    """SSH's in to |host| and syncs autotest.

    This method uses puppet apply to sync autotest.
    """
    logging.info('METHOD STUB called for syncing autotest on %s', host)
    # host.run('puppet apply autotest_sync')


def main(argv):
    common_util.setup_logging()
    args = common_util.parse_args(argv)
    requested_server_set = set(args.servers)
    devservers = devserver_list()
    master, drones = autotest_scheduler_drones()

    if args.operation == common_util.SYNC:
        if common_util.DEVS in requested_server_set:
            for server in devservers:
                devserver_sync(hosts.SSHHost(server))

        if common_util.DRONES in requested_server_set:
            for server in drones:
                autotest_sync(hosts.SSHHost(server))

        if common_util.SCHEDULER in requested_server_set:
            autotest_sync(master)

    elif args.operation == common_util.RESTART:
        if common_util.DEVS in requested_server_set:
            for server in devservers:
                devserver_restart(hosts.SSHHost(server))

        if common_util.DRONES in requested_server_set:
            for server in drones:
                autotest_restart(hosts.SSHHost(server))

        if common_util.SCHEDULER in requested_server_set:
            autotest_restart(master)

    elif args.operation == common_util.PRINT:
        if common_util.DEVS in requested_server_set:
            for server in devservers:
                print server

        if common_util.DRONES in requested_server_set:
            for server in drones:
                print server

        if common_util.SCHEDULER in requested_server_set:
            print master

    return 0


if __name__ == '__main__':
    main(sys.argv[1:])
