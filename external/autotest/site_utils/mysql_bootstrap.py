#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Bootstrap mysql.

The purpose of this module is to grant access to a new-user/host/password
combination on a remote db server. For example, if we were bootstrapping
a new autotest master A1 with a remote database server A2, the scheduler
running on A1 needs to access the database on A2 with the credentials
specified in the shadow_config of A1 (A1_user, A1_pass). To achieve this
we ssh into A2 and execute the grant privileges command for (A1_user,
A1_pass, A1_host). If OTOH the db server is running locally we only need
to grant permissions for (A1_user, A1_pass, localhost).

The operation to achieve this will look like:
    ssh/become into A2
    Execute mysql -u <default_user> -p<default_pass> -e
        "GRANT privileges on <db> to 'A1_user'@A1 identified by 'A1_pass';"

However this will only grant the right access permissions to A1, so we need
to repeat for all subsequent db clients we add. This will happen through puppet.

In the case of a vagrant cluster, a remote vm cannot ssh into the db server
vm with plain old ssh. However, the entire vm cluster is provisioned at the
same time, so we can grant access to all remote vm clients directly on the
database server without knowing their ips by using the ip of the gateway.
This works because the db server vm redirects its database port (3306) to
a predefined port (defined in the vagrant file, defaults to 8002), and all
other vms in the cluster can only access it through the vm host identified
by the gateway.

The operation to achieve this will look like:
    Provision the vagrant db server
    Execute mysql -u <default_user> -p<default_pass> -e
        "GRANT privileges on <db> to 'A1_user'@(gateway address)
         identified by 'A1_pass';"
This will grant the right access permissions to all vms running on the
host machine as long as they use the right port to access the database.
"""

import argparse
import logging
import socket
import subprocess
import sys

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils.lib import infra


class MySQLCommandError(Exception):
    """Generic mysql command execution exception."""


class MySQLCommandExecutor(object):
    """Class to shell out to mysql.

    USE THIS CLASS WITH CARE. It doesn't protect against SQL injection on
    assumption that anyone with access to our servers can run the same
    commands directly instead of through this module. Do not expose it
    through a webserver, it is meant solely as a utility module to allow
    easy database bootstrapping via puppet.
    """

    DEFAULT_USER = global_config.global_config.get_config_value(
            'AUTOTEST_WEB', 'default_db_user', default='root')

    DEFAULT_PASS = global_config.global_config.get_config_value(
            'AUTOTEST_WEB', 'default_db_pass', default='autotest')


    @classmethod
    def mysql_cmd(cls, cmd, user=DEFAULT_USER, password=DEFAULT_PASS,
                  host='localhost', port=3306):
        """Wrap the given mysql command.

        @param cmd: The mysql command to wrap with the --execute option.
        @param host: The host against which to run the command.
        @param user: The user to use in the given command.
        @param password: The password for the user.
        @param port: The port mysql server is listening on.
        """
        return ('mysql -u %s -p%s --host %s --port %s -e "%s"' %
                (user, password, host, port, cmd))


    @staticmethod
    def execute(dest_server, full_cmd):
        """Execute a mysql statement on a remote server by sshing into it.

        @param dest_server: The hostname of the remote mysql server.
        @param full_cmd: The full mysql command to execute.

        @raises MySQLCommandError: If the full_cmd failed on dest_server.
        """
        try:
            return infra.execute_command(dest_server, full_cmd)
        except subprocess.CalledProcessError as e:
            raise MySQLCommandError('Failed to execute %s against %s' %
                                    (full_cmd, dest_server))


    @classmethod
    def ping(cls, db_server, user=DEFAULT_USER, password=DEFAULT_PASS,
             use_ssh=False):
        """Ping the given db server as 'user' using 'password'.

        @param db_server: The host running the mysql server.
        @param user: The user to use in the ping.
        @param password: The password of the user.
        @param use_ssh: If False, the command is executed on localhost
            by supplying --host=db_server in the mysql command. Otherwise we
            ssh/become into the db_server and execute the command with
            --host=localhost.

        @raises MySQLCommandError: If the ping command fails.
        """
        if use_ssh:
            ssh_dest_server = db_server
            mysql_cmd_host = 'localhost'
        else:
            ssh_dest_server = 'localhost'
            mysql_cmd_host = db_server
        ping = cls.mysql_cmd(
                'SELECT version();', host=mysql_cmd_host, user=user,
                password=password)
        cls.execute(ssh_dest_server, ping)


def bootstrap(user, password, source_host, dest_host):
    """Bootstrap the given user against dest_host.

    Allow a user from source_host to access the db server running on
    dest_host.

    @param user: The user to bootstrap.
    @param password: The password for the user.
    @param source_host: The host from which the new user will access the db.
    @param dest_host: The hostname of the remote db server.

    @raises MySQLCommandError: If we can't ping the db server using the default
        user/password specified in the shadow_config under default_db_*, or
        we can't ping it with the new credentials after bootstrapping.
    """
    # Confirm ssh/become access.
    try:
        infra.execute_command(dest_host, 'echo "hello"')
    except subprocess.CalledProcessError as e:
        logging.error("Cannot become/ssh into dest host. You need to bootstrap "
                      "it using fab -H <hostname> bootstrap from the "
                      "chromeos-admin repo.")
        return
    # Confirm the default user has at least database read privileges. Note if
    # the default user has *only* read privileges everything else will still
    # fail. This is a remote enough case given our current setup that we can
    # avoid more complicated checking at this level.
    MySQLCommandExecutor.ping(dest_host, use_ssh=True)

    # Prepare and execute the grant statement for the new user.
    creds = {
        'new_user': user,
        'new_pass': password,
        'new_host': source_host,
    }
    # TODO(beeps): Restrict these permissions. For now we have a couple of
    # databases which may/may-not exist on various roles that need refactoring.
    grant_privileges = (
        "GRANT ALL PRIVILEGES ON *.* to '%(new_user)s'@'%(new_host)s' "
        "IDENTIFIED BY '%(new_pass)s'; FLUSH PRIVILEGES;")
    MySQLCommandExecutor.execute(
            dest_host, MySQLCommandExecutor.mysql_cmd(grant_privileges % creds))

    # Confirm the new user can ping the remote database server from localhost.
    MySQLCommandExecutor.ping(
            dest_host, user=user, password=password, use_ssh=False)


def get_gateway():
    """Return the address of the default gateway.

    @raises: subprocess.CalledProcessError: If the address of the gateway
        cannot be determined via netstat.
    """
    cmd = 'netstat -rn | grep "^0.0.0.0 " | cut -d " " -f10 | head -1'
    try:
        return infra.execute_command('localhost', cmd).rstrip('\n')
    except subprocess.CalledProcessError as e:
        logging.error('Unable to get gateway: %s', e)
        raise


def _parse_args(args):
    parser = argparse.ArgumentParser(description='A script to bootstrap mysql '
                                     'with credentials from the shadow_config.')
    parser.add_argument(
            '--enable_gateway', action='store_true', dest='enable_gateway',
            default=False, help='Enable gateway access for vagrant testing.')
    return parser.parse_args(args)


def main(argv):
    """Main bootstrapper method.

    Grants permissions to the appropriate user on localhost, then enables the
    access through the gateway if --enable_gateway is specified.
    """
    args = _parse_args(argv)
    dest_host = global_config.global_config.get_config_value(
            'AUTOTEST_WEB', 'host')
    user = global_config.global_config.get_config_value(
            'AUTOTEST_WEB', 'user')
    password = global_config.global_config.get_config_value(
            'AUTOTEST_WEB', 'password')

    # For access via localhost, one needs to specify localhost as the hostname.
    # Neither the ip or the actual hostname of localhost will suffice in
    # mysql version 5.5, without complications.
    local_hostname = ('localhost' if utils.is_localhost(dest_host)
                      else socket.gethostname())
    logging.info('Bootstrapping user %s on host %s against db server %s',
                 user, local_hostname, dest_host)
    bootstrap(user, password, local_hostname, dest_host)

    if args.enable_gateway:
        gateway = get_gateway()
        logging.info('Enabling access through gateway %s', gateway)
        bootstrap(user, password, gateway, dest_host)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
