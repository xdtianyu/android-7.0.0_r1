# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros import enterprise_base
from autotest_lib.client.cros import httpd

CROSDEV_FLAGS = [
    '--gaia-url=https://gaiastaging.corp.google.com',
    '--lso-url=https://gaiastaging.corp.google.com',
    '--google-apis-url=https://www-googleapis-test.sandbox.google.com',
    '--oauth2-client-id=236834563817.apps.googleusercontent.com',
    '--oauth2-client-secret=RsKv5AwFKSzNgE0yjnurkPVI',
    ('--cloud-print-url='
     'https://cloudprint-nightly-ps.sandbox.google.com/cloudprint'),
    '--ignore-urlfetcher-cert-requests']
CROSAUTO_FLAGS = [
    ('--cloud-print-url='
     'https://cloudprint-nightly-ps.sandbox.google.com/cloudprint'),
    '--ignore-urlfetcher-cert-requests']
TESTDMS_FLAGS = [
    '--ignore-urlfetcher-cert-requests',
    '--enterprise-enrollment-skip-robot-auth',
    '--disable-policy-key-verification']
FLAGS_DICT = {
    'prod': [],
    'cr-dev': CROSDEV_FLAGS,
    'cr-auto': CROSAUTO_FLAGS,
    'dm-test': TESTDMS_FLAGS,
    'dm-fake': TESTDMS_FLAGS
}
DMS_URL_DICT = {
    'prod': 'http://m.google.com/devicemanagement/data/api',
    'cr-dev': 'https://cros-dev.sandbox.google.com/devicemanagement/data/api',
    'cr-auto': 'https://cros-auto.sandbox.google.com/devicemanagement/data/api',
    'dm-test': 'http://chromium-dm-test.appspot.com/d/%s',
    'dm-fake': 'http://127.0.0.1:%d/'
}
DMSERVER = '--device-management-url=%s'


class EnterprisePolicyTest(enterprise_base.EnterpriseTest):
    """Base class for Enterprise Policy Tests."""

    def setup(self):
        os.chdir(self.srcdir)
        utils.make()

    def initialize(self, args=()):
        self._initialize_test_context(args)

        # Start AutoTest DM Server if using local fake server.
        if self.env == 'dm-fake':
            self.import_dmserver(self.srcdir)
            self.start_dmserver()
        self._initialize_chrome_extra_flags()
        self._web_server = None

    def cleanup(self):
        # Clean up AutoTest DM Server if using local fake server.
        if self.env == 'dm-fake':
            super(EnterprisePolicyTest, self).cleanup()

        # Stop web server if it was started.
        if self._web_server:
            self._web_server.stop()

        # Close Chrome instance if opened.
        if self.cr:
            self.cr.close()

    def start_webserver(self, port):
        """Set up an HTTP Server to serve pages from localhost.

        @param port: Port used by HTTP server.

        """
        if self.mode != 'list':
            self._web_server = httpd.HTTPListener(port, docroot=self.bindir)
            self._web_server.run()

    def _initialize_test_context(self, args=()):
        """Initialize class-level test context parameters.

        @raises error.TestError if an arg is given an invalid value or some
                combination of args is given incompatible values.

        """
        # Extract local parameters from command line args.
        args_dict = utils.args_to_dict(args)
        self.mode = args_dict.get('mode', 'all')
        self.case = args_dict.get('case')
        self.value = args_dict.get('value')
        self.env = args_dict.get('env', 'dm-fake')
        self.username = args_dict.get('username')
        self.password = args_dict.get('password')
        self.dms_name = args_dict.get('dms_name')

        # If |mode| is 'list', set |env| to generic 'prod', and blank out
        # the other key parameters: case, value.
        if self.mode == 'list':
            self.env = 'prod'
            self.case = None
            self.value = None

        # If |case| is given then set |mode| to 'single'.
        if self.case:
            self.mode = 'single'

        # If |mode| is 'all', then |env| must be 'dm-fake', and
        # the |case| and |value| args must not be given.
        if self.mode == 'all':
            if self.env != 'dm-fake':
                raise error.TestError('env must be "dm-fake" '
                                      'when mode=all.')
            if self.case:
                raise error.TestError('case must not be given '
                                      'when mode=all.')
            if self.value:
                raise error.TestError('value must not be given '
                                      'when mode=all.')

        # If |value| is given, set |is_value_given| flag to True. If it
        # was given as 'none', 'null', or '', then set |value| to 'null'.
        if self.value is not None:
            self.is_value_given = True
            if (self.value.lower() == 'none' or
                self.value.lower() == 'null' or
                self.value == ''):
                self.value = 'null'
        else:
            self.is_value_given = False

        # Verify |env| is a valid environment.
        if self.env is not None and self.env not in FLAGS_DICT:
            raise error.TestError('env=%s is invalid.' % self.env)

        # If |env| is 'dm-fake', ensure value and credentials are not given.
        if self.env == 'dm-fake':
            if self.is_value_given:
                raise error.TestError('value must not be given when using '
                                      'the fake DM Server.')
            if self.username or self.password:
                raise error.TestError('user credentials must not be given '
                                      'when using the fake DM Server.')

        # If either credential is not given, set both to default.
        if self.username is None or self.password is None:
            self.username = self.USERNAME
            self.password = self.PASSWORD

        # Verify |case| is given if |mode|==single.
        if self.mode == 'single' and not self.case:
            raise error.TestError('case must be given when mode is single.')

        # Verify |case| is given if a |value| is given.
        if self.is_value_given and self.case is None:
            raise error.TestError('value must not be given without also '
                                  'giving a test case.')

        # Verify |dms_name| is given iff |env|==dm-test.
        if self.env == 'dm-test' and not self.dms_name:
            raise error.TestError('dms_name must be given when using '
                                  'env=dm-test.')
        if self.env != 'dm-test' and self.dms_name:
            raise error.TestError('dms_name must not be given when not using '
                                  'env=dm-test.')

        # Log the test context parameters.
        logging.info('Test Context Parameters:')
        logging.info('  Run Mode: %r', self.mode)
        logging.info('  Test Case: %r', self.case)
        logging.info('  Expected Value: %r', self.value)
        logging.info('  Environment: %r', self.env)
        logging.info('  Username: %r', self.username)
        logging.info('  Password: %r', self.password)
        logging.info('  Test DMS Name: %r', self.dms_name)

    def _initialize_chrome_extra_flags(self):
        """Initialize flags used to create Chrome instance."""
        # Construct DM Server URL flags.
        env_flag_list = []
        if self.env != 'prod':
            if self.env == 'dm-fake':
                # Use URL provided by AutoTest DM server.
                dmserver_str = (DMSERVER % self.dm_server_url)
            else:
                # Use URL defined in DMS URL dictionary.
                dmserver_str = (DMSERVER % (DMS_URL_DICT[self.env]))
                if self.env == 'dm-test':
                    dmserver_str = (dmserver_str % self.dms_name)

            # Merge with other flags needed by non-prod enviornment.
            env_flag_list = ([dmserver_str] + FLAGS_DICT[self.env])

        self.extra_flags = env_flag_list
        self.cr = None

    def setup_case(self, policy_name, policy_value, policies_json):
        """Set up and confirm the preconditions of a test case.

        If the AutoTest fake DM Server is initialized, make a policy blob
        from |policies_json|, and upload it to the fake server.

        Launch a chrome browser, and sign in to Chrome OS. Examine the user's
        cryptohome vault, to confirm it signed in successfully.

        Open the Policies page, and confirm that it shows the specified
        |policy_name| and has the correct |policy_value|.

        @param policy_name: Name of the policy under test.
        @param policy_value: Expected value to appear on chrome://policy page.
        @param policies_json: JSON string to set up the fake DMS policy value.

        @raises error.TestError if cryptohome vault is not mounted for user.
        @raises error.TestFail if |policy_name| and |policy_value| are not
                shown on the Policies page.

        """
        # Set up policy on AutoTest DM Server only if initialized.
        if self.env == 'dm-fake':
            self.setup_policy(self._make_json_blob(policies_json))

        self._launch_chrome_browser()
        tab = self.navigate_to_url('chrome://policy')
        if not cryptohome.is_vault_mounted(user=self.username,
                                           allow_fail=True):
            raise error.TestError('Expected to find a mounted vault for %s.'
                                  % self.username)
        value_shown = self._get_policy_value_shown(tab, policy_name)
        if not self._policy_value_matches_shown(policy_value, value_shown):
            raise error.TestFail('Policy value shown is not correct: %s '
                                 '(expected: %s)' %
                                 (value_shown, policy_value))
        tab.Close()

    def _launch_chrome_browser(self):
        """Launch Chrome browser and sign in."""
        logging.info('Chrome Browser Arguments:')
        logging.info('  extra_browser_args: %s', self.extra_flags)
        logging.info('  username: %s', self.username)
        logging.info('  password: %s', self.password)

        self.cr = chrome.Chrome(extra_browser_args=self.extra_flags,
                                username=self.username,
                                password=self.password,
                                gaia_login=True,
                                disable_gaia_services=False,
                                autotest_ext=True)

    def navigate_to_url(self, url, tab=None):
        """Navigate tab to the specified |url|. Create new tab if none given.

        @param url: URL of web page to load.
        @param tab: browser tab to load (if any).
        @returns: browser tab loaded with web page.

        """
        logging.info('Navigating to URL: %r', url)
        if not tab:
            tab = self.cr.browser.tabs.New()
            tab.Activate()
        tab.Navigate(url, timeout=5)
        tab.WaitForDocumentReadyStateToBeComplete()
        return tab

    def _policy_value_matches_shown(self, policy_value, value_shown):
        """Compare |policy_value| to |value_shown| with whitespace removed.

        Compare the expected policy value with the value actually shown on the
        chrome://policies page. Before comparing, convert both values to JSON
        formatted strings, and remove all whitespace. Whitespace is removed
        because Chrome processes some policy values to show them in a more
        human readable format.

        @param policy_value: Expected value to appear on chrome://policy page.
        @param value_shown: Value as it appears on chrome://policy page.
        @param policies_json: JSON string to set up the fake DMS policy value.

        @returns: True if the strings match after removing all whitespace.

        """
        # Convert Python None or '' to JSON formatted 'null' string.
        if value_shown is None or value_shown == '':
            value_shown = 'null'
        if policy_value is None or policy_value == '':
            policy_value = 'null'

        # Remove whitespace.
        trimmed_value = ''.join(policy_value.split())
        trimmed_shown = ''.join(value_shown.split())
        logging.info('Trimmed policy value shown: %r (expected: %r)',
                     trimmed_shown, trimmed_value)
        return trimmed_value == trimmed_shown

    def _make_json_blob(self, policies_json):
        """Create policy blob from policies JSON object.

        @param policies_json: Policies JSON object (name-value pairs).
        @returns: Policy blob to be used to setup the policy server.

        """
        policies_json = self._move_modeless_to_mandatory(policies_json)
        policies_json = self._remove_null_policies(policies_json)

        policy_blob = """{
            "google/chromeos/user": %s,
            "managed_users": ["*"],
            "policy_user": "%s",
            "current_key_index": 0,
            "invalidation_source": 16,
            "invalidation_name": "test_policy"
        }""" % (json.dumps(policies_json), self.USERNAME)
        return policy_blob

    def _move_modeless_to_mandatory(self, policies_json):
        """Add the 'mandatory' mode if a policy's mode was omitted.

        The AutoTest fake DM Server requires that every policy be contained
        within either a 'mandatory' or 'recommended' dictionary, to indicate
        the mode of the policy. This function moves modeless policies into
        the 'mandatory' dictionary.

        @param policies_json: The policy JSON data (name-value pairs).
        @returns: dict of policies grouped by mode keys.

        """
        mandatory_policies = {}
        recommended_policies = {}
        collated_json = {}

        # Extract mandatory and recommended mode dicts.
        if 'mandatory' in policies_json:
            mandatory_policies = policies_json['mandatory']
            del policies_json['mandatory']
        if 'recommended' in policies_json:
            recommended_policies = policies_json['recommended']
            del policies_json['recommended']

        # Move any remaining modeless policies into mandatory dict.
        if policies_json:
            mandatory_policies.update(policies_json)

        # Collate all policies into mandatory & recommended dicts.
        if recommended_policies:
            collated_json.update({'recommended': recommended_policies})
        if mandatory_policies:
            collated_json.update({'mandatory': mandatory_policies})

        return collated_json

    def _remove_null_policies(self, policies_json):
        """Remove policy dict data that is set to None or ''.

        For the status of a policy to be shown as "Not set" on the
        chrome://policy page, the policy blob must contain no dictionary entry
        for that policy. This function removes policy NVPs from a copy of the
        |policies_json| dictionary that the test case had set to None or ''.

        @param policies_json: setup policy JSON data (name-value pairs).
        @returns: setup policy JSON data with all 'Not set' policies removed.

        """
        policies_json_copy = policies_json.copy()
        for policies in policies_json_copy.values():
            for policy_data in policies.items():
                if policy_data[1] is None or policy_data[1] == '':
                    policies.pop(policy_data[0])
        return policies_json_copy

    def _get_policy_value_shown(self, policy_tab, policy_name):
        """Get the value shown for the named policy on the Policies page.

        Takes |policy_name| as a parameter and returns the corresponding
        policy value shown on the chrome://policy page.

        @param policy_tab: Tab displaying the chrome://policy page.
        @param policy_name: The name of the policy.
        @returns: The value shown for the policy on the Policies page.

        """
        row_values = policy_tab.EvaluateJavaScript('''
            var section = document.getElementsByClassName("policy-table-section")[0];
            var table = section.getElementsByTagName('table')[0];
            rowValues = '';
            for (var i = 1, row; row = table.rows[i]; i++) {
               if (row.className !== 'expanded-value-container') {
                  var name_div = row.getElementsByClassName('name elide')[0];
                  var name = name_div.textContent;
                  if (name === '%s') {
                     var value_span = row.getElementsByClassName('value')[0];
                     var value = value_span.textContent;
                     var status_div = row.getElementsByClassName('status elide')[0];
                     var status = status_div.textContent;
                     rowValues = [name, value, status];
                     break;
                  }
               }
            }
            rowValues;
        ''' % policy_name)

        value_shown = row_values[1].encode('ascii', 'ignore')
        status_shown = row_values[2].encode('ascii', 'ignore')
        if status_shown == 'Not set.':
            return None
        return value_shown

    def get_elements_from_page(self, tab, cmd):
        """Get collection of page elements that match the |cmd| filter.

        @param tab: tab containing the page to be scraped.
        @param cmd: JavaScript command to evaluate on the page.
        @returns object containing elements on page that match the cmd.
        @raises: TestFail if matching elements are not found on the page.

        """
        try:
            elements = tab.EvaluateJavaScript(cmd)
        except Exception as err:
            raise error.TestFail('Unable to find matching elements on '
                                 'the test page: %s\n %r' %(tab.url, err))
        return elements

    def json_string(self, policy_value):
         """Convert policy value to a JSON formatted string.

         @param policy_value: object containing a policy value.
         @returns: string in JSON format.
         """
         return json.dumps(policy_value)

    def _validate_and_run_test_case(self, test_case, run_test):
        """Validate test case and call the test runner in the test class.

        @param test_case: name of the test case to run.
        @param run_test: method in test class that runs a test case.

        """
        if test_case not in self.TEST_CASES:
            raise error.TestError('Test case is not valid: %s' % test_case)
        logging.info('Running test case: %s', test_case)
        run_test(test_case)

    def run_once_impl(self, run_test):
        """Dispatch the common run modes for all child test classes.

        @param run_test: method in test class that runs a test case.

        """
        if self.mode == 'all':
            for test_case in sorted(self.TEST_CASES):
                self._validate_and_run_test_case(test_case, run_test)
        elif self.mode == 'single':
            self._validate_and_run_test_case(self.case, run_test)
        elif self.mode == 'list':
            logging.info('List Test Cases:')
            for test_case, value in sorted(self.TEST_CASES.items()):
                logging.info('  case=%s, value="%s"', test_case, value)
        else:
            raise error.TestError('Run mode is not valid: %s' % self.mode)

    def run_once(self):
        # The run_once() method is core to all autotest tests. We define it
        # herein to support tests that do not define their own override.
        self.run_once_impl(self.run_test_case)
