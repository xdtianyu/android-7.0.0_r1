# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Control file generation for the autoupdate_EndToEnd server-side test."""

import os
import re

import common
from autotest_lib.client.common_lib import control_data


_name_re = re.compile('\s*NAME\s*=')
_autotest_test_name = 'autoupdate_EndToEndTest'


def generate_full_control_file(test, env, orig_control_code):
    """Returns the parameterized control file for the test config.

    @param test: the test config object (TestConfig)
    @param env: the test environment parameters (TestEnv or None)
    @param orig_control_code: string containing the template control code

    @returns Parameterized control file based on args (string)

    """
    orig_name = control_data.parse_control_string(orig_control_code).name
    code_lines = orig_control_code.splitlines()
    for i, line in enumerate(code_lines):
        if _name_re.match(line):
            new_name = '%s_%s' % (orig_name, test.unique_name_suffix())
            code_lines[i] = line.replace(orig_name, new_name)
            break

    env_code_args = env.get_code_args() if env else ''
    return test.get_code_args() + env_code_args + '\n'.join(code_lines) + '\n'


def dump_autotest_control_file(test, env, control_code, directory):
    """Creates control file for test and returns the path to created file.

    @param test: the test config object (TestConfig)
    @param env: the test environment parameters (TestEnv)
    @param control_code: string containing the template control code
    @param directory: the directory to dump the control file to

    @returns Path to the newly dumped control file

    """
    if not os.path.exists(directory):
        os.makedirs(directory)

    parametrized_control_code = generate_full_control_file(
            test, env, control_code)

    control_file = os.path.join(directory,
                                test.get_control_file_name())
    with open(control_file, 'w') as fh:
        fh.write(parametrized_control_code)

    return control_file


def get_test_name():
  """Returns the name of the server side test."""
  return _autotest_test_name


def get_control_file_name():
  """Returns the path of the end-to-end base control file."""
  return os.path.join(
      common.autotest_dir, 'server', 'site_tests',
      get_test_name(), 'control')
