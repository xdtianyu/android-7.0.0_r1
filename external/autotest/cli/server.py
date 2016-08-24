# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
The server module contains the objects and methods used to manage servers in
Autotest.

The valid actions are:
list:      list all servers in the database
create:    create a server
delete:    deletes a server
modify:    modify a server's role or status.

The common options are:
--role / -r:     role that's related to server actions.

See topic_common.py for a High Level Design and Algorithm.
"""

import common

from autotest_lib.cli import action_common
from autotest_lib.cli import topic_common
from autotest_lib.client.common_lib import error
# The django setup is moved here as test_that uses sqlite setup. If this line
# is in server_manager, test_that unittest will fail.
from autotest_lib.frontend import setup_django_environment
from autotest_lib.site_utils import server_manager
from autotest_lib.site_utils import server_manager_utils


class server(topic_common.atest):
    """Server class

    atest server [list|create|delete|modify] <options>
    """
    usage_action = '[list|create|delete|modify]'
    topic = msg_topic = 'server'
    msg_items = '<server>'

    def __init__(self, hostname_required=True):
        """Add to the parser the options common to all the server actions.

        @param hostname_required: True to require the command has hostname
                                  specified. Default is True.
        """
        super(server, self).__init__()

        self.parser.add_option('-r', '--role',
                               help='Name of a role',
                               type='string',
                               default=None,
                               metavar='ROLE')
        self.parser.add_option('-x', '--action',
                               help=('Set to True to apply actions when role '
                                     'or status is changed, e.g., restart '
                                     'scheduler when a drone is removed.'),
                               action='store_true',
                               default=False,
                               metavar='ACTION')

        self.topic_parse_info = topic_common.item_parse_info(
                attribute_name='hostname', use_leftover=True)

        self.hostname_required = hostname_required


    def parse(self):
        """Parse command arguments.
        """
        role_info = topic_common.item_parse_info(attribute_name='role')
        kwargs = {}
        if self.hostname_required:
            kwargs['req_items'] = 'hostname'
        (options, leftover) = super(server, self).parse([role_info], **kwargs)
        if options.web_server:
            self.invalid_syntax('Server actions will access server database '
                                'defined in your local global config. It does '
                                'not rely on RPC, no autotest server needs to '
                                'be specified.')

        # self.hostname is a list. Action on server only needs one hostname at
        # most.
        if ((not self.hostname and self.hostname_required) or
            len(self.hostname) > 1):
            self.invalid_syntax('`server` topic can only manipulate 1 server. '
                                'Use -h to see available options.')
        if self.hostname:
            # Override self.hostname with the first hostname in the list.
            self.hostname = self.hostname[0]
        self.role = options.role
        return (options, leftover)


    def output(self, results):
        """Display output.

        For most actions, the return is a string message, no formating needed.

        @param results: return of the execute call.
        """
        print results


class server_help(server):
    """Just here to get the atest logic working. Usage is set by its parent.
    """
    pass


class server_list(action_common.atest_list, server):
    """atest server list [--role <role>]"""

    def __init__(self):
        """Initializer.
        """
        super(server_list, self).__init__(hostname_required=False)
        self.parser.add_option('-t', '--table',
                               help=('List details of all servers in a table, '
                                     'e.g., \tHostname | Status  | Roles     | '
                                     'note\t\tserver1  | primary | scheduler | '
                                     'lab'),
                               action='store_true',
                               default=False,
                               metavar='TABLE')
        self.parser.add_option('-s', '--status',
                               help='Only show servers with given status',
                               type='string',
                               default=None,
                               metavar='STATUS')
        self.parser.add_option('-u', '--summary',
                               help=('Show the summary of roles and status '
                                     'only, e.g.,\tscheduler: server1(primary) '
                                     'server2(backup)\t\tdrone: server3(primary'
                                     ') server4(backup)'),
                               action='store_true',
                               default=False,
                               metavar='SUMMARY')


    def parse(self):
        """Parse command arguments.
        """
        (options, leftover) = super(server_list, self).parse()
        self.table = options.table
        self.status = options.status
        self.summary = options.summary
        if self.table and self.summary:
            self.invalid_syntax('Option --table and --summary cannot be both '
                                'specified.')
        return (options, leftover)


    def execute(self):
        """Execute the command.

        @return: A list of servers matched given hostname and role.
        """
        try:
            return server_manager_utils.get_servers(hostname=self.hostname,
                                                    role=self.role,
                                                    status=self.status)
        except (server_manager_utils.ServerActionError,
                error.InvalidDataError) as e:
            self.failure(e, what_failed='Failed to find servers',
                         item=self.hostname, fatal=True)


    def output(self, results):
        """Display output.

        @param results: return of the execute call, a list of server object that
                        contains server information.
        """
        if not results:
            self.failure('No server is found.',
                         what_failed='Failed to find servers',
                         item=self.hostname, fatal=True)
        else:
            print server_manager_utils.get_server_details(results, self.table,
                                                          self.summary)


class server_create(server):
    """atest server create hostname --role <role> --note <note>
    """

    def __init__(self):
        """Initializer.
        """
        super(server_create, self).__init__()
        self.parser.add_option('-n', '--note',
                               help='note of the server',
                               type='string',
                               default=None,
                               metavar='NOTE')


    def parse(self):
        """Parse command arguments.
        """
        (options, leftover) = super(server_create, self).parse()
        self.note = options.note

        if not self.role:
            self.invalid_syntax('--role is required to create a server.')

        return (options, leftover)


    def execute(self):
        """Execute the command.

        @return: A Server object if it is created successfully.
        """
        try:
            return server_manager.create(hostname=self.hostname, role=self.role,
                                         note=self.note)
        except (server_manager_utils.ServerActionError,
                error.InvalidDataError) as e:
            self.failure(e, what_failed='Failed to create server',
                         item=self.hostname, fatal=True)


    def output(self, results):
        """Display output.

        @param results: return of the execute call, a server object that
                        contains server information.
        """
        if results:
            print 'Server %s is added to server database:\n' % self.hostname
            print results


class server_delete(server):
    """atest server delete hostname"""

    def execute(self):
        """Execute the command.

        @return: True if server is deleted successfully.
        """
        try:
            server_manager.delete(hostname=self.hostname)
            return True
        except (server_manager_utils.ServerActionError,
                error.InvalidDataError) as e:
            self.failure(e, what_failed='Failed to delete server',
                         item=self.hostname, fatal=True)


    def output(self, results):
        """Display output.

        @param results: return of the execute call.
        """
        if results:
            print ('Server %s is deleted from server database successfully.' %
                   self.hostname)


class server_modify(server):
    """atest server modify hostname

    modify action can only change one input at a time. Available inputs are:
    --status:       Status of the server.
    --note:         Note of the server.
    --role:         New role to be added to the server.
    --delete_role:  Existing role to be deleted from the server.
    """

    def __init__(self):
        """Initializer.
        """
        super(server_modify, self).__init__()
        self.parser.add_option('-s', '--status',
                               help='Status of the server',
                               type='string',
                               metavar='STATUS')
        self.parser.add_option('-n', '--note',
                               help='Note of the server',
                               type='string',
                               default=None,
                               metavar='NOTE')
        self.parser.add_option('-d', '--delete',
                               help=('Set to True to delete given role.'),
                               action='store_true',
                               default=False,
                               metavar='DELETE')
        self.parser.add_option('-a', '--attribute',
                               help='Name of the attribute of the server',
                               type='string',
                               default=None,
                               metavar='ATTRIBUTE')
        self.parser.add_option('-e', '--value',
                               help='Value for the attribute of the server',
                               type='string',
                               default=None,
                               metavar='VALUE')


    def parse(self):
        """Parse command arguments.
        """
        (options, leftover) = super(server_modify, self).parse()
        self.status = options.status
        self.note = options.note
        self.delete = options.delete
        self.attribute = options.attribute
        self.value = options.value
        self.action = options.action

        # modify supports various options. However, it's safer to limit one
        # option at a time so no complicated role-dependent logic is needed
        # to handle scenario that both role and status are changed.
        # self.parser is optparse, which does not have function in argparse like
        # add_mutually_exclusive_group. That's why the count is used here.
        flags = [self.status is not None, self.role is not None,
                 self.attribute is not None, self.note is not None]
        if flags.count(True) != 1:
            msg = ('Action modify only support one option at a time. You can '
                   'try one of following 5 options:\n'
                   '1. --status:                Change server\'s status.\n'
                   '2. --note:                  Change server\'s note.\n'
                   '3. --role with optional -d: Add/delete role from server.\n'
                   '4. --attribute --value:     Set/change the value of a '
                   'server\'s attribute.\n'
                   '5. --attribute -d:          Delete the attribute from the '
                   'server.\n'
                   '\nUse option -h to see a complete list of options.')
            self.invalid_syntax(msg)
        if (self.status != None or self.note != None) and self.delete:
            self.invalid_syntax('--delete does not apply to status or note.')
        if self.attribute != None and not self.delete and self.value == None:
            self.invalid_syntax('--attribute must be used with option --value '
                                'or --delete.')
        return (options, leftover)


    def execute(self):
        """Execute the command.

        @return: The updated server object if it is modified successfully.
        """
        try:
            return server_manager.modify(hostname=self.hostname, role=self.role,
                                         status=self.status, delete=self.delete,
                                         note=self.note,
                                         attribute=self.attribute,
                                         value=self.value, action=self.action)
        except (server_manager_utils.ServerActionError,
                error.InvalidDataError) as e:
            self.failure(e, what_failed='Failed to modify server',
                         item=self.hostname, fatal=True)


    def output(self, results):
        """Display output.

        @param results: return of the execute call, which is the updated server
                        object.
        """
        if results:
            print 'Server %s is modified successfully.' % self.hostname
            print results
