#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=W0311

from collections import namedtuple
import argparse
import glob
import json
import os
import pprint
import re
import subprocess

_EXPECTATIONS_DIR = 'expectations'
_AUTOTEST_RESULT_TEMPLATE = 'gs://chromeos-autotest-results/%s-chromeos-test/chromeos*/graphics_dEQP/debug/graphics_dEQP.DEBUG'
# Use this template for tryjob results:
#_AUTOTEST_RESULT_TEMPLATE = 'gs://chromeos-autotest-results/%s-ihf/*/graphics_dEQP/debug/graphics_dEQP.DEBUG'
_BOARD_REGEX = re.compile(r'ChromeOS BOARD = (.+)')
_CPU_FAMILY_REGEX = re.compile(r'ChromeOS CPU family = (.+)')
_GPU_FAMILY_REGEX = re.compile(r'ChromeOS GPU family = (.+)')
_TEST_FILTER_REGEX = re.compile(r'dEQP test filter = (.+)')
_HASTY_MODE_REGEX = re.compile(r'\'hasty\': \'True\'|Running in hasty mode.')

#04/23 07:30:21.624 INFO |graphics_d:0240| TestCase: dEQP-GLES3.functional.shaders.operator.unary_operator.bitwise_not.highp_ivec3_vertex
#04/23 07:30:21.840 INFO |graphics_d:0261| Result: Pass
_TEST_RESULT_REGEX = re.compile(r'TestCase: (.+?)$\n.+? Result: (.+?)$',
                                re.MULTILINE)
_HASTY_TEST_RESULT_REGEX = re.compile(
    r'\[stdout\] Test case \'(.+?)\'..$\n'
    r'.+?\[stdout\]   (Pass|Fail|QualityWarning) \((.+)\)', re.MULTILINE)
Logfile = namedtuple('Logfile', 'job_id name gs_path')


def execute(cmd_list):
  sproc = subprocess.Popen(cmd_list, stdout=subprocess.PIPE)
  return sproc.communicate()[0]


def get_metadata(s):
  cpu = re.search(_CPU_FAMILY_REGEX, s).group(1)
  gpu = re.search(_GPU_FAMILY_REGEX, s).group(1)
  board = re.search(_BOARD_REGEX, s).group(1)
  filter = re.search(_TEST_FILTER_REGEX, s).group(1)
  hasty = False
  if re.search(_HASTY_MODE_REGEX, s):
    hasty = True
  print('Found results from %s for GPU = %s, filter = %s and hasty = %r.' %
        (board, gpu, filter, hasty))
  return board, gpu, filter, hasty


def get_logs_from_gs(autotest_result_path):
  logs = []
  gs_paths = execute(['gsutil', 'ls', autotest_result_path]).splitlines()
  for gs_path in gs_paths:
    job_id = gs_path.split('/')[3].split('-')[0]
    # DEBUG logs have more information than INFO logs, especially for hasty.
    name = os.path.join('logs', job_id + '_graphics_dEQP.DEBUG')
    logs.append(Logfile(job_id, name, gs_path))
  for log in logs:
    execute(['gsutil', 'cp', log.gs_path, log.name])
  return logs


def get_local_logs():
  logs = []
  for name in glob.glob(os.path.join('logs', '*_graphics_dEQP.INFO')):
    job_id = name.split('_')[0]
    logs.append(Logfile(job_id, name, name))
  for name in glob.glob(os.path.join('logs', '*_graphics_dEQP.DEBUG')):
    job_id = name.split('_')[0]
    logs.append(Logfile(job_id, name, name))
  return logs


def get_all_tests(text):
  tests = []
  for test, result in re.findall(_TEST_RESULT_REGEX, text):
    tests.append((test, result))
  for test, result, details in re.findall(_HASTY_TEST_RESULT_REGEX, text):
    tests.append((test, result))
  return tests


def get_not_passing_tests(text):
  not_passing = []
  for test, result in re.findall(_TEST_RESULT_REGEX, text):
    if not (result == 'Pass' or result == 'NotSupported'):
      not_passing.append((test, result))
  for test, result, details in re.findall(_HASTY_TEST_RESULT_REGEX, text):
    if result != 'Pass':
      not_passing.append((test, result))
  return not_passing


def load_expectation_dict(json_file):
  data = {}
  if os.path.isfile(json_file):
    print('Loading file ' + json_file)
    with open(json_file, 'r') as f:
      text = f.read()
      data = json.loads(text)
  return data


def load_expectations(json_file):
  data = load_expectation_dict(json_file)
  expectations = {}
  # Convert from dictionary of lists to dictionary of sets.
  for key in data:
    expectations[key] = set(data[key])
  return expectations


def expectation_list_to_dict(tests):
  data = {}
  tests = list(set(tests))
  for test, result in tests:
    if data.has_key(result):
      new_list = list(set(data[result].append(test)))
      data.pop(result)
      data[result] = new_list
    else:
      data[result] = [test]
  return data


def save_expectation_dict(expectation_path, expectation_dict):
  # Clean up obsolete expectations.
  for file_name in glob.glob(expectation_path + '.*'):
    if not '.hasty.' in file_name or '.hasty' in expectation_path:
      os.remove(file_name)
  # Dump json for next iteration.
  with open(expectation_path + '.json', 'w') as f:
    json.dump(expectation_dict,
              f,
              sort_keys=True,
              indent=4,
              separators=(',', ': '))
  # Dump plain text for autotest.
  for key in expectation_dict:
    if expectation_dict[key]:
      with open(expectation_path + '.' + key, 'w') as f:
        for test in expectation_dict[key]:
          f.write(test)
          f.write('\n')


# Figure out duplicates and move them to Flaky result set/list.
def process_flaky(status_dict):
  """Figure out duplicates and move them to Flaky result set/list."""
  clean_dict = {}
  flaky = set([])
  if status_dict.has_key('Flaky'):
    flaky = status_dict['Flaky']

  # FLaky tests are tests with 2 distinct results.
  for key1 in status_dict.keys():
    for key2 in status_dict.keys():
      if key1 != key2:
        flaky |= status_dict[key1] & status_dict[key2]

  # Remove Flaky tests from other status and convert to dict of list.
  for key in status_dict.keys():
    if key != 'Flaky':
      not_flaky = list(status_dict[key] - flaky)
      not_flaky.sort()
      print('Number of "%s" is %d.' % (key, len(not_flaky)))
      clean_dict[key] = not_flaky

  # And finally process flaky list/set.
  flaky_list = list(flaky)
  flaky_list.sort()
  clean_dict['Flaky'] = flaky_list

  return clean_dict


def merge_expectation_list(expectation_path, tests):
  status_dict = {}
  expectation_json = expectation_path + '.json'
  if os.access(expectation_json, os.R_OK):
    status_dict = load_expectations(expectation_json)
  else:
    print 'Could not load', expectation_json
  for test, result in tests:
    if status_dict.has_key(result):
      new_set = status_dict[result]
      new_set.add(test)
      status_dict.pop(result)
      status_dict[result] = new_set
    else:
      status_dict[result] = set([test])
  clean_dict = process_flaky(status_dict)
  save_expectation_dict(expectation_path, clean_dict)


def load_log(name):
  """Load test log and clean it from stderr spew."""
  with open(name) as f:
    lines = f.read().splitlines()
  text = ''
  for line in lines:
    if ('dEQP test filter =' in line or 'ChromeOS BOARD = ' in line or
        'ChromeOS CPU family =' in line or 'ChromeOS GPU family =' in line or
        'TestCase: ' in line or 'Result: ' in line or
        'Test Options: ' in line or 'Running in hasty mode.' in line or
        # For hasty logs we have:
        ' Pass (' in line or ' Fail (' in line or 'QualityWarning (' in line or
        ' Test case \'' in line):
      text += line + '\n'
  # TODO(ihf): Warn about or reject log files missing the end marker.
  return text


def process_logs(logs):
  for log in logs:
    text = load_log(log.name)
    if text:
      print('================================================================')
      print('Loading %s...' % log.name)
      _, gpu, filter, hasty = get_metadata(text)
      tests = get_all_tests(text)
      print('Found %d test results.' % len(tests))
      if tests:
        # GPU family goes first in path to simplify adding/deleting families.
        output_path = os.path.join(_EXPECTATIONS_DIR, gpu)
        if not os.access(output_path, os.R_OK):
          os.makedirs(output_path)
        expectation_path = os.path.join(output_path, filter)
        if hasty:
          expectation_path = os.path.join(output_path, filter + '.hasty')
        merge_expectation_list(expectation_path, tests)


argparser = argparse.ArgumentParser(
    description='Download from GS and process dEQP logs into expectations.')
argparser.add_argument(
    'result_ids',
    metavar='result_id',
    nargs='*',  # Zero or more result_ids specified.
    help='List of result log IDs (wildcards for gsutil like 5678* are ok).')
args = argparser.parse_args()

print pprint.pformat(args)
# This is somewhat optional. Remove existing expectations to start clean, but
# feel free to process them incrementally.
execute(['rm', '-rf', _EXPECTATIONS_DIR])
for id in args.result_ids:
  gs_path = _AUTOTEST_RESULT_TEMPLATE % id
  logs = get_logs_from_gs(gs_path)

# This will include the just downloaded logs from GS as well.
logs = get_local_logs()
process_logs(logs)
