#!/usr/bin/env python

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to sync lab servers to the "Allowed Networks" of a CloudSQL instance.

For a lab server to access CloudSQL instance, the server's IP must be added to
the "Allowed Networks" list of the CloudSQL instance. This tool is to be used to
read the list of lab servers from server database and update the list of
"Allowed Networks" of a given CloudSQL instance.

The tool also reads CLOUD/tko_access_servers from global config to add these
servers to the "Allowed Networks" list of the CloudSQL instance. This allows
servers that do not run Autotest code can access the CloudSQL instance.

Note that running this tool will overwrite existing IPs in the "Allowed
Networks" list. Therefore, manually editing that list from CloudSQL console
should be prohibited. Instead, the servers should be added to
CLOUD/tko_access_servers in shadow_config.ini.

"""

import argparse
import socket
import sys

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import frontend


ROLES_REQUIRE_TKO_ACCESS = {'scheduler', 'drone', 'shard', 'database', 'afe'}

def gcloud_login(project):
    """Login to Google Cloud service for gcloud command to run.

    @param project: Name of the Google Cloud project.
    """
    # Login with user account. If the user hasn't log in yet, the script will
    # print a url and ask for a verification code. User should load the url in
    # browser, and copy the verification code from the web page. When private IP
    # can be supported to be added using non-corp account, the login can be done
    # through service account and key file, e.g.,
    # gcloud auth activate-service-account --key-file ~/key.json
    utils.run('gcloud auth login', stdout_tee=sys.stdout,
              stderr_tee=sys.stderr, stdin=sys.stdin)


def update_allowed_networks(project, instance, afe=None, extra_servers=None):
    """Update the "Allowed Networks" list of the given CloudSQL instance.

    @param project: Name of the Google Cloud project.
    @param instance: Name of the CloudSQL instance.
    @param afe: Server of the frontend RPC, default to None to use the server
                specified in global config.
    @param extra_servers: Extra servers to be included in the "Allowed Networks"
                          list. Default is None.
    """
    # Get the IP address of all servers need access to CloudSQL instance.
    rpc = frontend.AFE(server=afe)
    servers = [s['hostname'] for s in rpc.run('get_servers')
               if s['status'] != 'repair_required' and
               ROLES_REQUIRE_TKO_ACCESS.intersection(s['roles'])]
    if extra_servers:
        servers.extend(extra_servers.split(','))
    # Extra servers can be listed in CLOUD/tko_access_servers shadow config.
    tko_servers = global_config.global_config.get_config_value(
            'CLOUD', 'tko_access_servers', default='')
    if tko_servers:
        servers.extend(tko_servers.split(','))
    ips = [socket.gethostbyname(name) for name in servers]

    login = False
    while True:
        try:
            utils.run('gcloud config set project %s' % project)
            utils.run('gcloud sql instances patch %s --authorized-networks %s'
                      % (instance, ','.join(ips)), stdout_tee=sys.stdout,
                      stderr_tee=sys.stderr)
            return
        except error.CmdError:
            if login:
                raise

            # Try to login and retry if the command failed.
            gcloud_login(project)
            login = True


def main():
    """main script."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', type=str, dest='project',
                        help='Name of the Google Cloud project.')
    parser.add_argument('--instance', type=str, dest='instance',
                        help='Name of the CloudSQL instance.')
    parser.add_argument('--afe', type=str, dest='afe',
                        help='Name of the RPC server to get server list.',
                        default=None)
    parser.add_argument('--extra_servers', type=str, dest='extra_servers',
                        help=('Extra servers to be included in the "Allowed '
                              'Networks" list separated by comma.'),
                        default=None)
    options = parser.parse_args()

    update_allowed_networks(options.project, options.instance, options.afe,
                            options.extra_servers)


if __name__ == '__main__':
    main()