#!/usr/bin/python
# pylint: disable-msg=C0111

import json
import os, unittest

import common

from autotest_lib.client.common_lib import control_data, autotemp

ControlData = control_data.ControlData

CONTROL = """
AUTHOR = 'Author'
DEPENDENCIES = "console, power"
DOC = \"\"\"\
doc stuff\"\"\"
# EXPERIMENTAL should implicitly be False
NAME = 'nA' "mE"
RUN_VERIFY = False
SYNC_COUNT = 2
TIME='short'
TEST_CLASS=u'Kernel'
TEST_CATEGORY='Stress'
TEST_TYPE='client'
RETRIES = 5
REQUIRE_SSP = False
ATTRIBUTES = "suite:smoke, suite:bvt"
"""


class ParseControlTest(unittest.TestCase):
    def setUp(self):
        self.control_tmp = autotemp.tempfile(unique_id='control_unit',
                                             text=True)
        os.write(self.control_tmp.fd, CONTROL)


    def tearDown(self):
        self.control_tmp.clean()


    def test_parse_control(self):
        cd = control_data.parse_control(self.control_tmp.name, True)
        self.assertEquals(cd.author, "Author")
        self.assertEquals(cd.dependencies, set(['console', 'power']))
        self.assertEquals(cd.doc, "doc stuff")
        self.assertEquals(cd.experimental, False)
        self.assertEquals(cd.name, "nAmE")
        self.assertEquals(cd.run_verify, False)
        self.assertEquals(cd.sync_count, 2)
        self.assertEquals(cd.time, "short")
        self.assertEquals(cd.test_class, "kernel")
        self.assertEquals(cd.test_category, "stress")
        self.assertEquals(cd.test_type, "client")
        self.assertEquals(cd.retries, 5)
        self.assertEquals(cd.require_ssp, False)
        self.assertEquals(cd.attributes,
                          set(["suite:smoke","suite:bvt","subsystem:default"]))


class ParseControlFileBugTemplate(unittest.TestCase):
    def setUp(self):
        self.control_tmp = autotemp.tempfile(unique_id='control_unit',
                                             text=True)
        self.bug_template = {
            'owner': 'someone@something.org',
            'labels': ['a', 'b'],
            'status': None,
            'summary': None,
            'title': None,
            'cc': ['a@something, b@something'],
        }


    def tearDown(self):
        self.control_tmp.clean()


    def insert_bug_template(self, control_file_string):
        """Insert a bug template into the control file string.

        @param control_file_string: A string of the control file contents
            this test will run on.

        @return: The control file string with the BUG_TEMPLATE line.
        """
        bug_template_line = 'BUG_TEMPLATE = %s' % json.dumps(self.bug_template)
        return control_file_string + bug_template_line


    def verify_bug_template(self, new_bug_template):
        """Verify that the bug template given matches the original.

        @param new_bug_template: A bug template pulled off parsing the
            control file.

        @raises AssetionError: If a value under a give key in the bug template
            doesn't match the value in self.bug_template.
        @raises KeyError: If a key in either bug template is missing.
        """
        for key, value in new_bug_template.iteritems():
            self.assertEqual(value, self.bug_template[key])


    def test_bug_template_parsing(self):
        """Basic parsing test for a bug templates in a test control file."""
        os.write(self.control_tmp.fd, self.insert_bug_template(CONTROL))
        cd = control_data.parse_control(self.control_tmp.name, True)
        self.verify_bug_template(cd.bug_template)


    def test_bug_template_list(self):
        """Test that lists in the bug template can handle other datatypes."""
        self.bug_template['labels'].append({'foo': 'bar'})
        os.write(self.control_tmp.fd, self.insert_bug_template(CONTROL))
        cd = control_data.parse_control(self.control_tmp.name, True)
        self.verify_bug_template(cd.bug_template)


    def test_bad_template(self):
        """Test that a bad bug template doesn't result in a bad control data."""
        self.bug_template = 'foobarbug_template'
        os.write(self.control_tmp.fd, self.insert_bug_template(CONTROL))
        cd = control_data.parse_control(self.control_tmp.name, True)
        self.assertFalse(hasattr(cd, 'bug_template'))


class SetMethodTests(unittest.TestCase):
    def setUp(self):
        self.required_vars = control_data.REQUIRED_VARS
        control_data.REQUIRED_VARS = set()


    def tearDown(self):
        control_data.REQUIRED_VARS = self.required_vars


    def test_bool(self):
        cd = ControlData({}, 'filename')
        cd._set_bool('foo', 'False')
        self.assertEquals(cd.foo, False)
        cd._set_bool('foo', True)
        self.assertEquals(cd.foo, True)
        cd._set_bool('foo', 'FALSE')
        self.assertEquals(cd.foo, False)
        cd._set_bool('foo', 'true')
        self.assertEquals(cd.foo, True)
        self.assertRaises(ValueError, cd._set_bool, 'foo', '')
        self.assertRaises(ValueError, cd._set_bool, 'foo', 1)
        self.assertRaises(ValueError, cd._set_bool, 'foo', [])
        self.assertRaises(ValueError, cd._set_bool, 'foo', None)


    def test_int(self):
        cd = ControlData({}, 'filename')
        cd._set_int('foo', 0)
        self.assertEquals(cd.foo, 0)
        cd._set_int('foo', '0')
        self.assertEquals(cd.foo, 0)
        cd._set_int('foo', '-1', min=-2, max=10)
        self.assertEquals(cd.foo, -1)
        self.assertRaises(ValueError, cd._set_int, 'foo', 0, min=1)
        self.assertRaises(ValueError, cd._set_int, 'foo', 1, max=0)
        self.assertRaises(ValueError, cd._set_int, 'foo', 'x')
        self.assertRaises(ValueError, cd._set_int, 'foo', '')
        self.assertRaises(TypeError, cd._set_int, 'foo', None)


    def test_set(self):
        cd = ControlData({}, 'filename')
        cd._set_set('foo', 'a')
        self.assertEquals(cd.foo, set(['a']))
        cd._set_set('foo', 'a,b,c')
        self.assertEquals(cd.foo, set(['a', 'b', 'c']))
        cd._set_set('foo', ' a , b , c     ')
        self.assertEquals(cd.foo, set(['a', 'b', 'c']))
        cd._set_set('foo', None)
        self.assertEquals(cd.foo, set(['None']))


    def test_string(self):
        cd = ControlData({}, 'filename')
        cd._set_string('foo', 'a')
        self.assertEquals(cd.foo, 'a')
        cd._set_string('foo', 'b')
        self.assertEquals(cd.foo, 'b')
        cd._set_string('foo', 'B')
        self.assertEquals(cd.foo, 'B')
        cd._set_string('foo', 1)
        self.assertEquals(cd.foo, '1')
        cd._set_string('foo', None)
        self.assertEquals(cd.foo, 'None')
        cd._set_string('foo', [])
        self.assertEquals(cd.foo, '[]')


    def test_option(self):
        options = ['a', 'b']
        cd = ControlData({}, 'filename')
        cd._set_option('foo', 'a', options)
        self.assertEquals(cd.foo, 'a')
        cd._set_option('foo', 'b', options)
        self.assertEquals(cd.foo, 'b')
        cd._set_option('foo', 'B', options)
        self.assertEquals(cd.foo, 'B')
        self.assertRaises(ValueError, cd._set_option,
                          'foo', 'x', options)
        self.assertRaises(ValueError, cd._set_option,
                          'foo', 1, options)
        self.assertRaises(ValueError, cd._set_option,
                          'foo', [], options)
        self.assertRaises(ValueError, cd._set_option,
                          'foo', None, options)


    def test_set_attributes(self):
        cd = ControlData({}, 'filename')
        cd.set_attributes('suite:bvt')
        self.assertEquals(cd.attributes, set(['suite:bvt',
                                              'subsystem:default']))
        cd.set_attributes('suite:bvt, subsystem:network')
        self.assertEquals(cd.attributes, set(['suite:bvt',
                                              'subsystem:network']))


    def test_get_test_time_index(self):
        inputs = [time.upper() for time in
                  ControlData.TEST_TIME_LIST]
        time_min_index = [ControlData.get_test_time_index(time)
                          for time in inputs]
        expected_time_index = range(len(ControlData.TEST_TIME_LIST))
        self.assertEqual(time_min_index, expected_time_index)


    def test_get_test_time_index_failure(self):
        def fail():
            """Test function to raise ControlVariableException exception
            for invalid TIME setting."""
            index = ControlData.get_test_time_index('some invalid TIME')

        self.assertRaises(control_data.ControlVariableException, fail)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    unittest.main()
