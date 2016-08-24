# pylint: disable-msg=C0111
#!/usr/bin/python
#
# Copyright 2008 Google Inc. All Rights Reserved.

"""Tests for shard."""

import unittest

import common
from autotest_lib.cli import cli_mock


class shard_list_unittest(cli_mock.cli_unittest):
    values = [{'hostname': u'shard1', u'id': 1, 'labels': ['board:lumpy']},
              {'hostname': u'shard2', u'id': 3, 'labels': ['board:daisy']},
              {'hostname': u'shard3', u'id': 5, 'labels': ['board:stumpy']},
              {'hostname': u'shard4', u'id': 6, 'labels': ['board:link']}]


    def test_shard_list(self):
        self.run_cmd(argv=['atest', 'shard', 'list'],
                     rpcs=[('get_shards', {}, True, self.values)],
                     out_words_ok=['shard1', 'shard2', 'shard3', 'shard4'],
                     out_words_no=['plat0', 'plat1'])


class shard_create_unittest(cli_mock.cli_unittest):
    def test_execute_create_two_shards(self):
        self.run_cmd(argv=['atest', 'shard', 'create',
                           '-l', 'board:lumpy', 'shard0'],
                     rpcs=[('add_shard',
                            {'hostname': 'shard0', 'labels': 'board:lumpy'},
                            True, 42)],
                     out_words_ok=['Created', 'shard0'])


    def test_execute_create_two_shards_bad(self):
        self.run_cmd(argv=['atest', 'shard', 'create',
                           '-l', 'board:lumpy', 'shard0'],
                     rpcs=[('add_shard',
                            {'hostname': 'shard0', 'labels': 'board:lumpy'},
                            False,
                            '''ValidationError: {'name':
                            'This value must be unique (shard1)'}''')],
                     out_words_no=['shard0'],
                     err_words_ok=['shard0', 'ValidationError'])


class shard_delete_unittest(cli_mock.cli_unittest):
    def test_execute_delete_shards(self):
        self.run_cmd(argv=['atest', 'shard', 'delete',
                           'shard0', 'shard1', '--no-confirmation'],
                     rpcs=[('delete_shard', {'hostname': 'shard0'}, True, None),
                           ('delete_shard', {'hostname': 'shard1'}, True, None)
                           ],
                     out_words_ok=['Deleted', 'shard0', 'shard1'])


if __name__ == '__main__':
    unittest.main()
