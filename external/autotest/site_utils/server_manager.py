# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides functions to manage servers in server database
(defined in global config section AUTOTEST_SERVER_DB).

create(hostname, role=None, note=None)
    Create a server with given role, with status backup.

delete(hostname)
    Delete a server from the database. If the server is in primary status, its
    roles will be replaced by a backup server first.

modify(hostname, role=None, status=None, note=None, delete=False,
       attribute=None, value=None)
    Modify a server's role, status, note, or attribute:
    1. Add role to a server. If the server is in primary status, proper actions
       like service restart will be executed to enable the role.
    2. Delete a role from a server. If the server is in primary status, proper
       actions like service restart will be executed to disable the role.
    3. Change status of a server. If the server is changed from or to primary
       status, proper actions like service restart will be executed to enable
       or disable each role of the server.
    4. Change note of a server. Note is a field you can add description about
       the server.
    5. Change/delete attribute of a server. Attribute can be used to store
       information about a server. For example, the max_processes count for a
       drone.

"""


import datetime

import common

from autotest_lib.frontend.server import models as server_models
from autotest_lib.site_utils import server_manager_actions
from autotest_lib.site_utils import server_manager_utils


def _add_role(server, role, action):
    """Add a role to the server.

    @param server: An object of server_models.Server.
    @param role: Role to be added to the server.
    @param action: Execute actions after role or status is changed. Default to
                   False.

    @raise ServerActionError: If role is failed to be added.
    """
    server_models.validate(role=role)
    if role in server.get_role_names():
        raise server_manager_utils.ServerActionError(
                'Server %s already has role %s.' % (server.hostname, role))

    # Verify server
    if not server_manager_utils.check_server(server.hostname, role):
        raise server_manager_utils.ServerActionError(
                'Server %s is not ready for role %s.' % (server.hostname, role))

    if (role in server_models.ServerRole.ROLES_REQUIRE_UNIQUE_INSTANCE and
        server.status == server_models.Server.STATUS.PRIMARY):
        servers = server_models.Server.objects.filter(
                roles__role=role, status=server_models.Server.STATUS.PRIMARY)
        if len(servers) >= 1:
            raise server_manager_utils.ServerActionError(
                'Role %s must be unique. Server %s already has role %s.' %
                (role, servers[0].hostname, role))

    server_models.ServerRole.objects.create(server=server, role=role)

    # If needed, apply actions to enable the role for the server.
    server_manager_actions.try_execute(server, [role], enable=True,
                                       post_change=True, do_action=action)

    print 'Role %s is added to server %s.' % (role, server.hostname)


def _delete_role(server, role, action=False):
    """Delete a role from the server.

    @param server: An object of server_models.Server.
    @param role: Role to be deleted from the server.
    @param action: Execute actions after role or status is changed. Default to
                   False.

    @raise ServerActionError: If role is failed to be deleted.
    """
    server_models.validate(role=role)
    if role not in server.get_role_names():
        raise server_manager_utils.ServerActionError(
                'Server %s does not have role %s.' % (server.hostname, role))

    if server.status == server_models.Server.STATUS.PRIMARY:
        server_manager_utils.warn_missing_role(role, server)

    # Apply actions to disable the role for the server before the role is
    # removed from the server.
    server_manager_actions.try_execute(server, [role], enable=False,
                                       post_change=False, do_action=action)

    print 'Deleting role %s from server %s...' % (role, server.hostname)
    server.roles.get(role=role).delete()

    # Apply actions to disable the role for the server after the role is
    # removed from the server.
    server_manager_actions.try_execute(server, [role], enable=False,
                                       post_change=True, do_action=action)

    # If the server is in status primary and has no role, change its status to
    # backup.
    if (not server.get_role_names() and
        server.status == server_models.Server.STATUS.PRIMARY):
        print ('Server %s has no role, change its status from primary to backup'
               % server.hostname)
        server.status = server_models.Server.STATUS.BACKUP
        server.save()

    print 'Role %s is deleted from server %s.' % (role, server.hostname)


def _change_status(server, status, action):
    """Change the status of the server.

    @param server: An object of server_models.Server.
    @param status: New status of the server.
    @param action: Execute actions after role or status is changed. Default to
                   False.

    @raise ServerActionError: If status is failed to be changed.
    """
    server_models.validate(status=status)
    if server.status == status:
        raise server_manager_utils.ServerActionError(
                'Server %s already has status of %s.' %
                (server.hostname, status))
    if (not server.roles.all() and
        status == server_models.Server.STATUS.PRIMARY):
        raise server_manager_utils.ServerActionError(
                'Server %s has no role associated. Server must have a role to '
                'be in status primary.' % server.hostname)

    # Abort the action if the server's status will be changed to primary and
    # the Autotest instance already has another server running an unique role.
    # For example, a scheduler server is already running, and a backup server
    # with role scheduler should not be changed to status primary.
    unique_roles = server.roles.filter(
            role__in=server_models.ServerRole.ROLES_REQUIRE_UNIQUE_INSTANCE)
    if unique_roles and status == server_models.Server.STATUS.PRIMARY:
        for role in unique_roles:
            servers = server_models.Server.objects.filter(
                    roles__role=role.role,
                    status=server_models.Server.STATUS.PRIMARY)
            if len(servers) == 1:
                raise server_manager_utils.ServerActionError(
                        'Role %s must be unique. Server %s already has the '
                        'role.' % (role.role, servers[0].hostname))

    # Post a warning if the server's status will be changed from primary to
    # other value and the server is running a unique role across database, e.g.
    # scheduler.
    if server.status == server_models.Server.STATUS.PRIMARY:
        for role in server.get_role_names():
            server_manager_utils.warn_missing_role(role, server)

    enable = status == server_models.Server.STATUS.PRIMARY
    server_manager_actions.try_execute(server, server.get_role_names(),
                                       enable=enable, post_change=False,
                                       do_action=action)

    prev_status = server.status
    server.status = status
    server.save()

    # Apply actions to enable/disable roles of the server after the status is
    # changed.
    server_manager_actions.try_execute(server, server.get_role_names(),
                                       enable=enable, post_change=True,
                                       prev_status=prev_status,
                                       do_action=action)

    print ('Status of server %s is changed from %s to %s. Affected roles: %s' %
           (server.hostname, prev_status, status,
            ', '.join(server.get_role_names())))


@server_manager_utils.verify_server(exist=False)
def create(hostname, role=None, note=None):
    """Create a new server.

    The status of new server will always be backup, user need to call
    atest server modify hostname --status primary
    to set the server's status to primary.

    @param hostname: hostname of the server.
    @param role: role of the new server, default to None.
    @param note: notes about the server, default to None.

    @return: A Server object that contains the server information.
    """
    server_models.validate(hostname=hostname, role=role)
    server = server_models.Server.objects.create(
            hostname=hostname, status=server_models.Server.STATUS.BACKUP,
            note=note, date_created=datetime.datetime.now())
    server_models.ServerRole.objects.create(server=server, role=role)
    return server


@server_manager_utils.verify_server()
def delete(hostname, server=None):
    """Delete given server from server database.

    @param hostname: hostname of the server to be deleted.
    @param server: Server object from database query, this argument should be
                   injected by the verify_server_exists decorator.

    @raise ServerActionError: If delete server action failed, e.g., server is
            not found in database or server is primary but no backup is found.
    """
    print 'Deleting server %s from server database.' % hostname

    if (server_manager_utils.use_server_db() and
        server.status == server_models.Server.STATUS.PRIMARY):
        print ('Server %s is in status primary, need to disable its '
               'current roles first.' % hostname)
        for role in server.roles.all():
            _delete_role(server, role.role)

    server.delete()
    print 'Server %s is deleted from server database.' % hostname


@server_manager_utils.verify_server()
def modify(hostname, role=None, status=None, delete=False, note=None,
           attribute=None, value=None, action=False, server=None):
    """Modify given server with specified actions.

    @param hostname: hostname of the server to be modified.
    @param role: Role to be added to the server.
    @param status: Modify server status.
    @param delete: True to delete given role from the server, default to False.
    @param note: Note of the server.
    @param attribute: Name of an attribute of the server.
    @param value: Value of an attribute of the server.
    @param action: Execute actions after role or status is changed. Default to
                   False.
    @param server: Server object from database query, this argument should be
                   injected by the verify_server_exists decorator.

    @raise InvalidDataError: If the operation failed with any wrong value of
                             the arguments.
    @raise ServerActionError: If any operation failed.
    """
    if role:
        if not delete:
            _add_role(server, role, action)
        else:
            _delete_role(server, role, action)

    if status:
        _change_status(server, status, action)

    if note is not None:
        server.note = note
        server.save()

    if attribute and value:
        server_manager_utils.change_attribute(server, attribute, value)
    elif attribute and delete:
        server_manager_utils.delete_attribute(server, attribute)

    return server
