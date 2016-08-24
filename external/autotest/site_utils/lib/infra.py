# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import getpass
import subprocess
import os

import common
from autotest_lib.server.hosts import ssh_host
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


@contextlib.contextmanager
def chdir(dirname=None):
    """A context manager to help change directories.

    Will chdir into the provided dirname for the lifetime of the context and
    return to cwd thereafter.

    @param dirname: The dirname to chdir into.
    """
    curdir = os.getcwd()
    try:
        if dirname is not None:
            os.chdir(dirname)
        yield
    finally:
        os.chdir(curdir)


def local_runner(cmd, stream_output=False):
    """
    Runs a command on the local system as the current user.

    @param cmd: The command to run.
    @param stream_output: If True, streams the stdout of the process.

    @returns: The output of cmd.
    @raises CalledProcessError: If there was a non-0 return code.
    """
    if not stream_output:
        return subprocess.check_output(cmd, shell=True)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    while proc.poll() is None:
        print proc.stdout.readline().rstrip('\n')


_host_objects = {}

def host_object_runner(host, **kwargs):
    """
    Returns a function that returns the output of running a command via a host
    object.

    @param host: The host to run a command on.
    @returns: A function that can invoke a command remotely.
    """
    try:
        host_object = _host_objects[host]
    except KeyError:
        username = global_config.global_config.get_config_value(
                'CROS', 'infrastructure_user')
        host_object = ssh_host.SSHHost(host, user=username)
        _host_objects[host] = host_object

    def runner(cmd):
        """
        Runs a command via a host object on the enclosed host.  Translates
        host.run errors to the subprocess equivalent to expose a common API.

        @param cmd: The command to run.
        @returns: The output of cmd.
        @raises CalledProcessError: If there was a non-0 return code.
        """
        try:
            return host_object.run(cmd).stdout
        except error.AutotestHostRunError as e:
            exit_status = e.result_obj.exit_status
            command = e.result_obj.command
            raise subprocess.CalledProcessError(exit_status, command)
    return runner


def googlesh_runner(host, **kwargs):
    """
    Returns a function that return the output of running a command via shelling
    out to `googlesh`.

    @param host: The host to run a command on
    @returns: A function that can invoke a command remotely.
    """
    def runner(cmd):
        """
        Runs a command via googlesh on the enclosed host.

        @param cmd: The command to run.
        @returns: The output of cmd.
        @raises CalledProcessError: If there was a non-0 return code.
        """
        out = subprocess.check_output(['googlesh', '-s', '-uchromeos-test',
                                       '-m%s' % host, '%s' % cmd])
        return out
    return runner


def execute_command(host, cmd, **kwargs):
    """
    Executes a command on the host `host`.  This an optimization that if
    we're already chromeos-test, we can just ssh to the machine in question.
    Or if we're local, we don't have to ssh at all.

    @param host: The hostname to execute the command on.
    @param cmd: The command to run.  Special shell syntax (such as pipes)
                is allowed.
    @param kwargs: Key word arguments for the runner functions.
    @returns: The output of the command.
    """
    if utils.is_localhost(host):
        runner = local_runner
    elif getpass.getuser() == 'chromeos-test':
        runner = host_object_runner(host)
    else:
        runner = googlesh_runner(host)

    return runner(cmd, **kwargs)


def _csv_to_list(s):
    """
    Converts a list seperated by commas into a list of strings.

    >>> _csv_to_list('')
    []
    >>> _csv_to_list('one')
    ['one']
    >>> _csv_to_list('one, two,three')
    ['one', 'two', 'three']
    """
    return [x.strip() for x in s.split(',') if x]


# The goal with these functions is to give you a list of hosts that are valid
# arguments to ssh.  Note that this only really works since our instances use
# names that are findable by our default /etc/resolv.conf `search` domains,
# because all of our instances have names under .corp
def sam_servers():
    """
    Generate a list of all scheduler/afe instances of autotest.

    Note that we don't include the mysql database host if the database is split
    from the rest of the system.
    """
    sams_config = global_config.global_config.get_config_value(
            'CROS', 'sam_instances', default='')
    sams = _csv_to_list(sams_config)
    return set(sams)


def extra_servers():
    """
    Servers that have an autotest checkout in /usr/local/autotest, but aren't
    in any other list.

    @returns: A set of hosts.
    """
    servers = global_config.global_config.get_config_value(
                'CROS', 'extra_servers', default='')
    return set(_csv_to_list(servers))


def test_instance():
    """
    A server that is set up to run tests of the autotest infrastructure.

    @returns: A hostname
    """
    server = global_config.global_config.get_config_value(
                'CROS', 'test_instance', default='')
    return server


# The most reliable way to pull information about the state of the lab is to
# look at the global/shadow config on each server.  The best way to do this is
# via the global_config module.  Therefore, we invoke python on the remote end
# to call global_config to get whatever values we want.
_VALUE_FROM_CONFIG = '''
cd /usr/local/autotest
python -c "
import common
from autotest_lib.client.common_lib import global_config
print global_config.global_config.get_config_value(
  '%s', '%s', default='')
"
'''
# There's possibly cheaper ways to do some of this, for example, we could scrape
# instance:13467 for the list of drones, but this way you can get the list of
# drones that is what should/will be running, and not what the scheduler thinks
# is running.  (It could have kicked one out, or we could be bringing a new one
# into rotation.)  So scraping the config on remote servers, while slow, gives
# us consistent logical results.


def _scrape_from_instances(section, key):
    sams = sam_servers()
    all_servers = set()
    for sam in sams:
        servers_csv = execute_command(sam, _VALUE_FROM_CONFIG % (section, key))
        servers = _csv_to_list(servers_csv)
        for server in servers:
            if server == 'localhost':
                all_servers.add(sam)
            else:
                all_servers.add(server)
    return all_servers


def database_servers():
    """
    Generate a list of all database servers running for instances of autotest.

    @returns: An iterable of all hosts.
    """
    return _scrape_from_instances('AUTOTEST_WEB', 'host')


def drone_servers():
    """
    Generate a list of all drones used by all instances of autotest in
    production.

    @returns: An iterable of drone servers.
    """
    return _scrape_from_instances('SCHEDULER', 'drones')


def devserver_servers():
    """
    Generate a list of all devservers.

    @returns: An iterable of all hosts.
    """
    zone = global_config.global_config.get_config_value(
            'CLIENT', 'dns_zone')
    servers = _scrape_from_instances('CROS', 'dev_server_hosts')
    # The default text we get back here isn't something you can ssh into unless
    # you've set up your /etc/resolve.conf to automatically try .cros, so we
    # append the zone to try and make this more in line with everything else.
    return set([server+'.'+zone for server in servers])


def shard_servers():
    """
    Generate a list of all shard servers.

    @returns: An iterable of all shard servers.
    """
    shard_hostnames = set()
    sams = sam_servers()
    for sam in sams:
        afe = frontend_wrappers.RetryingAFE(server=sam)
        shards = afe.run('get_shards')
        for shard in shards:
            shard_hostnames.add(shard['hostname'])

    return list(shard_hostnames)
