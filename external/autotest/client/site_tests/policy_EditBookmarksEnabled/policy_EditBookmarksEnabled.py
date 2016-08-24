# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base


class policy_EditBookmarksEnabled(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of EditBookmarksEnabled policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for all valid values of the
    EditBookmarksEnabled user policy: True, False, and not set. 'Not set'
    means that the policy value is undefined. This should induce the default
    behavior, equivalent to what is seen by an un-managed user.

    When set True or not set, bookmarks can be added, removed, or modified.
    When set False, bookmarks cannot be added, removed, or modified, though
    existing bookmarks (if any) are still available.

    """
    version = 1

    POLICY_NAME = 'EditBookmarksEnabled'
    BOOKMARKS = '''
    [
        {
          "name": "Google",
          "url": "https://www.google.com/"
        },
        {
          "name": "CNN",
          "url": "http://www.cnn.com/"
        },
        {
          "name": "IRS",
          "url": "http://www.irs.gov/"
        }
    ]
    '''
    SUPPORTING_POLICIES = {
        'BookmarkBarEnabled': True,
        'ManagedBookmarks': BOOKMARKS
    }

    # Dictionary of named test cases and policy data.
    TEST_CASES = {
        'True_Enabled': True,
        'False_Disabled': False,
        'NotSet_Enabled': None
    }

    def _test_edit_bookmarks_enabled(self, policy_value, policies_json):
        """Verify CrOS enforces EditBookmarksEnabled policy.

        When EditBookmarksEnabled is true or not set, the UI allows the user
        to add bookmarks. When false, the UI does not allow the user to add
        bookmarks.

        Warning: When the 'Bookmark Editing' setting on the CPanel User
        Settings page is set to 'Enable bookmark editing', then the
        EditBookmarksEnabled policy on the client will be not set. Thus, to
        verify the 'Enable bookmark editing' choice from a production or
        staging DMS, use case=NotSet_Enabled.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.

        """
        logging.info('Running _test_edit_bookmarks_enabled(%s, %s)',
                     policy_value, policies_json)
        self.setup_case(self.POLICY_NAME, policy_value, policies_json)
        add_bookmark_is_disabled = self._is_add_bookmark_disabled()
        if policy_value == 'true' or policy_value == 'null':
            if add_bookmark_is_disabled:
                raise error.TestFail('Add Bookmark should be enabled.')
        else:
            if not add_bookmark_is_disabled:
                raise error.TestFail('Add Bookmark should be disabled.')

    def _is_add_bookmark_disabled(self):
        """Check whether add-new-bookmark-command menu item is disabled.

        @returns: True if add-new-bookmarks-command is disabled.

        """
        tab = self.cr.browser.tabs.New()
        tab.Navigate('chrome://bookmarks/#1')
        tab.WaitForDocumentReadyStateToBeComplete()

        # Wait until list.reload() is defined on bmm page.
        tab.WaitForJavaScriptExpression(
            "typeof bmm.list.reload == 'function'", 60)
        time.sleep(1)  # Allow JS to run after function is defined.

        # Check if add-new-bookmark menu command has disabled property.
        is_disabled = tab.EvaluateJavaScript(
            '$("add-new-bookmark-command").disabled;')
        logging.info('add-new-bookmark-command is disabled: %s', is_disabled)
        tab.Close()
        return is_disabled

    def run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| and |policies_json| data based on the
        test |case|. If the user specified an expected |value|, then use it to
        set the |policy_value| and blank out |policies_json|.

        @param case: Name of the test case to run.

        """
        if self.is_value_given:
            # If |value| was given by user, then set expected |policy_value|
            # to the given value, and setup |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the defaults required by the test |case|.
            policy_value = self.json_string(self.TEST_CASES[case])
            policy_json = {self.POLICY_NAME: self.TEST_CASES[case]}
            policies_json = self.SUPPORTING_POLICIES.copy()
            policies_json.update(policy_json)

        # Run test using values configured for the test case.
        self._test_edit_bookmarks_enabled(policy_value, policies_json)
