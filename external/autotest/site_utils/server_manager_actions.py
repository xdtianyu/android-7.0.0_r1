# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides utility functions to help managing servers in server
database (defined in global config section AUTOTEST_SERVER_DB).

After a role is added or removed from a server, certain services may need to
be restarted. For example, scheduler needs to be restarted after a drone is
added to a primary server. This module includes functions to check if actions
are required to be executed and what actions to executed on which servers.
"""

import subprocess
import sys

import common

from autotest_lib.frontend.server import models as server_models
from autotest_lib.site_utils import server_manager_utils
from autotest_lib.site_utils.lib import infra


# Actions that must be executed for server management action to be effective.
# Each action is a tuple:
# (the role of which the command should be executed, the command)
RESTART_SCHEDULER = (server_models.ServerRole.ROLE.SCHEDULER,
                     'sudo service scheduler restart')
RESTART_HOST_SCHEDULER = (server_models.ServerRole.ROLE.HOST_SCHEDULER,
                          'sudo service host-scheduler restart')
RESTART_SUITE_SCHEDULER = (server_models.ServerRole.ROLE.SUITE_SCHEDULER,
                           'sudo service suite_scheduler restart')
RELOAD_APACHE = (server_models.ServerRole.ROLE.SCHEDULER,
                 'sudo service apache reload')

STOP_SCHEDULER = (server_models.ServerRole.ROLE.SCHEDULER,
                  'sudo service scheduler stop')
STOP_HOST_SCHEDULER = (server_models.ServerRole.ROLE.HOST_SCHEDULER,
                       'sudo service host-scheduler stop')
STOP_SUITE_SCHEDULER = (server_models.ServerRole.ROLE.SUITE_SCHEDULER,
                        'sudo service suite_scheduler stop')

# Dictionary of actions needed for a role to be enabled. Key is the role, and
# value is a list of action. All these actions should be applied after the role
# is added to the server, or the server's status is changed to primary.
ACTIONS_AFTER_ROLE_APPLIED = {
        server_models.ServerRole.ROLE.SCHEDULER: [RESTART_SCHEDULER],
        server_models.ServerRole.ROLE.HOST_SCHEDULER: [RESTART_HOST_SCHEDULER],
        server_models.ServerRole.ROLE.SUITE_SCHEDULER:
                [RESTART_SUITE_SCHEDULER],
        server_models.ServerRole.ROLE.DRONE: [RESTART_SCHEDULER],
        server_models.ServerRole.ROLE.DATABASE:
                [RESTART_SCHEDULER, RESTART_HOST_SCHEDULER, RELOAD_APACHE],
        server_models.ServerRole.ROLE.DEVSERVER: [RESTART_SCHEDULER],
        }

# Dictionary of actions needed for a role to be disabled. Key is the role, and
# value is a list of action.
# Action should be taken before role is deleted from a server, or the server's
# status is changed to primary.
ACTIONS_BEFORE_ROLE_REMOVED = {
        server_models.ServerRole.ROLE.SCHEDULER: [STOP_SCHEDULER],
        server_models.ServerRole.ROLE.HOST_SCHEDULER: [STOP_HOST_SCHEDULER],
        server_models.ServerRole.ROLE.SUITE_SCHEDULER: [STOP_SUITE_SCHEDULER],
        server_models.ServerRole.ROLE.DATABASE:
                [STOP_SCHEDULER, STOP_HOST_SCHEDULER],
        }
# Action should be taken after role is deleted from a server, or the server's
# status is changed to primary.
ACTIONS_AFTER_ROLE_REMOVED = {
        server_models.ServerRole.ROLE.DRONE: [RESTART_SCHEDULER],
        server_models.ServerRole.ROLE.DEVSERVER: [RESTART_SCHEDULER],
        }


def apply(action):
    """Apply an given action.

    It usually involves ssh to the server with specific role and run the
    command, e.g., ssh to scheduler server and restart scheduler.

    @param action: A tuple of (the role of which the command should be executed,
                   the command)
    @raise ServerActionError: If the action can't be applied due to database
                              issue.
    @param subprocess.CalledProcessError: If command is failed to be
                                          executed.
    """
    role = action[0]
    command = action[1]
    # Find the servers with role
    servers = server_manager_utils.get_servers(
            role=role, status=server_models.Server.STATUS.PRIMARY)
    if not servers:
        print >> sys.stderr, ('WARNING! Action %s failed to be applied. No '
                              'server with given role %s was found.' %
                              (action, role))
        return

    for server in servers:
        print 'Run command `%s` on server %s' % (command, server.hostname)
        try:
            infra.execute_command(server.hostname, command)
        except subprocess.CalledProcessError as e:
            print >> sys.stderr, ('Failed to check server %s, error: %s' %
                                  (server.hostname, e))


def try_execute(server, roles, enable, post_change,
                prev_status=server_models.Server.STATUS.BACKUP,
                do_action=False):
    """Try to execute actions for given role changes of the server.

    @param server: Server that has the role changes.
    @param roles: A list of roles changed.
    @param enable: Set to True if the roles are enabled, i.e., added to server.
                   If it's False, the roles are removed from the server.
    @param post_change: Set to True if to apply actions should be applied after
                        the role changes, otherwise, set to False.
    @param prev_status: The previous status after the status change if any. This
                        is to help to decide if actions should be executed,
                        since actions should be applied if the server's status
                        is changed from primary to other status. Default to
                        backup.
    @param do_action: Set to True to execute actions, otherwise, post a warning.
    """
    if not server_manager_utils.use_server_db():
        return
    # This check is to prevent actions to be applied to server not in primary
    # role or server database is not enabled. Note that no action is needed
    # before a server is changed to primary status. If that assumption is
    # no longer valid, this method needs to be updated accordingly.
    if (server.status != server_models.Server.STATUS.PRIMARY and
        prev_status != server_models.Server.STATUS.PRIMARY):
        return

    if enable:
        if post_change:
            possible_actions = ACTIONS_AFTER_ROLE_APPLIED
    else:
        if post_change:
            possible_actions = ACTIONS_AFTER_ROLE_REMOVED
        else:
            possible_actions = ACTIONS_BEFORE_ROLE_REMOVED

    all_actions = []
    for role in roles:
        all_actions.extend(possible_actions.get(role, []))
    for action in set(all_actions):
        if do_action:
            apply(action)
        else:
            message = ('WARNING! Action %s is skipped. Please manually '
                       'execute the action to make your change effective.' %
                       str(action))
            print >> sys.stderr, message
