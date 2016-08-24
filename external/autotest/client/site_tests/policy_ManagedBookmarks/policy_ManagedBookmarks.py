# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base


class policy_ManagedBookmarks(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of ManagedBookmarks policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for a range of valid values
    of the ManagedBookmarks user policy, as defined by three test cases:
    NotSet, SingleBookmark_Shown, and MultiBookmark_Shown.

    When NotSet, the policy value is undefined. This induces the default
    behavior of not showing the managed bookmarks folder, which is equivalent
    to what is seen by an un-managed user.

    When one or more bookmarks are specified by the policy, then the Managed
    Bookmarks folder is shown, and the specified bookmarks within it.

    """
    version = 1

    POLICY_NAME = 'ManagedBookmarks'
    SINGLE_BOOKMARK = '''
    {
      "name": "Google",
      "url": "https://www.google.com/"
    }
    '''
    MULTI_BOOKMARK = '''
    {
      "name": "Google",
      "url": "https://www.google.com/"
    }
    ,{
      "name": "CNN",
      "url": "http://www.cnn.com/"
    }
    ,{
      "name": "IRS",
      "url": "http://www.irs.gov/"
    }
    '''
    SUPPORTING_POLICIES = {
        'BookmarkBarEnabled': True
    }

    # Dictionary of named test cases and policy values.
    TEST_CASES = {
        'NotSet_NotShown': None,
        'SingleBookmark_Shown': SINGLE_BOOKMARK,
        'MultiBookmark_Shown': MULTI_BOOKMARK
    }

    def _test_managed_bookmarks(self, policy_value, policies_json):
        """Verify CrOS enforces ManagedBookmarks policy.

        When ManagedBookmarks is not set, the UI shall not show the managed
        bookmarks folder nor its contents. When set to one or more bookmarks
        the UI shows the folder and its contents.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.

        """
        self.setup_case(self.POLICY_NAME, policy_value, policies_json)
        logging.info('Running _test_managed_bookmarks(policy_value=%s, '
                     'policies_json=%s)', policy_value, policies_json)
        if policy_value is None:
            if self._managed_bookmarks_are_shown(policy_value):
                raise error.TestFail('Managed Bookmarks should be hidden.')
        else:
            if not self._managed_bookmarks_are_shown(policy_value):
                raise error.TestFail('Managed Bookmarks should be shown.')

    def _managed_bookmarks_are_shown(self, policy_bookmarks):
        """Check whether managed bookmarks are shown in the UI.

        @returns: True if the managed bookmarks are shown.

        """
        # Extract dictionary of folders shown in bookmark tree.
        tab = self._open_boomark_manager_to_folder(0)
        cmd = 'document.getElementsByClassName("tree-item");'
        tree_items = self.get_elements_from_page(tab, cmd)

        # Scan bookmark tree for a folder with the domain-name in title.
        domain_name = self.USERNAME.split('@')[1]
        folder_title = domain_name + ' bookmarks'
        for bookmark_element in tree_items.itervalues():
            bookmark_node = bookmark_element['bookmarkNode']
            bookmark_title = bookmark_node['title']
            if bookmark_title == folder_title:
                folder_id = bookmark_node['id'].encode('ascii', 'ignore')
                break
        else:
            tab.Close()
            return False
        tab.Close()

        # Extract list of bookmarks shown in bookmark list-pane.
        tab = self._open_boomark_manager_to_folder(folder_id)
        cmd = '''
            var bookmarks = [];
            var listPane = document.getElementById("list-pane");
            var labels = listPane.getElementsByClassName("label");
            for (var i = 0; i < labels.length; i++) {
               bookmarks.push(labels[i].textContent);
            }
            bookmarks;
        '''
        bookmark_items = self.get_elements_from_page(tab, cmd)
        tab.Close()

        # Get list of expected bookmarks as set by policy.
        json_bookmarks = json.loads('[%s]' % policy_bookmarks)
        bookmarks_expected = [bmk['name'] for bmk in json_bookmarks]

        # Compare bookmarks shown vs expected.
        if bookmark_items != bookmarks_expected:
            raise error.TestFail('Bookmarks shown are not correct: %s '
                                 '(expected: %s)' %
                                 (bookmark_items, bookmarks_expected))
        return True

    def _open_boomark_manager_to_folder(self, folder_number):
        """Open bookmark manager page and select specified folder.

        @param folder_number: folder to select when opening page.
        @returns: tab loaded with bookmark manager page.

        """
        # Open Bookmark Manager with specified folder selected.
        bmp_url = ('chrome://bookmarks/#%s' % folder_number)
        tab = self.navigate_to_url(bmp_url)

        # Wait until list.reload() is defined on page.
        tab.WaitForJavaScriptExpression(
            "typeof bmm.list.reload == 'function'", 60)
        time.sleep(1)  # Allow JS to run after function is defined.
        return tab

    def _run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| string and |policies_json| data based
        on the test |case|. If the user specified an expected |value| in the
        command line args, then use it to set the |policy_value| and blank out
        the |policies_json|.

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
            policies_json = self.SUPPORTING_POLICIES.copy()
            if self.TEST_CASES[case] is None:
                policy_value = None
                policy_json = {self.POLICY_NAME: None}
            else:
                policy_value = self.TEST_CASES[case]
                policy_json = {self.POLICY_NAME: ('[%s]' % policy_value)}
            policies_json.update(policy_json)

        # Run test using values configured for the test case.
        self._test_managed_bookmarks(policy_value, policies_json)

    def run_once(self):
        self.run_once_impl(self._run_test_case)
