#!/usr/bin/python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from __future__ import print_function
from collections import namedtuple
import json, os, re, sys

AUTOTEST_NAME = 'graphics_PiglitBVT'
INPUT_DIR = './piglit_logs/'
OUTPUT_DIR = './test_scripts/'
OUTPUT_FILE_PATTERN = OUTPUT_DIR + '/%s/' + AUTOTEST_NAME + '_%d.sh'
OUTPUT_FILE_SLICES = 20
PIGLIT_PATH = '/usr/local/piglit/lib/piglit/'
PIGLIT64_PATH = '/usr/local/piglit/lib64/piglit/'

# Do not generate scripts with "bash -e" as we want to handle errors ourself.
FILE_HEADER = '#!/bin/bash\n\n'

# Script fragment function that kicks off individual piglit tests.
FILE_RUN_TEST = '\n\
function run_test()\n\
{\n\
  local name="$1"\n\
  local time="$2"\n\
  local command="$3"\n\
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"\n\
  echo "+ Running test [$name] of expected runtime $time sec: [$command]"\n\
  sync\n\
  $command\n\
  if [ $? == 0 ] ; then\n\
    let "need_pass--"\n\
    echo "+ pass :: $name"\n\
  else\n\
    let "failures++"\n\
    echo "+ fail :: $name"\n\
  fi\n\
}\n\
'

# Script fragment that sumarizes the overall status.
FILE_SUMMARY = 'popd\n\
\n\
if [ $need_pass == 0 ] ; then\n\
  echo "+---------------------------------------------+"\n\
  echo "| Overall pass, as all %d tests have passed. |"\n\
  echo "+---------------------------------------------+"\n\
else\n\
  echo "+-----------------------------------------------------------+"\n\
  echo "| Overall failure, as $need_pass tests did not pass and $failures failed. |"\n\
  echo "+-----------------------------------------------------------+"\n\
fi\n\
exit $need_pass\n\
'

# Control file template for executing a slice.
CONTROL_FILE = "\
# Copyright 2014 The Chromium OS Authors. All rights reserved.\n\
# Use of this source code is governed by a BSD-style license that can be\n\
# found in the LICENSE file.\n\
\n\
NAME = '" + AUTOTEST_NAME + "'\n\
AUTHOR = 'chromeos-gfx'\n\
PURPOSE = 'Collection of automated tests for OpenGL implementations.'\n\
CRITERIA = 'All tests in a slice have to pass, otherwise it will fail.'\n\
TIME='SHORT'\n\
TEST_CATEGORY = 'Functional'\n\
TEST_CLASS = 'graphics'\n\
TEST_TYPE = 'client'\n\
JOB_RETRIES = 2\n\
\n\
BUG_TEMPLATE = {\n\
    'labels': ['Cr-OS-Kernel-Graphics'],\n\
}\n\
\n\
DOC = \"\"\"\n\
Piglit is a collection of automated tests for OpenGL implementations.\n\
\n\
The goal of Piglit is to help improve the quality of open source OpenGL drivers\n\
by providing developers with a simple means to perform regression tests.\n\
\n\
This control file runs slice %d out of %d slices of a passing subset of the\n\
original collection.\n\
\n\
http://piglit.freedesktop.org\n\
\"\"\"\n\
\n\
job.run_test('" + AUTOTEST_NAME + "', test_slice=%d)\
"

def output_control_file(sl, slices):
  """
  Write control file for slice sl to disk.
  """
  filename = 'control.%d' % sl
  with open(filename, 'w+') as f:
    print(CONTROL_FILE % (sl, slices, sl), file=f)


def append_script_header(f, need_pass, piglit_path):
  """
  Write the beginning of the test script to f.
  """
  print(FILE_HEADER, file=f)
  # need_pass is the script variable that counts down to zero and gets returned.
  print('need_pass=%d' % need_pass, file=f)
  print('failures=0', file=f)
  print('PIGLIT_PATH=%s' % piglit_path, file=f)
  print('export PIGLIT_SOURCE_DIR=%s' % piglit_path, file=f)
  print('export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIGLIT_PATH/lib', file=f)
  print('export DISPLAY=:0', file=f)
  print('export XAUTHORITY=/home/chronos/.Xauthority', file=f)
  print('', file=f)
  print(FILE_RUN_TEST, file=f)
  print('', file=f)
  print('pushd $PIGLIT_PATH', file=f)


def append_script_summary(f, need_pass):
  """
  Append the summary to the test script f with a required pass count.
  """
  print(FILE_SUMMARY % need_pass, file=f)


def mkdir_p(path):
  """
  Create all directories in path.
  """
  try:
    os.makedirs(path)
  except OSError:
    if os.path.isdir(path):
      pass
    else:
      raise

def get_filepaths(family_root, regex):
  """
  Find all files that were placed into family_root.
  Used to find regular log files (*results.json) and expectations*.json.
  """
  main_files = []
  for root, _, files in os.walk(family_root):
    for filename in files:
      if re.search(regex, filename):
        main_files.append(os.path.join(root, filename))
  return main_files


def load_files(main_files):
  """
  The log files are just python dictionaries, load them from disk.
  """
  d = {}
  for main_file in main_files:
    d[main_file] = json.loads(open(main_file).read())
  return d


# Define a Test data structure containing the command line and runtime.
Test = namedtuple('Test', 'command time passing_count not_passing_count')

def get_test_statistics(log_dict):
  """
  Figures out for each test how often is passed/failed, the command line and
  how long it runs.
  """
  statistics = {}
  for main_file in log_dict:
    for test in log_dict[main_file]['tests']:
      # Initialize for all known test names to zero stats.
      statistics[test] = Test(None, 0.0, 0, 0)

  for main_file in log_dict:
    print('Updating statistics from %s.' % main_file, file=sys.stderr)
    tests = log_dict[main_file]['tests']
    for test in tests:
      command = statistics[test].command
      # Verify that each board uses the same command.
      if 'command' in tests[test]:
        if command:
          assert(command == tests[test]['command'])
        else:
          command = tests[test]['command']
      # Bump counts.
      if tests[test]['result'] == 'pass':
        statistics[test] = Test(command,
                                max(tests[test]['time'],
                                    statistics[test].time),
                                statistics[test].passing_count + 1,
                                statistics[test].not_passing_count)
      else:
        statistics[test] = Test(command,
                                statistics[test].time,
                                statistics[test].passing_count,
                                statistics[test].not_passing_count + 1)

  return statistics


def get_max_passing(statistics):
  """
  Gets the maximum count of passes a test has.
  """
  max_passing_count = 0
  for test in statistics:
    max_passing_count = max(statistics[test].passing_count, max_passing_count)
  return max_passing_count


def get_passing_tests(statistics, expectations):
  """
  Gets a list of all tests that never failed and have a maximum pass count.
  """
  tests = []
  max_passing_count = get_max_passing(statistics)
  for test in statistics:
    if (statistics[test].passing_count == max_passing_count and
        statistics[test].not_passing_count == 0):
      if test not in expectations:
        tests.append(test)
  return sorted(tests)


def get_intermittent_tests(statistics):
  """
  Gets tests that failed at least once and passed at least once.
  """
  tests = []
  max_passing_count = get_max_passing(statistics)
  for test in statistics:
    if (statistics[test].passing_count > 0 and
        statistics[test].passing_count < max_passing_count and
        statistics[test].not_passing_count > 0):
      tests.append(test)
  return sorted(tests)


def cleanup_command(cmd, piglit_path):
  """
  Make script less location dependent by stripping path from commands.
  """
  cmd = cmd.replace(piglit_path, '')
  cmd = cmd.replace('framework/../', '')
  cmd = cmd.replace('tests/../', '')
  return cmd

def process_gpu_family(family, family_root):
  """
  This takes a directory with log files from the same gpu family and processes
  the result log into |slices| runable scripts.
  """
  print('--> Processing "%s".' % family, file=sys.stderr)
  piglit_path = PIGLIT_PATH
  if family == 'other':
    piglit_path = PIGLIT64_PATH

  log_dict = load_files(get_filepaths(family_root, 'results\.json$'))
  # Load all expectations but ignore suggested.
  exp_dict = load_files(get_filepaths(family_root, 'expectations.*\.json$'))
  statistics = get_test_statistics(log_dict)
  expectations = compute_expectations(exp_dict, statistics, family, piglit_path)
  # Try to help the person updating piglit by collecting the variance
  # across different log files into one expectations file per family.
  output_suggested_expectations(expectations, family, family_root)

  # Now start computing the new test scripts.
  passing_tests = get_passing_tests(statistics, expectations)

  slices = OUTPUT_FILE_SLICES
  current_slice = 1
  slice_tests = []
  time_slice = 0
  num_processed = 0
  num_pass_total = len(passing_tests)
  time_total = 0
  for test in passing_tests:
    time_total += statistics[test].time

  # Generate one script containing all tests. This can be used as a simpler way
  # to run everything, but also to have an easier diff when updating piglit.
  filename = OUTPUT_FILE_PATTERN % (family, 0)
  # Ensure the output directory for this family exists.
  mkdir_p(os.path.dirname(os.path.realpath(filename)))
  if passing_tests:
    with open(filename, 'w+') as f:
      append_script_header(f, num_pass_total, piglit_path)
      for test in passing_tests:
        cmd = cleanup_command(statistics[test].command, piglit_path)
        time_test = statistics[test].time
        print('run_test "%s" %.1f "%s"' % (test, 0.0, cmd), file=f)
      append_script_summary(f, num_pass_total)

  # Slice passing tests into several pieces to get below BVT's 20 minute limit.
  # TODO(ihf): If we ever get into the situation that one test takes more than
  # time_total / slice we would get an empty slice afterward. Fortunately the
  # stderr spew should warn the operator of this.
  for test in passing_tests:
    # We are still writing all the tests that belong in the current slice.
    if time_slice < time_total / slices:
      slice_tests.append(test)
      time_test = statistics[test].time
      time_slice += time_test
      num_processed += 1

    # We finished the slice. Now output the file with all tests in this slice.
    if time_slice >= time_total / slices or num_processed == num_pass_total:
      filename = OUTPUT_FILE_PATTERN % (family, current_slice)
      with open(filename, 'w+') as f:
        need_pass = len(slice_tests)
        append_script_header(f, need_pass, piglit_path)
        for test in slice_tests:
          # Make script less location dependent by stripping path from commands.
          cmd = cleanup_command(statistics[test].command, piglit_path)
          time_test = statistics[test].time
          # TODO(ihf): Pass proper time_test instead of 0.0 once we can use it.
          print('run_test "%s" %.1f "%s"'
                % (test, 0.0, cmd), file=f)
        append_script_summary(f, need_pass)
        output_control_file(current_slice, slices)

      print('Slice %d: max runtime for %d passing tests is %.1f seconds.'
            % (current_slice, need_pass, time_slice), file=sys.stderr)
      current_slice += 1
      slice_tests = []
      time_slice = 0

  print('Total max runtime on "%s" for %d passing tests is %.1f seconds.' %
          (family, num_pass_total, time_total), file=sys.stderr)


def insert_expectation(expectations, test, expectation):
  """
  Insert test with expectation into expectations directory.
  """
  if not test in expectations:
    # Just copy the whole expectation.
    expectations[test] = expectation
  else:
    # Copy over known fields one at a time but don't overwrite existing.
    expectations[test]['result'] = expectation['result']
    if (not 'crbug' in expectations[test] and 'crbug' in expectation):
      expectations[test]['crbug'] = expectation['crbug']
    if (not 'comment' in expectations[test] and 'comment' in expectation):
      expectations[test]['comment'] = expectation['comment']
    if (not 'command' in expectations[test] and 'command' in expectation):
      expectations[test]['command'] = expectation['command']
    if (not 'pass rate' in expectations[test] and 'pass rate' in expectation):
      expectations[test]['pass rate'] = expectation['pass rate']


def compute_expectations(exp_dict, statistics, family, piglit_path):
  """
  Analyze intermittency and output suggested test expectations.
  The suggested test expectation
  Test expectations are dictionaries with roughly the same structure as logs.
  """
  flaky_tests = get_intermittent_tests(statistics)
  print('Encountered %d tests that do not always pass in "%s" logs.' %
        (len(flaky_tests), family), file=sys.stderr)

  max_passing = get_max_passing(statistics)
  expectations = {}
  # Merge exp_dict which we loaded from disk into new expectations.
  for filename in exp_dict:
    for test in exp_dict[filename]['tests']:
      expectation = exp_dict[filename]['tests'][test]
      # Historic results not considered flaky as pass rate makes no sense
      # without current logs.
      expectation['result'] = 'skip'
      if 'pass rate' in expectation:
        expectation.pop('pass rate')
      # Overwrite historic commands with recently observed ones.
      if test in statistics:
        expectation['command'] = cleanup_command(statistics[test].command,
                                                 piglit_path)
        insert_expectation(expectations, test, expectation)
      else:
        print ('Historic test [%s] not found in new logs. '
               'Dropping it from expectations.' % test, file=sys.stderr)

  # Handle the computed flakiness from the result logs that we just processed.
  for test in flaky_tests:
    pass_rate = statistics[test].passing_count / float(max_passing)
    command = statistics[test].command
    # Loading a json converts everything to string anyways, so save it as such
    # and make it only 2 significiant digits.
    expectation = {'result': 'flaky',
                   'pass rate': '%.2f' % pass_rate,
                   'command': command}
    insert_expectation(expectations, test, expectation)

  return expectations


def output_suggested_expectations(expectations, family, family_root):
  filename = os.path.join(family_root,
                          'suggested_exp_to_rename_%s.json' % family)
  with open(filename, 'w+') as f:
    json.dump({'tests': expectations}, f, indent=2, sort_keys=True,
              separators=(',', ': '))


def get_gpu_families(root):
  """
  We consider each directory under root a possible gpu family.
  """
  files = os.listdir(root)
  families = []
  for f in files:
    if os.path.isdir(os.path.join(root, f)):
      families.append(f)
  return families


def generate_scripts(root):
  """
  For each family under root create the corresponding set of passing test
  scripts.
  """
  families = get_gpu_families(root)
  for family in families:
    process_gpu_family(family, os.path.join(root, family))


# We check the log files in as highly compressed binaries.
print('Uncompressing log files...', file=sys.stderr)
os.system('bunzip2 ' + INPUT_DIR + '/*/*/*results.json.bz2')

# Generate the scripts.
generate_scripts(INPUT_DIR)

# Binary should remain the same, otherwise use
#   git checkout -- piglit_output
# or similar to reverse.
print('Recompressing log files...', file=sys.stderr)
os.system('bzip2 -9 ' + INPUT_DIR + '/*/*/*results.json')
