# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import sys
from multiprocessing import Process

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome

policy_testserver = None


class EnterpriseTest(test.test):
    """Base class for policy tests."""

    USERNAME = 'autotest@managedchrome.com'
    PASSWORD = 'test0000'

    def import_dmserver(self, proto_path):
        """Import the DM testserver from chrome source.

        @param proto_path: location of proto files.
        """
        telemetry_src = '/usr/local/telemetry/src'
        sys.path.append(os.path.join(telemetry_src,
                                     'chrome/browser/policy/test'))
        sys.path.append(os.path.join(telemetry_src,
                                     'net/tools/testserver'))
        sys.path.append(os.path.join(telemetry_src,
                                     'third_party/protobuf/python/google'))
        sys.path.append(os.path.join(telemetry_src,
                                     'third_party/tlslite'))
        sys.path.append(proto_path)
        global policy_testserver
        import policy_testserver

    def start_dmserver(self):
        """Start the local DM testserver."""
        policy_server_runner = policy_testserver.PolicyServerRunner()
        self._policy_location = os.path.join(self.tmpdir, 'policy.json')
        port = utils.get_unused_port()
        # The first argument is always ignored since it is expected to be the
        # path to the executable. Hence passing an empty string for first
        # argument.
        sys.argv = ['',
                    '--config-file=%s' % self._policy_location,
                    '--host=127.0.0.1',
                    '--log-file=%s/dm_server.log' % self.debugdir,
                    '--log-level=DEBUG',
                    '--port=%d' % port
                   ]
        self.dm_server = Process(target=policy_server_runner.main)
        self.dm_server.start()
        self.dm_server_url = 'http://127.0.0.1:%d/' % port

    def stop_dmserver(self):
        """Stop the local DM server."""
        os.system('wget %sconfiguration/test/exit' % self.dm_server_url)
        self.dm_server.join()

    def initialize(self):
        self.start_dmserver()

    def cleanup(self):
        self.stop_dmserver()

    def setup_policy(self, policy_blob):
        """Write policy blob to file used by the DM server to read policy.

        @param policy_blob: JSON policy blob to be written to the policy file.
        """
        with open(self._policy_location, 'w') as f:
            f.write(policy_blob)

    def create_chrome(self,
                      autotest_ext=False,
                      extra_browser_args='',
                      username=None,
                      password=None):
        """Create an instance of chrome for enterprise testing.

        @param autotest_ext: Load a component extension with privileges to
                             invoke chrome.autotestPrivate.
        @param extra_browser_args: Additional argument(s) to pass to the
                                   browser. It should be a string.
        @param username: Log in using this username instead of the default.
        @param password: Log in using this password instead of the default.

        @return: A telemetry browser instance.
        """
        extra_browser_args = (extra_browser_args +
                              '--device-management-url=%s ' %
                              self.dm_server_url +
                              '--enterprise-enrollment-skip-robot-auth')
        username = self.USERNAME if username is None else username
        password = self.PASSWORD if password is None else password
        return chrome.Chrome(extra_browser_args=extra_browser_args,
                             autotest_ext=autotest_ext,
                             disable_gaia_services=False,
                             gaia_login=True,
                             username=username,
                             password=password)
