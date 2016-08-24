#!/usr/bin/python

from __future__ import print_function

import argparse
import multiprocessing.pool
import subprocess
import sys

import common
from autotest_lib.server import frontend
from autotest_lib.site_utils.lib import infra

DEPLOY_PRODUCTION_LOCAL = ('/usr/local/autotest/site_utils/'
                           'deploy_production_local.py')
POOL_SIZE = 124
PUSH_ORDER = {'database': 0,
              'drone': 1,
              'shard': 1,
              'golo_proxy': 1,
              'afe': 2,
              'scheduler': 2,
              'host_scheduler': 2,
              'suite_scheduler': 2}


def discover_servers(afe, server_filter=set()):
    """Discover the in-production servers to update.

    @param afe: Server to contact with RPC requests.
    @param server_filter: A set of servers to get status for.

    @returns: A list of a list of tuple of (server_name, server_status, roles).
              The list is sorted by the order to be updated. Servers in the same
              sublist can be pushed together.

    """
    # Example server details....
    # {
    #     'hostname': 'server1',
    #     'status': 'backup',
    #     'roles': ['drone', 'scheduler'],
    #     'attributes': {'max_processes': 300}
    # }
    rpc = frontend.AFE(server=afe)
    servers = rpc.run('get_servers')

    # Do not update servers that need repair, and filter the server list by
    # given server_filter if needed.
    servers = [s for s in servers
               if (s['status'] != 'repair_required' and
                   (not server_filter or s['hostname'] in server_filter))]

    # Do not update reserve, devserver or crash_server (not YET supported).
    servers = [s for s in servers if 'devserver' not in s['roles'] and
               'crash_server' not in s['roles'] and
               'reserve' not in s['roles']]

    sorted_servers = []
    for i in range(max(PUSH_ORDER.values()) + 1):
        sorted_servers.append([])
    servers_with_unknown_order = []
    for server in servers:
        info = (server['hostname'], server['status'], server['roles'])
        try:
            order = min([PUSH_ORDER[r] for r in server['roles']
                         if r in PUSH_ORDER])
            sorted_servers[order].append(info)
        except ValueError:
            # All roles are not indexed in PUSH_ORDER.
            servers_with_unknown_order.append(info)

    # Push all servers with unknown roles together.
    if servers_with_unknown_order:
        sorted_servers.append(servers_with_unknown_order)

    found_servers = set([s['hostname'] for s in servers])
    # Inject the servers passed in by user but not found in server database.
    extra_servers = []
    for server in server_filter - found_servers:
        extra_servers.append((server, 'unknown', ['unknown']))
    if extra_servers:
        sorted_servers.append(extra_servers)

    return sorted_servers


def parse_arguments(args):
    """Parse command line arguments.

    @param args: The command line arguments to parse. (usually sys.argv[1:])

    @returns An argparse.Namespace populated with argument values.
    """
    parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description='Command to update an entire autotest installation.',
            epilog=('Update all servers:\n'
                    '  deploy_production.py\n'
                    '\n'
                    'Update one server:\n'
                    '  deploy_production.py <server>\n'
                    '\n'
                    'Send arguments to remote deploy_production_local.py:\n'
                    '  deploy_production.py -- --dryrun\n'
                    '\n'
                    'See what arguments would be run on specified servers:\n'
                    '  deploy_production.py --dryrun <server_a> <server_b> --'
                    ' --skip-update\n'))

    parser.add_argument('-v', '--verbose', action='store_true', dest='verbose',
            help='Log all deploy script output.')
    parser.add_argument('--continue', action='store_true', dest='cont',
            help='Continue to the next server on failure.')
    parser.add_argument('--afe', default='cautotest',
            help='What is the main server for this installation? (cautotest).')
    parser.add_argument('--dryrun', action='store_true',
            help='Don\'t actually run remote commands.')
    parser.add_argument('args', nargs=argparse.REMAINDER,
            help=('<server>, <server> ... -- <remote_arg>, <remote_arg> ...'))

    results = parser.parse_args(args)

    # We take the args list and further split it down. Everything before --
    # is a server name, and everything after it is an argument to pass along
    # to deploy_production_local.py.
    #
    # This:
    #   server_a, server_b -- --dryrun --skip-report
    #
    # Becomes:
    #   args.servers['server_a', 'server_b']
    #   args.args['--dryrun', '--skip-report']
    try:
        local_args_index = results.args.index('--') + 1
    except ValueError:
        # If -- isn't present, they are all servers.
        results.servers = results.args
        results.args = []
    else:
        # Split arguments.
        results.servers = results.args[:local_args_index-1]
        results.args = results.args[local_args_index:]

    return results


def update_server(inputs):
    """Deploy for given server.

    @param inputs: Inputs for the update action, including:
                   server: Name of the server to update.
                   status: Status of the server.
                   options: Options for the update.

    @return: A tuple of (server, success, output), where:
             server: Name of the server to be updated.
             sucess: True if update succeeds, False otherwise.
             output: A string of the deploy_production_local script output
                     including any errors.

    """
    server = inputs['server']
    status = inputs['status']
    options = inputs['options']
    print('Updating server %s...' % server)
    if status == 'backup':
        extra_args = ['--skip-service-status']
    else:
        extra_args = []

    cmd = ('%s %s' %
           (DEPLOY_PRODUCTION_LOCAL, ' '.join(options.args + extra_args)))
    output = '%s: %s' % (server, cmd)
    success = True
    if not options.dryrun:
        try:
            output = infra.execute_command(server, cmd)
        except subprocess.CalledProcessError as e:
            success = False
            output = e.output
    return server, success, output


def update_in_parallel(servers, options):
    """Update a group of servers in parallel.

    Exit the process with error if any server failed to be updated and
    options.cont is not set.

    @param servers: A list of tuple of (server_name, server_status, roles).
    @param options: Options for the push.

    """
    args = []
    for server, status, _ in servers:
        args.append({'server': server,
                     'status': status,
                     'options': options})
    # The update actions run in parallel. If any update failed, we should wait
    # for other running updates being finished. Abort in the middle of an update
    # may leave the server in a bad state.
    pool = multiprocessing.pool.ThreadPool(POOL_SIZE)
    failed_servers = []
    results = pool.imap_unordered(update_server, args)
    for server, success, output in results:
        if options.dryrun:
            print('Dry run, updating server %s is skipped.' % server)
        elif success:
            print('Successfully updated server %s.' % server)
            if options.verbose:
                print(output)
                print()
        else:
            error = ('Failed to update server %s.\nError: %s' %
                     (server, output))
            print(error)
            failed_servers.append(server)
    if failed_servers and not options.cont:
        print('Error! Failed to update following servers: %s' %
              failed_servers)
        sys.exit(1)


def update_group(servers, options):
    """Update a group of servers in parallel.

    Exit the process with error if any server failed to be updated and
    options.cont is not set.

    @param servers: A list of tuple of (server_name, server_status, roles).
    @param options: Options for the push.

    """
    # If it's allowed to continue updating even after some update fails, update
    # all servers together.
    if options.cont:
        update_in_parallel(servers, options)
        return

    # Pick on server per role in the group to update first. Abort if any update
    # failed.
    server_per_role = {}
    # Each server can be used to qualify only one role.
    server_picked = set()
    for server, status, roles in servers:
        for role in roles:
            if not role in server_per_role and not server in server_picked:
                server_per_role[role] = (server, status, roles)
                server_picked.add(server)
                break
    update_in_parallel(server_per_role.values(), options)

    rest_servers = [s for s in servers if not s[0] in server_picked]
    update_in_parallel(rest_servers, options)


def main(args):
    """Main routine that drives all the real work.

    @param args: The command line arguments to parse. (usually sys.argv[1:])

    @returns The system exit code.
    """
    options = parse_arguments(args)

    print('Retrieving server status...')
    sorted_servers = discover_servers(options.afe, set(options.servers or []))

    # Display what we plan to update.
    print('Will update (in this order):')
    i = 1
    for servers in sorted_servers:
        print('%s Group %d (%d servers) %s' % ('='*30, i, len(servers), '='*30))
        for server, status, roles in servers:
            print('\t%-36s:\t%s\t%s' % (server, status, roles))
        i += 1
    print()

    for servers in sorted_servers:
        update_group(servers, options)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
