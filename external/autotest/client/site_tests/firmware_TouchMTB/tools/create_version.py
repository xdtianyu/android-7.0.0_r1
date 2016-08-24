#!/usr/bin/python -u
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module creates the version information of this firmware test suite.

This module is expected to be executed from the chroot of a host so that
it could extract the git repo information to create the version information.

The version file contains a pair of a date-time string and a hash like

  version: 2014-01-23.16:18:24 1feddb865cf3f9c158d8cca17a6e68c71b500571

where the hash is the git hash of the latest patch that satisfies
both conditions:
  (1) it could be found in cros/master remote branch on the host, and
  (2) it modifies firmware_TouchMTB test,
and the date-time string is the commit time of the above latest patch in the
format of yyyy-mm-dd.hh:mm:ss.

If the there are any commits on the current local branch on top of the
above latest patch that have modified this test, the version information
will be shown like

version: revised since 2014-01-23.16:18:24 1feddb865cf3f9c158d8cca17a6e68c71b500
571

We use "revised since ..." to indicate that some modifications have been made
since the above latest patch.
"""


import os
import re
import sys

import common
from common_util import simple_system_output


VERSION_FILENAME = '/tmp/.version'

LOCAL_BR = 'HEAD'
TOT_BR = 'cros/master'


def _run_git_cmd(cmd):
    """Run git command.

    When a server test is invoked, the present working directory may look
    like "/tmp/test_that.xxxx/..."  To be able to run git commands
    against the autotest repo, it is required to pushd to autotest project
    temporarily.
    """
    new_cmd = ('pushd %s > /dev/null; %s; popd > /dev/null' %
               (common.client_dir, cmd))
    return simple_system_output(new_cmd)


def _get_common_ancestor(branch_a, branch_b):
    """Get the nearest common ancestor of the given branches."""
    return _run_git_cmd('git merge-base %s %s' % (branch_a, branch_b))


def _get_first_commit_matching_pattern(commit, pattern):
    """Get the first commit previous to this commit that matches the pattern."""
    cmd = 'git log {} --grep="{}" --pretty=format:"%H" -1'
    return _run_git_cmd(cmd.format(commit, pattern))


def _get_git_commit(branch):
    return _run_git_cmd('git rev-parse %s' % branch)


def _get_date_time(commit):
    """Get the commit date time in ISO 8601 format."""
    cmd = 'git log {} --pretty=format:"%ci" -1'
    ts = _run_git_cmd(cmd.format(commit))
    date, time, _ = ts.split()
    return '%s.%s' % (date, time)


def create_version_file(version_filename):
    file_path = os.path.dirname(os.path.abspath(sys.modules[__name__].__file__))
    result = re.search('site_tests\/(.*?)\/', file_path)
    if result is None:
        print 'Failed to find the test name.'
    test_name = result.group(1)
    common_ancestor = _get_common_ancestor(LOCAL_BR, TOT_BR)
    latest_test_commit = _get_first_commit_matching_pattern(common_ancestor,
                                                            test_name)
    date_time_str = _get_date_time(latest_test_commit)

    local_commit = _get_git_commit(LOCAL_BR)
    if local_commit == latest_test_commit:
        version_template = 'version: %s %s '
    else:
        version_template = 'version: revised since %s %s '

    version_info = version_template % (date_time_str, latest_test_commit)
    with open(version_filename, 'w') as version_file:
        version_file.write(version_info)


if __name__ == '__main__':
    version_filename = sys.argv[1] if len(sys.argv) > 1 else VERSION_FILENAME
    create_version_file(version_filename)
