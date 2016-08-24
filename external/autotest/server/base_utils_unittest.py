#!/usr/bin/python

__author__ = 'raphtee@google.com (Travis Miller)'

import unittest
import common
from autotest_lib.server import utils


class UtilsTest(unittest.TestCase):

    def setUp(self):
        # define out machines here
        self.machines = ['mach1', 'mach2', 'mach3', 'mach4', 'mach5',
                                'mach6', 'mach7']

        self.ntuples = [['mach1', 'mach2'], ['mach3', 'mach4'],
                        ['mach5', 'mach6']]
        self.failures = []
        self.failures.append(('mach7', "machine can not be tupled"))


    def test_form_cell_mappings(self):
        (ntuples, failures) = utils.form_ntuples_from_machines(self.machines)
        self.assertEquals(self.ntuples, ntuples)
        self.assertEquals(self.failures, failures)


    # parse_machine() test cases
    def test_parse_machine_good(self):
        '''test that parse_machine() is outputting the correct data'''
        gooddata = (('host',                ('host', 'root', '', 22)),
                    ('host:21',             ('host', 'root', '', 21)),
                    ('user@host',           ('host', 'user', '', 22)),
                    ('user:pass@host',      ('host', 'user', 'pass', 22)),
                    ('user:pass@host:1234', ('host', 'user', 'pass', 1234)),

                    ('user:pass@10.3.2.1',
                     ('10.3.2.1', 'user', 'pass', 22)),
                    ('user:pass@10.3.2.1:1234',
                     ('10.3.2.1', 'user', 'pass', 1234)),

                    ('::1',                 ('::1', 'root', '', 22)),
                    ('user:pass@abdc::ef',  ('abdc::ef', 'user', 'pass', 22)),
                    ('abdc::ef:99',         ('abdc::ef:99', 'root', '', 22)),
                    ('user:pass@[abdc::ef:99]',
                     ('abdc::ef:99', 'user', 'pass', 22)),
                    ('user:pass@[abdc::ef]:1234',
                     ('abdc::ef', 'user', 'pass', 1234)),
                   )
        for machine, result in gooddata:
            self.assertEquals(utils.parse_machine(machine), result)


    def test_parse_machine_override(self):
        '''Test that parse_machine() defaults can be overridden'''
        self.assertEquals(utils.parse_machine('host', 'bob', 'foo', 1234),
                          ('host', 'bob', 'foo', 1234))


    def test_parse_machine_bad(self):
        '''test that bad data passed to parse_machine() will raise an exception'''
        baddata = ((':22', IndexError),         # neglect to pass a hostname #1
                   ('user@', IndexError),       # neglect to pass a hostname #2
                   ('user@:22', IndexError),    # neglect to pass a hostname #3
                   (':pass@host', ValueError),  # neglect to pass a username
                   ('host:', ValueError),       # empty port after hostname
                   ('[1::2]:', ValueError),     # empty port after IPv6
                  )
        for machine, exception in baddata:
            self.assertRaises(exception, utils.parse_machine, machine)


if __name__ == "__main__":
    unittest.main()
