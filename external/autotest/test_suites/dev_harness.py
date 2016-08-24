# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast, os, random

from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import frontend
from autotest_lib.server.cros.dynamic_suite import constants, tools

"""A wrapper around a dynamic suite control file to enable local testing.

usage: server/autoserv test_suites/dev_harness.py

dev_harness.py pulls in a suite control file from disk and injects all the
variables that are normally injected by the create_suite_job() RPC, without
which the suite cannot be successfully executed.

This file can be passed directly to autoserv and executed locally,
allowing for the testing of test suite control file changes without
staging them in the context of a build.  It also means that one can
test code changes in the dynamic_suite infrastructure without being
forced to reimage the DUTs every time (by setting SKIP_IMAGE to
False).
"""

# __file__ is not defined when this is run by autoserv, so hard-code :-/
CONF_FILE = '/usr/local/autotest/test_suites/dev_harness_conf'
SUITE_CONTROL_FILE = 'test_suites/control.dummy'
config_dict = {}
new_globals = globals()
if os.path.exists(CONF_FILE):
    with open(CONF_FILE, 'r') as config:
        config_dict.update(ast.literal_eval(config.read()))
else:
    # Default values.  Put a raw dict into CONF_FILE to override.
    config_dict = {'build': 'x86-mario-release/R23-2814.0.0',
                   'board': 'x86-mario',
                   'check_hosts': True,
                   'pool': None,
                   'num': 2,
                   'file_bugs': False,
                   'SKIP_IMAGE': False}
new_globals.update(config_dict)

ds = dev_server.ImageServer.resolve(new_globals['build'])
if new_globals['SKIP_IMAGE']:
    AFE = frontend.AFE(debug=True)

    repo_url = tools.package_url_pattern() % (ds.url(), new_globals['build'])
    m = random.choice(AFE.get_hostnames(label='board:'+new_globals['board']))

    label = AFE.get_labels(
        name=constants.VERSION_PREFIX+new_globals['build'])[0]
    label.add_hosts([m])
    AFE.set_host_attribute(constants.JOB_REPO_URL, repo_url, hostname=m)
else:
    ds.trigger_download(new_globals['build'])
new_globals['devserver_url'] = ds.url()

execfile(os.path.join(job.autodir, SUITE_CONTROL_FILE), new_globals, locals())
