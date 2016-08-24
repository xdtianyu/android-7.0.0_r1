#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs on autotest servers from a cron job to self update them.

This script is designed to run on all autotest servers to allow them to
automatically self-update based on the manifests used to create their (existing)
repos.
"""

from __future__ import print_function

import ConfigParser
import argparse
import os
import re
import subprocess
import sys
import time

import common

from autotest_lib.client.common_lib import global_config

# How long after restarting a service do we watch it to see if it's stable.
SERVICE_STABILITY_TIMER = 120


class DirtyTreeException(Exception):
    """Raised when the tree has been modified in an unexpected way."""


class UnknownCommandException(Exception):
    """Raised when we try to run a command name with no associated command."""


class UnstableServices(Exception):
    """Raised if a service appears unstable after restart."""


def strip_terminal_codes(text):
    """This function removes all terminal formatting codes from a string.

    @param text: String of text to cleanup.
    @returns String with format codes removed.
    """
    ESC = '\x1b'
    return re.sub(ESC+r'\[[^m]*m', '', text)


def verify_repo_clean():
    """This function verifies that the current repo is valid, and clean.

    @raises DirtyTreeException if the repo is not clean.
    @raises subprocess.CalledProcessError on a repo command failure.
    """
    out = subprocess.check_output(['repo', 'status'], stderr=subprocess.STDOUT)
    out = strip_terminal_codes(out).strip()

    CLEAN_STATUS_OUTPUT = 'nothing to commit (working directory clean)'
    if out != CLEAN_STATUS_OUTPUT:
      raise DirtyTreeException(out)


def repo_versions():
    """This function collects the versions of all git repos in the general repo.

    @returns A dictionary mapping project names to git hashes for HEAD.
    @raises subprocess.CalledProcessError on a repo command failure.
    """
    cmd = ['repo', 'forall', '-p', '-c', 'pwd && git log -1 --format=%h']
    output = strip_terminal_codes(subprocess.check_output(cmd))

    # The expected output format is:

    # project chrome_build/
    # /dir/holding/chrome_build
    # 73dee9d
    #
    # project chrome_release/
    # /dir/holding/chrome_release
    # 9f3a5d8

    lines = output.splitlines()

    PROJECT_PREFIX = 'project '

    project_heads = {}
    for n in range(0, len(lines), 4):
        project_line = lines[n]
        project_dir = lines[n+1]
        project_hash = lines[n+2]
        # lines[n+3] is a blank line, but doesn't exist for the final block.

        # Convert 'project chrome_build/' -> 'chrome_build'
        assert project_line.startswith(PROJECT_PREFIX)
        name = project_line[len(PROJECT_PREFIX):].rstrip('/')

        project_heads[name] = (project_dir, project_hash)

    return project_heads


def repo_sync():
    """Perform a repo sync.

    @raises subprocess.CalledProcessError on a repo command failure.
    """
    subprocess.check_output(['repo', 'sync'])


def discover_update_commands():
    """Lookup the commands to run on this server.

    These commonly come from shadow_config.ini, since they vary by server type.

    @returns List of command names in string format.
    """
    try:
        return global_config.global_config.get_config_value(
                'UPDATE', 'commands', type=list)

    except (ConfigParser.NoSectionError, global_config.ConfigError):
        return []


def discover_restart_services():
    """Find the services that need restarting on the current server.

    These commonly come from shadow_config.ini, since they vary by server type.

    @returns List of service names in string format.
    """
    try:
        # From shadow_config.ini, lookup which services to restart.
        return global_config.global_config.get_config_value(
                'UPDATE', 'services', type=list)

    except (ConfigParser.NoSectionError, global_config.ConfigError):
        return []


def update_command(cmd_tag, dryrun=False):
    """Restart a command.

    The command name is looked up in global_config.ini to find the full command
    to run, then it's executed.

    @param cmd_tag: Which command to restart.
    @param dryrun: If true print the command that would have been run.

    @raises UnknownCommandException If cmd_tag can't be looked up.
    @raises subprocess.CalledProcessError on a command failure.
    """
    # Lookup the list of commands to consider. They are intended to be
    # in global_config.ini so that they can be shared everywhere.
    cmds = dict(global_config.global_config.config.items(
        'UPDATE_COMMANDS'))

    if cmd_tag not in cmds:
        raise UnknownCommandException(cmd_tag, cmds)

    expanded_command = cmds[cmd_tag].replace('AUTOTEST_REPO',
                                              common.autotest_dir)

    print('Running: %s: %s' % (cmd_tag, expanded_command))
    if dryrun:
        print('Skip: %s' % expanded_command)
    else:
        try:
            subprocess.check_output(expanded_command, shell=True,
                                    stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as e:
            print('FAILED:')
            print(e.output)
            raise


def restart_service(service_name, dryrun=False):
    """Restart a service.

    Restarts the standard service with "service <name> restart".

    @param service_name: The name of the service to restart.
    @param dryrun: Don't really run anything, just print out the command.

    @raises subprocess.CalledProcessError on a command failure.
    """
    cmd = ['sudo', 'service', service_name, 'restart']
    print('Restarting: %s' % service_name)
    if dryrun:
        print('Skip: %s' % ' '.join(cmd))
    else:
        subprocess.check_call(cmd)


def service_status(service_name):
    """Return the results "status <name>" for a given service.

    This string is expected to contain the pid, and so to change is the service
    is shutdown or restarted for any reason.

    @param service_name: The name of the service to check on.

    @returns The output of the external command.
             Ex: autofs start/running, process 1931

    @raises subprocess.CalledProcessError on a command failure.
    """
    return subprocess.check_output(['sudo', 'status', service_name])


def restart_services(service_names, dryrun=False, skip_service_status=False):
    """Restart services as needed for the current server type.

    Restart the listed set of services, and watch to see if they are stable for
    at least SERVICE_STABILITY_TIMER. It restarts all services quickly,
    waits for that delay, then verifies the status of all of them.

    @param service_names: The list of service to restart and monitor.
    @param dryrun: Don't really restart the service, just print out the command.
    @param skip_service_status: Set to True to skip service status check.
                                Default is False.

    @raises subprocess.CalledProcessError on a command failure.
    @raises UnstableServices if any services are unstable after restart.
    """
    service_statuses = {}

    if dryrun:
        for name in service_names:
            restart_service(name, dryrun=True)
        return

    # Restart each, and record the status (including pid).
    for name in service_names:
        restart_service(name)
        service_statuses[name] = service_status(name)

    # Skip service status check if --skip-service-status is specified. Used for
    # servers in backup status.
    if skip_service_status:
        print('--skip-service-status is specified, skip checking services.')
        return

    # Wait for a while to let the services settle.
    time.sleep(SERVICE_STABILITY_TIMER)

    # Look for any services that changed status.
    unstable_services = [n for n in service_names
                         if service_status(n) != service_statuses[n]]

    # Report any services having issues.
    if unstable_services:
        raise UnstableServices(unstable_services)


def run_deploy_actions(dryrun=False, skip_service_status=False):
    """Run arbitrary update commands specified in global.ini.

    @param dryrun: Don't really restart the service, just print out the command.
    @param skip_service_status: Set to True to skip service status check.
                                Default is False.

    @raises subprocess.CalledProcessError on a command failure.
    @raises UnstableServices if any services are unstable after restart.
    """
    cmds = discover_update_commands()
    if cmds:
        print('Running update commands:', ', '.join(cmds))
        for cmd in cmds:
            update_command(cmd, dryrun=dryrun)

    services = discover_restart_services()
    if services:
        print('Restarting Services:', ', '.join(services))
        restart_services(services, dryrun=dryrun,
                         skip_service_status=skip_service_status)


def report_changes(versions_before, versions_after):
    """Produce a report describing what changed in all repos.

    @param versions_before: Results of repo_versions() from before the update.
    @param versions_after: Results of repo_versions() from after the update.

    @returns string containing a human friendly changes report.
    """
    result = []

    if versions_after:
        for project in sorted(set(versions_before.keys() + versions_after.keys())):
            result.append('%s:' % project)

            _, before_hash = versions_before.get(project, (None, None))
            after_dir, after_hash = versions_after.get(project, (None, None))

            if project not in versions_before:
                result.append('Added.')

            elif project not in versions_after:
                result.append('Removed.')

            elif before_hash == after_hash:
                result.append('No Change.')

            else:
                hashes = '%s..%s' % (before_hash, after_hash)
                cmd = ['git', 'log', hashes, '--oneline']
                out = subprocess.check_output(cmd, cwd=after_dir,
                                              stderr=subprocess.STDOUT)
                result.append(out.strip())

            result.append('')
    else:
        for project in sorted(versions_before.keys()):
            _, before_hash = versions_before[project]
            result.append('%s: %s' % (project, before_hash))
        result.append('')

    return '\n'.join(result)


def parse_arguments(args):
    """Parse command line arguments.

    @param args: The command line arguments to parse. (ususally sys.argsv[1:])

    @returns An argparse.Namespace populated with argument values.
    """
    parser = argparse.ArgumentParser(
            description='Command to update an autotest server.')
    parser.add_argument('--skip-verify', action='store_false',
                        dest='verify', default=True,
                        help='Disable verification of a clean repository.')
    parser.add_argument('--skip-update', action='store_false',
                        dest='update', default=True,
                        help='Skip the repository source code update.')
    parser.add_argument('--skip-actions', action='store_false',
                        dest='actions', default=True,
                        help='Skip the post update actions.')
    parser.add_argument('--skip-report', action='store_false',
                        dest='report', default=True,
                        help='Skip the git version report.')
    parser.add_argument('--actions-only', action='store_true',
                        help='Run the post update actions (restart services).')
    parser.add_argument('--dryrun', action='store_true',
                        help='Don\'t actually run any commands, just log.')
    parser.add_argument('--skip-service-status', action='store_true',
                        help='Skip checking the service status.')

    results = parser.parse_args(args)

    if results.actions_only:
        results.verify = False
        results.update = False
        results.report = False

    # TODO(dgarrett): Make these behaviors support dryrun.
    if results.dryrun:
        results.verify = False
        results.update = False

    return results


def main(args):
    """Main method."""
    os.chdir(common.autotest_dir)
    global_config.global_config.parse_config_file()

    behaviors = parse_arguments(args)

    if behaviors.verify:
        try:
            print('Checking tree status:')
            verify_repo_clean()
            print('Clean.')
        except DirtyTreeException as e:
            print('Local tree is dirty, can\'t perform update safely.')
            print()
            print('repo status:')
            print(e.args[0])
            return 1

    versions_before = repo_versions()
    versions_after = {}

    if behaviors.update:
        print('Updating Repo.')
        repo_sync()
        versions_after = repo_versions()

    if behaviors.actions:
        try:
            run_deploy_actions(
                    dryrun=behaviors.dryrun,
                    skip_service_status=behaviors.skip_service_status)
        except UnstableServices as e:
            print('The following services were not stable after '
                  'the update:')
            print(e.args[0])
            return 1

    if behaviors.report:
        print('Changes:')
        print(report_changes(versions_before, versions_after))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
