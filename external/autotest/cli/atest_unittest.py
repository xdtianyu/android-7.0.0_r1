#!/usr/bin/python
#
# Copyright 2008 Google Inc. All Rights Reserved.

"""Test for atest."""

import unittest

import common
from autotest_lib.cli import cli_mock

ATEST_USAGE_STRING = ('atest [acl|host|job|label|shard|atomicgroup|test|user|'
                      'server|stable_version] [action] [options]')

class main_unittest(cli_mock.cli_unittest):
    """Unittest for atest command.
    """

    def _test_help(self, argv, out_words_ok, err_words_ok):
        """Test help output.

        @param argv: A list of argument.
        @param out_words_ok: Expected output.
        @param err_words_ok: Expected output when input arguments are invalid.
        """
        saved_outputs = None
        for help in ['-h', '--help', 'help']:
            outputs = self.run_cmd(argv + [help], exit_code=0,
                                   out_words_ok=out_words_ok,
                                   err_words_ok=err_words_ok)
            if not saved_outputs:
                saved_outputs = outputs
            else:
                self.assertEqual(outputs, saved_outputs)


    def test_main_help(self):
        """Main help level"""
        self._test_help(argv=['atest'],
                        out_words_ok=[ATEST_USAGE_STRING],
                        err_words_ok=[])


    def test_main_help_topic(self):
        """Topic level help"""
        self._test_help(argv=['atest', 'host'],
                        out_words_ok=['atest host ',
                                      '[create|delete|list|stat|mod|jobs]'
                                      ' [options]'],
                        err_words_ok=[])


    def test_main_help_action(self):
        """Action level help"""
        self._test_help(argv=['atest:', 'host', 'mod'],
                        out_words_ok=['atest host mod [options]'],
                        err_words_ok=[])


    def test_main_no_topic(self):
        """Test output when no topic is specified."""
        self.run_cmd(['atest'], exit_code=1,
                     out_words_ok=[ATEST_USAGE_STRING],
                     err_words_ok=['No topic argument'])


    def test_main_bad_topic(self):
        """Test output when an invalid topic is specified."""
        self.run_cmd(['atest', 'bad_topic'], exit_code=1,
                     out_words_ok=[ATEST_USAGE_STRING],
                     err_words_ok=['Invalid topic bad_topic\n'])


    def test_main_bad_action(self):
        """Test output when an invalid action is specified."""
        self.run_cmd(['atest', 'host', 'bad_action'], exit_code=1,
                     out_words_ok=['atest host [create|delete|list|stat|'
                                   'mod|jobs] [options]'],
                     err_words_ok=['Invalid action bad_action'])


if __name__ == '__main__':
    unittest.main()
