#!/usr/bin/python

# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Method to add or modify ATTRIBUTES in the test control files whose
ATTRIBUTES either not match to SUITE or not in the attribute whitelist."""

import argparse
import logging
import os
import sys

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.server.cros.dynamic_suite.suite import Suite


def main(argv):
  """main scripts to seed attributes in test control files.

  Args:
    @param argv: Command line arguments including `sys.argv[0]`.
  """
  # Parse execution cmd
  parser = argparse.ArgumentParser(
      description='Seed ATTRIBUTES in test control files.')
  parser.add_argument('--execute', action='store_true', default=False,
                      help='Execute the script to seed attributes in all '
                           'test control files.')
  args = parser.parse_args(argv)

  # When execute is True, run the script to seed attributes in control files.
  if args.execute:
    # Get the whitelist path, hardcode the path currently
    path_whitelist = os.path.join(common.autotest_dir,
                                  'site_utils/attribute_whitelist.txt')

    # Go through all control file, check whether attribute matches suite. Return
    # a changelist which contains the paths to the control files not match.
    fs_getter = Suite.create_fs_getter(common.autotest_dir)
    changelist = AttrSuiteMatch(
        fs_getter.get_control_file_list(), path_whitelist)
    count = len(changelist)

    logging.info('Starting to seed attributes in %d control files...' % count)
    # Modify attributes based on suite for the control files not match.
    for path in changelist:
      logging.info('Seeding ATTRIBUTES in %s' % path)
      count = count - 1
      logging.info('%d files remaining...' % count)
      SeedAttributes(path)

    logging.info('Finished seeding attributes.')

  # When not specify 'execute' in cmd, not modify control files.
  else:
    logging.info('No files are modified. To seed attributes in control files, '
                 'please add \'--execute\' argument when run the script.')


def AttrSuiteMatch(path_list, path_whitelist):
  """Check whether attributes are in the attribute whitelist and match with the
  suites in the control files.

  Args:
    @param path_list: a list of path to the control files to be checked.
    @param path_whitelist: path to the attribute whitelist.

  Returns:
    A list of paths to the control files that failed at checking.
  """
  unmatch_pathlist = []

  # Read the whitelist to a set, if path is invalid, throw IOError.
  with open(path_whitelist, 'r') as f:
    whitelist = {line.strip() for line in f.readlines() if line.strip()}

  # Read the attr in the control files, check with whitelist and suite.
  for path in path_list:
    cd = control_data.parse_control(path, True)
    cd_attrs = cd.attributes

    # Test whether attributes in the whitelist
    if not (whitelist >= cd_attrs):
      unmatch_pathlist.append(path)
    # Test when suite exists, whether attributes match suites
    if hasattr(cd, 'suite'):
      target_attrs = set(
            'suite:' + x.strip() for x in cd.suite.split(',') if x.strip())
      if cd_attrs != target_attrs:
          unmatch_pathlist.append(path)
    # Test when suite not exists, whether attributes is empty
    elif not hasattr(cd, 'suite') and cd_attrs:
      unmatch_pathlist.append(path)

  return unmatch_pathlist


def SeedAttributes(path_controlfile):
  """Seed attributes in a control file.

  Read and re-write a control file with modified contents with attributes added.

  Args:
    @param path_controlfile: path to control file

  Returns:
    None
  """
  # Parse attribute from suite, and prepare ATTRIBUTES line.
  cd = control_data.parse_control(path_controlfile, True)
  suite = cd.suite

  attr_items = set(
      'suite:' + x.strip() for x in suite.split(',') if x.strip())
  attr_items = list(attr_items)
  attr_items.sort(key = str.lower)
  attr_line = ', '.join(attr_items)
  attr_line = 'ATTRIBUTES = \"' + attr_line + '\"\n'

  # Read control file and modify the suite line with attribute added.
  with open(path_controlfile, 'r') as f:
    lines = f.readlines()
    index = [i for i, val in enumerate(lines) if val.startswith('SUITE =') or
             val.startswith('SUITE=')][0]
    suite_line = lines[index]
    lines[index] = attr_line + suite_line

  # Write the modified contents back to file
  with open(path_controlfile, 'w') as f:
    f.writelines(lines)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
