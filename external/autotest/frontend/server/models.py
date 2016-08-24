# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Django model for server database.
"""

from django.db import models as dbmodels

import common
from autotest_lib.client.common_lib import enum
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.frontend.afe import model_logic


class Server(dbmodels.Model, model_logic.ModelExtensions):
    """Models a server."""
    DETAIL_FMT = ('Hostname     : %(hostname)s\n'
                  'Status       : %(status)s\n'
                  'Roles        : %(roles)s\n'
                  'Attributes   : %(attributes)s\n'
                  'Date Created : %(date_created)s\n'
                  'Date Modified: %(date_modified)s\n'
                  'Note         : %(note)s\n')

    STATUS_LIST = ['primary', 'backup', 'repair_required']
    STATUS = enum.Enum(*STATUS_LIST, string_values=True)

    hostname = dbmodels.CharField(unique=True, max_length=128)
    cname = dbmodels.CharField(null=True, blank=True, default=None,
                               max_length=128)
    status = dbmodels.CharField(unique=False, max_length=128,
                                choices=STATUS.choices())
    date_created = dbmodels.DateTimeField(null=True, blank=True)
    date_modified = dbmodels.DateTimeField(null=True, blank=True)
    note = dbmodels.TextField(null=True, blank=True)

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for class Server."""
        db_table = 'servers'


    def __unicode__(self):
        """A string representation of the Server object.
        """
        roles = ','.join([r.role for r in self.roles.all()])
        attributes = dict([(a.attribute, a.value)
                           for a in self.attributes.all()])
        return self.DETAIL_FMT % {'hostname': self.hostname,
                                  'status': self.status,
                                  'roles': roles,
                                  'attributes': attributes,
                                  'date_created': self.date_created,
                                  'date_modified': self.date_modified,
                                  'note': self.note}


    def get_role_names(self):
        """Get a list of role names of the server.

        @return: A list of role names of the server.
        """
        return [r.role for r in self.roles.all()]


    def get_details(self):
        """Get a dictionary with all server details.

        For example:
        {
            'hostname': 'server1',
            'status': 'backup',
            'roles': ['drone', 'scheduler'],
            'attributes': {'max_processes': 300}
        }

        @return: A dictionary with all server details.
        """
        details = {}
        details['hostname'] = self.hostname
        details['status'] = self.status
        details['roles'] = self.get_role_names()
        attributes = dict([(a.attribute, a.value)
                           for a in self.attributes.all()])
        details['attributes'] = attributes
        details['date_created'] = self.date_created
        details['date_modified'] = self.date_modified
        details['note'] = self.note
        return details


class ServerRole(dbmodels.Model, model_logic.ModelExtensions):
    """Role associated with hosts."""
    # Valid roles for a server.
    ROLE_LIST = ['afe', 'scheduler', 'host_scheduler', 'drone', 'devserver',
                 'database', 'database_slave', 'suite_scheduler',
                 'crash_server', 'shard', 'golo_proxy', 'reserve']
    ROLE = enum.Enum(*ROLE_LIST, string_values=True)
    # When deleting any of following roles from a primary server, a working
    # backup must be available if user_server_db is enabled in global config.
    ROLES_REQUIRE_BACKUP = [ROLE.SCHEDULER, ROLE.HOST_SCHEDULER,
                            ROLE.DATABASE, ROLE.SUITE_SCHEDULER,
                            ROLE.DRONE]
    # Roles that must be assigned to a single primary server in an Autotest
    # instance
    ROLES_REQUIRE_UNIQUE_INSTANCE = [ROLE.SCHEDULER,
                                     ROLE.HOST_SCHEDULER,
                                     ROLE.DATABASE,
                                     ROLE.SUITE_SCHEDULER]

    server = dbmodels.ForeignKey(Server, related_name='roles')
    role = dbmodels.CharField(max_length=128, choices=ROLE.choices())

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for the ServerRole class."""
        db_table = 'server_roles'


class ServerAttribute(dbmodels.Model, model_logic.ModelExtensions):
    """Attribute associated with hosts."""
    server = dbmodels.ForeignKey(Server, related_name='attributes')
    attribute = dbmodels.CharField(max_length=128)
    value = dbmodels.TextField(null=True, blank=True)
    date_modified = dbmodels.DateTimeField(null=True, blank=True)

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for the ServerAttribute class."""
        db_table = 'server_attributes'


# Valid values for each type of input.
RANGE_LIMITS={'role': ServerRole.ROLE_LIST,
              'status': Server.STATUS_LIST}

def validate(**kwargs):
    """Verify command line arguments, raise InvalidDataError if any is invalid.

    The function verify following inputs for the database query.
    1. Any key in RANGE_LIMITS, i.e., role and status. Value should be a valid
       role or status.
    2. hostname. The code will try to resolve given hostname. If the hostname
       does not exist in the network, InvalidDataError will be raised.
    Sample usage of this function:
    validate(role='drone', status='backup', hostname='server1')

    @param kwargs: command line arguments, e.g., `status='primary'`
    @raise InvalidDataError: If any argument value is invalid.
    """
    for key, value in kwargs.items():
        # Ignore any None value, so callers won't need to filter out None
        # value as it won't be used in queries.
        if not value:
            continue
        if value not in RANGE_LIMITS.get(key, [value]):
            raise error.InvalidDataError(
                    '%s %s is not valid, it must be one of %s.' %
                    (key, value,
                     ', '.join(RANGE_LIMITS[key])))
        elif key == 'hostname':
            if not ping_runner.PingRunner().simple_ping(value):
                raise error.InvalidDataError('Can not reach server with '
                                             'hostname "%s".' % value)
