# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
The job module contains the objects and methods used to
manage jobs in Autotest.

The valid actions are:
list:    lists job(s)
create:  create a job
abort:   abort job(s)
stat:    detailed listing of job(s)

The common options are:

See topic_common.py for a High Level Design and Algorithm.
"""

from autotest_lib.cli import topic_common, action_common


class site_suite(topic_common.atest):
    """Suite class
    atest suite [create] [options]"""
    usage_action = '[create]'
    topic = msg_topic = 'suite'
    msg_items = ''


class site_suite_help(site_suite):
    """Just here to get the atest logic working.
    Usage is set by its parent"""
    pass


class site_suite_create(action_common.atest_create, site_suite):
    """Class containing the code for creating a suite."""
    msg_items = 'suite_id'

    def __init__(self):
        super(site_suite_create, self).__init__()

        self.parser.add_option('-b', '--board', help='Board to test. Required.',
                               metavar='BOARD')
        self.parser.add_option('-i', '--build',
                               help='OS image to install before running the '
                                    'test, e.g. '
                                    'x86-alex-release/R17-1412.144.0-a1-b115.'
                                    ' Required.',
                               metavar='BUILD')
        self.parser.add_option('-c', '--check_hosts',
                               default=False,
                               help='Check that enough live hosts exist to '\
                                    'run this suite. Default False.',
                               action='store_true',
                               metavar='CHECK_HOSTS')
        self.parser.add_option('-f', '--file_bugs', default=False,
                               help='File bugs on test failures.',
                               action='store_true', metavar='FILE_BUGS')
        self.parser.add_option('-n', '--num', type=int,
                               help='Number of machines to schedule across.',
                               metavar='NUM')
        self.parser.add_option('-p', '--pool', help='Pool of machines to use.',
                               metavar='POOL')
        self.parser.add_option('-w', '--wait_for_results',
                               default=True,
                               help=('Set to False for suite job to exit '
                                     'without waiting for test jobs to finish. '
                                     'Default is True.'),
                               metavar='WAIT_FOR_RESULTS')


    def parse(self):
        board_info = topic_common.item_parse_info(attribute_name='board',
                                                  inline_option='board')
        build_info = topic_common.item_parse_info(attribute_name='build',
                                                  inline_option='build')
        pool_info = topic_common.item_parse_info(attribute_name='pool',
                                                 inline_option='pool')
        num_info = topic_common.item_parse_info(attribute_name='num',
                                                inline_option='num')
        check_info = topic_common.item_parse_info(attribute_name='check_hosts',
                                                  inline_option='check_hosts')
        bugs_info = topic_common.item_parse_info(attribute_name='file_bugs',
                                                 inline_option='file_bugs')
        suite_info = topic_common.item_parse_info(attribute_name='name',
                                                  use_leftover=True)
        wait_for_results_info = topic_common.item_parse_info(
            attribute_name='wait_for_results',
            inline_option='wait_for_results')

        options, leftover = site_suite.parse(
            self,
            [suite_info, board_info, build_info, pool_info, num_info,
             check_info, bugs_info, wait_for_results_info],
            req_items='name')
        self.data = {}
        name = getattr(self, 'name')
        if len(name) > 1:
            self.invalid_syntax('Too many arguments specified, only expected '
                                'to receive suite name: %s' % name)
        self.data['suite_name'] = name[0]
        self.data['pool'] = options.pool  # None is OK.
        self.data['num'] = options.num  # None is OK.
        self.data['check_hosts'] = options.check_hosts
        self.data['file_bugs'] = options.file_bugs
        self.data['wait_for_results'] = options.wait_for_results
        if options.board:
            self.data['board'] = options.board
        else:
            self.invalid_syntax('--board is required.')
        if options.build:
            self.data['build'] = options.build
        else:
            self.invalid_syntax('--build is required.')

        return options, leftover


    def execute(self):
        return [self.execute_rpc(op='create_suite_job', **self.data)]
