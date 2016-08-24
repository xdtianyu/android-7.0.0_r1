#!/usr/bin/python

"""Unit tests for the perf_uploader.py module.

"""

import json, unittest

import common
from autotest_lib.tko import models as tko_models
from autotest_lib.tko.perf_upload import perf_uploader


class test_aggregate_iterations(unittest.TestCase):
    """Tests for the aggregate_iterations function."""

    _PERF_ITERATION_DATA = {
        '1': [
            {
                'description': 'metric1',
                'value': 1,
                'stddev': 0.0,
                'units': 'units1',
                'higher_is_better': True,
                'graph': None
            },
            {
                'description': 'metric2',
                'value': 10,
                'stddev': 0.0,
                'units': 'units2',
                'higher_is_better': True,
                'graph': 'graph1',
            },
            {
                'description': 'metric2',
                'value': 100,
                'stddev': 1.7,
                'units': 'units3',
                'higher_is_better': False,
                'graph': 'graph2',
            }
        ],
        '2': [
            {
                'description': 'metric1',
                'value': 2,
                'stddev': 0.0,
                'units': 'units1',
                'higher_is_better': True,
                'graph': None,
            },
            {
                'description': 'metric2',
                'value': 20,
                'stddev': 0.0,
                'units': 'units2',
                'higher_is_better': True,
                'graph': 'graph1',
            },
            {
                'description': 'metric2',
                'value': 200,
                'stddev': 21.2,
                'units': 'units3',
                'higher_is_better': False,
                'graph': 'graph2',
            }
        ],
    }


    def setUp(self):
        """Sets up for each test case."""
        self._perf_values = []
        for iter_num, iter_data in self._PERF_ITERATION_DATA.iteritems():
            self._perf_values.append(
                    tko_models.perf_value_iteration(iter_num, iter_data))


    def test_one_iteration(self):
        """Tests that data for 1 iteration is aggregated properly."""
        result = perf_uploader._aggregate_iterations([self._perf_values[0]])
        self.assertEqual(len(result), 3, msg='Expected results for 3 metrics.')
        key = [('metric1', None), ('metric2', 'graph1'), ('metric2', 'graph2')]
        self.assertTrue(
            all([x in result for x in key]),
            msg='Parsed metrics not as expected.')
        msg = 'Perf values for metric not aggregated properly.'
        self.assertEqual(result[('metric1', None)]['value'], [1], msg=msg)
        self.assertEqual(result[('metric2', 'graph1')]['value'], [10], msg=msg)
        self.assertEqual(result[('metric2', 'graph2')]['value'], [100], msg=msg)
        msg = 'Standard deviation values not retained properly.'
        self.assertEqual(result[('metric1', None)]['stddev'], 0.0, msg=msg)
        self.assertEqual(result[('metric2', 'graph1')]['stddev'], 0.0, msg=msg)
        self.assertEqual(result[('metric2', 'graph2')]['stddev'], 1.7, msg=msg)


    def test_two_iterations(self):
        """Tests that data for 2 iterations is aggregated properly."""
        result = perf_uploader._aggregate_iterations(self._perf_values)
        self.assertEqual(len(result), 3, msg='Expected results for 3 metrics.')
        key = [('metric1', None), ('metric2', 'graph1'), ('metric2', 'graph2')]
        self.assertTrue(
            all([x in result for x in key]),
            msg='Parsed metrics not as expected.')
        msg = 'Perf values for metric not aggregated properly.'
        self.assertEqual(result[('metric1', None)]['value'], [1, 2], msg=msg)
        self.assertEqual(result[('metric2', 'graph1')]['value'], [10, 20],
                         msg=msg)
        self.assertEqual(result[('metric2', 'graph2')]['value'], [100, 200],
                         msg=msg)


class test_compute_avg_stddev(unittest.TestCase):
    """Tests for the compute_avg_stddev function."""

    def setUp(self):
        """Sets up for each test case."""
        self._perf_values = {
            'metric1': {'value': [10, 20, 30], 'stddev': 0.0},
            'metric2': {'value': [2.0, 3.0, 4.0], 'stddev': 0.0},
            'metric3': {'value': [1], 'stddev': 1.7},
        }


    def test_avg_stddev(self):
        """Tests that averages and standard deviations are computed properly."""
        perf_uploader._compute_avg_stddev(self._perf_values)
        result = self._perf_values  # The input dictionary itself is modified.
        self.assertEqual(len(result), 3, msg='Expected results for 3 metrics.')
        self.assertTrue(
            all([x in result for x in ['metric1', 'metric2', 'metric3']]),
            msg='Parsed metrics not as expected.')
        msg = 'Average value not computed properly.'
        self.assertEqual(result['metric1']['value'], 20, msg=msg)
        self.assertEqual(result['metric2']['value'], 3.0, msg=msg)
        self.assertEqual(result['metric3']['value'], 1, msg=msg)
        msg = 'Standard deviation value not computed properly.'
        self.assertEqual(result['metric1']['stddev'], 10.0, msg=msg)
        self.assertEqual(result['metric2']['stddev'], 1.0, msg=msg)
        self.assertEqual(result['metric3']['stddev'], 1.7, msg=msg)


class test_json_config_file_sanity(unittest.TestCase):
    """Sanity tests for the JSON-formatted presentation config file."""

    def test_parse_json(self):
        """Verifies _parse_config_file function."""
        perf_uploader._parse_config_file(
                perf_uploader._PRESENTATION_CONFIG_FILE)


    def test_proper_json(self):
        """Verifies the file can be parsed as proper JSON."""
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')


    def test_unique_test_names(self):
        """Verifies that each test name appears only once in the JSON file."""
        json_obj = []
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json_obj = json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')

        name_set = set([x['autotest_name'] for x in json_obj])
        self.assertEqual(len(name_set), len(json_obj),
                         msg='Autotest names not unique in the JSON file.')


    def test_required_master_name(self):
        """Verifies that master name must be specified."""
        json_obj = []
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json_obj = json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')

        for entry in json_obj:
            if not 'master_name' in entry:
                self.fail('Missing master field for test %s.' %
                          entry['autotest_name'])


class test_gather_presentation_info(unittest.TestCase):
    """Tests for the gather_presentation_info function."""

    _PRESENT_INFO = {
        'test_name': {
            'master_name': 'new_master_name',
            'dashboard_test_name': 'new_test_name',
        }
    }

    _PRESENT_INFO_MISSING_MASTER = {
        'test_name': {
            'dashboard_test_name': 'new_test_name',
        }
    }


    def test_test_name_specified(self):
        """Verifies gathers presentation info correctly."""
        result = perf_uploader._gather_presentation_info(
                self._PRESENT_INFO, 'test_name')
        self.assertTrue(
                all([key in result for key in
                     ['test_name', 'master_name']]),
                msg='Unexpected keys in resulting dictionary: %s' % result)
        self.assertEqual(result['master_name'], 'new_master_name',
                         msg='Unexpected "master_name" value: %s' %
                             result['master_name'])
        self.assertEqual(result['test_name'], 'new_test_name',
                         msg='Unexpected "test_name" value: %s' %
                             result['test_name'])


    def test_test_name_not_specified(self):
        """Verifies exception raised if test is not there."""
        self.assertRaises(
                perf_uploader.PerfUploadingError,
                perf_uploader._gather_presentation_info,
                        self._PRESENT_INFO, 'other_test_name')


    def test_master_not_specified(self):
        """Verifies exception raised if master is not there."""
        self.assertRaises(
                perf_uploader.PerfUploadingError,
                perf_uploader._gather_presentation_info,
                    self._PRESENT_INFO_MISSING_MASTER, 'test_name')


class test_get_id_from_version(unittest.TestCase):
    """Tests for the _get_id_from_version function."""

    def test_correctly_formatted_versions(self):
        """Verifies that the expected ID is returned when input is OK."""
        chrome_version = '27.0.1452.2'
        cros_version = '27.3906.0.0'
        # 1452.2 + 3906.0.0
        # --> 01452 + 002 + 03906 + 000 + 00
        # --> 14520020390600000
        self.assertEqual(
                14520020390600000,
                perf_uploader._get_id_from_version(
                        chrome_version, cros_version))

        chrome_version = '25.10.1000.0'
        cros_version = '25.1200.0.0'
        # 1000.0 + 1200.0.0
        # --> 01000 + 000 + 01200 + 000 + 00
        # --> 10000000120000000
        self.assertEqual(
                10000000120000000,
                perf_uploader._get_id_from_version(
                        chrome_version, cros_version))

    def test_returns_none_when_given_invalid_input(self):
        """Checks the return value when invalid input is given."""
        chrome_version = '27.0'
        cros_version = '27.3906.0.0'
        self.assertIsNone(perf_uploader._get_id_from_version(
                chrome_version, cros_version))


class test_get_version_numbers(unittest.TestCase):
    """Tests for the _get_version_numbers function."""

    def test_with_valid_versions(self):
      """Checks the version numbers used when data is formatted as expected."""
      self.assertEqual(
              ('34.5678.9.0', '34.5.678.9'),
              perf_uploader._get_version_numbers(
                  {
                      'CHROME_VERSION': '34.5.678.9',
                      'CHROMEOS_RELEASE_VERSION': '5678.9.0',
                  }))

    def test_with_missing_version_raises_error(self):
      """Checks that an error is raised when a version is missing."""
      with self.assertRaises(perf_uploader.PerfUploadingError):
          perf_uploader._get_version_numbers(
              {
                  'CHROMEOS_RELEASE_VERSION': '5678.9.0',
              })

    def test_with_unexpected_version_format_raises_error(self):
      """Checks that an error is raised when there's a rN suffix."""
      with self.assertRaises(perf_uploader.PerfUploadingError):
          perf_uploader._get_version_numbers(
              {
                  'CHROME_VERSION': '34.5.678.9',
                  'CHROMEOS_RELEASE_VERSION': '5678.9.0r1',
              })


class test_format_for_upload(unittest.TestCase):
    """Tests for the format_for_upload function."""

    _PERF_DATA = {
        ('metric1', 'graph_name'): {
            'value': 2.7,
            'stddev': 0.2,
            'units': 'msec',
            'graph': 'graph_name',
            'higher_is_better': False,
        },
        ('metric2', None): {
            'value': 101.35,
            'stddev': 5.78,
            'units': 'frames_per_sec',
            'graph': None,
            'higher_is_better': True,
        },
    }

    _PRESENT_INFO = {
        'master_name': 'new_master_name',
        'test_name': 'new_test_name',
    }

    def setUp(self):
        self._perf_data = self._PERF_DATA

    def _verify_result_string(self, actual_result, expected_result):
        """Verifies a JSON string matches the expected result.

        This function compares JSON objects rather than strings, because of
        possible floating-point values that need to be compared using
        assertAlmostEqual().

        @param actual_result: The candidate JSON string.
        @param expected_result: The reference JSON string that the candidate
            must match.

        """
        actual = json.loads(actual_result)
        expected = json.loads(expected_result)

        fail_msg = 'Unexpected result string: %s' % actual_result
        self.assertEqual(len(actual), len(expected), msg=fail_msg)
        # Make sure the dictionaries in 'expected' are in the same order
        # as the dictionaries in 'actual' before comparing their values.
        actual = sorted(actual, key=lambda x: x['test'])
        expected = sorted(expected, key=lambda x: x['test'])
        # Now compare the results.
        for idx in xrange(len(actual)):
            keys_actual = set(actual[idx].keys())
            keys_expected = set(expected[idx].keys())
            self.assertEqual(len(keys_actual), len(keys_expected),
                             msg=fail_msg)
            self.assertTrue(all([key in keys_actual for key in keys_expected]),
                            msg=fail_msg)

            self.assertEqual(
                    actual[idx]['supplemental_columns']['r_cros_version'],
                    expected[idx]['supplemental_columns']['r_cros_version'],
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['supplemental_columns']['r_chrome_version'],
                    expected[idx]['supplemental_columns']['r_chrome_version'],
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['supplemental_columns']['a_default_rev'],
                    expected[idx]['supplemental_columns']['a_default_rev'],
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['supplemental_columns']['a_hardware_identifier'],
                    expected[idx]['supplemental_columns']['a_hardware_identifier'],
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['supplemental_columns']['a_hardware_hostname'],
                    expected[idx]['supplemental_columns']['a_hardware_hostname'],
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['bot'], expected[idx]['bot'], msg=fail_msg)
            self.assertEqual(
                    actual[idx]['revision'], expected[idx]['revision'], msg=fail_msg)
            self.assertAlmostEqual(
                    actual[idx]['value'], expected[idx]['value'], 4,
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['units'], expected[idx]['units'], msg=fail_msg)
            self.assertEqual(
                    actual[idx]['master'], expected[idx]['master'],
                    msg=fail_msg)
            self.assertAlmostEqual(
                    actual[idx]['error'], expected[idx]['error'], 4,
                    msg=fail_msg)
            self.assertEqual(
                    actual[idx]['test'], expected[idx]['test'], msg=fail_msg)
            self.assertEqual(
                    actual[idx]['higher_is_better'],
                    expected[idx]['higher_is_better'], msg=fail_msg)


    def test_format_for_upload(self):
        """Verifies format_for_upload generates correct json data."""
        result = perf_uploader._format_for_upload(
                'platform', '25.1200.0.0', '25.10.1000.0', 'WINKY E2A-F2K-Q35',
                'i7', 'test_machine', self._perf_data, self._PRESENT_INFO)
        expected_result_string = (
                '[{"supplemental_columns": {"r_cros_version": "25.1200.0.0", '
                '"a_default_rev" : "r_chrome_version",'
                '"a_hardware_identifier" : "WINKY E2A-F2K-Q35",'
                '"a_hardware_hostname" : "test_machine",'
                '"r_chrome_version": "25.10.1000.0"}, "bot": "cros-platform-i7", '
                '"higher_is_better": false, "value": 2.7, '
                '"revision": 10000000120000000, '
                '"units": "msec", "master": "new_master_name", '
                '"error": 0.2, "test": "new_test_name/graph_name/metric1"}, '
                '{"supplemental_columns": {"r_cros_version": "25.1200.0.0", '
                '"a_default_rev" : "r_chrome_version",'
                '"a_hardware_identifier" : "WINKY E2A-F2K-Q35",'
                '"a_hardware_hostname" : "test_machine",'
                '"r_chrome_version": "25.10.1000.0"}, "bot": "cros-platform-i7", '
                '"higher_is_better": true, "value": 101.35, '
                '"revision": 10000000120000000, '
                '"units": "frames_per_sec", "master": "new_master_name", '
                '"error": 5.78, "test": "new_test_name/metric2"}]')

        self._verify_result_string(result['data'], expected_result_string)


if __name__ == '__main__':
    unittest.main()
