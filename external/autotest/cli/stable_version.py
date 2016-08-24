# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module contains functions to get or update stable version for a given
board.

The valid actions are:
list:   Show version of a given board or list all boards and their stable
        versions if --board option is not specified.
modify: Set the stable version of a given board to the given value.
delete: Delete the stable version of a given board. So its stable version will
        use the value for board `DEFAULT`.
"""

import common

from autotest_lib.cli import topic_common


class stable_version(topic_common.atest):
    """stable_version class

    atest stable_version [list|delete|modify] <options>
    """
    usage_action = '[list|delete|modify]'
    topic = msg_topic = 'stable_version'
    msg_items = '<stable_version>'

    def __init__(self):
        """Add to the parser the options common to all the
        stable_version actions.
        """
        super(stable_version, self).__init__()

        self.parser.add_option('-b', '--board',
                               help='Name of the board',
                               type='string',
                               default=None,
                               metavar='BOARD')

        self.topic_parse_info = topic_common.item_parse_info(
                attribute_name='board', use_leftover=True)


    def parse(self):
        """Parse command arguments.
        """
        board_info = topic_common.item_parse_info(attribute_name='board')
        (options, leftover) = super(stable_version, self).parse([board_info])

        self.board = options.board
        return (options, leftover)


    def output(self, results):
        """Display output.

        For most actions, the return is a string message, no formating needed.

        @param results: return of the execute call.
        """
        if results:
            print results


class stable_version_help(stable_version):
    """Just here to get the atest logic working. Usage is set by its parent.
    """
    pass


class stable_version_list(stable_version):
    """atest stable_version list [--board <board>]"""

    def execute(self):
        """Execute list stable version action.
        """
        if self.board:
            version = self.execute_rpc(op='get_stable_version',
                                       board=self.board)
            return {self.board: version}
        else:
            return self.execute_rpc(op='get_all_stable_versions')


    def output(self, results):
        """Display output.

        @param results: A dictionary of board:version.
        """
        board_columns = max([len(s) for s in results.keys()])
        version_columns = max([len(s) for s in results.values()])
        total_columns = board_columns + version_columns + 3
        format = '%%-%ds | %%s' % board_columns
        print '=' * total_columns
        print format % ('board', 'version')
        print '-' * total_columns
        for board,version in results.iteritems():
            print format % (board, version)
        print '=' * total_columns


class stable_version_modify(stable_version):
    """atest stable_version modify --board <board> --version <version>

    Change the stable version of a given board to the given value.
    """

    def __init__(self):
        """Add to the parser the options common to all the
        stable_version actions.
        """
        super(stable_version_modify, self).__init__()

        self.parser.add_option('-i', '--version',
                               help='Stable version.',
                               type='string',
                               metavar='VERSION')

        self.topic_parse_info = topic_common.item_parse_info(
                attribute_name='board', use_leftover=True)


    def parse(self):
        """Parse command arguments.
        """
        options,leftover = super(stable_version_modify, self).parse()

        self.version = options.version
        if not self.board or not self.version:
            self.invalid_syntax('Both --board and --version arguments must be '
                                'specified.')


    def execute(self):
        """Execute delete stable version action.
        """
        current_version = self.execute_rpc(op='get_stable_version',
                                           board=self.board)
        if current_version == self.version:
            print ('Board %s already has stable version of %s.' %
                   (self.board, self.version))
            return

        self.execute_rpc(op='set_stable_version', board=self.board,
                         version=self.version)
        print ('Stable version for board %s is changed from %s to %s.' %
               (self.board, current_version, self.version))


class stable_version_delete(stable_version):
    """atest stable_version delete --board <board>

    Delete a stable version entry in afe_stable_versions table for a given
    board, so default stable version will be used.
    """

    def parse(self):
        """Parse command arguments.
        """
        super(stable_version_delete, self).parse()
        if not self.board:
            self.invalid_syntax('`board` argument must be specified to delete '
                                'a stable version entry.')
        if self.board == 'DEFAULT':
            self.invalid_syntax('Stable version for board DEFAULT can not be '
                                'deleted.')


    @topic_common.atest.require_confirmation(
            'Are you sure to delete stable version for the given board?')
    def execute(self):
        """Execute delete stable version action.
        """
        self.execute_rpc(op='delete_stable_version', board=self.board)
        print 'Stable version for board %s is deleted.' % self.board
        default_stable_version = self.execute_rpc(op='get_stable_version')
        print ('Stable version for board %s is default to %s' %
               (self.board, default_stable_version))
