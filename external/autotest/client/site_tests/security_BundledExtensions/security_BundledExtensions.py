# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

class security_BundledExtensions(test.test):
    """Verify security properties of bundled (on-disk) extensions."""
    version = 1

    def load_baseline(self):
        """
        Loads the set of expected permissions.

        @return Dictionary of expected permissions.
        """
        bfile = open(os.path.join(self.bindir, 'baseline'))
        with open(os.path.join(self.bindir, 'baseline')) as bfile:
            baseline = []
            for line in bfile:
                if not line.startswith('#'):
                    baseline.append(line)
            baseline = json.loads(''.join(baseline))
        self._ignored_extension_ids = baseline['ignored_extension_ids']
        self._bundled_crx_baseline = baseline['bundled_crx_baseline']
        self._component_extension_baseline = baseline[
            'component_extension_baseline']
        self._official_components = baseline['official_components']
        self._extensions_info = None


    def _get_stable_extensions_info(self, ext):
        """
        Poll condition that verifies that we're getting a stable list of
        extensions from chrome.autotestPrivate.getExtensionInfo.

        @return list of dicts, each representing an extension.
        """
        logging.info("Poll")
        prev = self._extensions_info
        ext.ExecuteJavaScript('''
            window.__extensions_info = null;
            chrome.autotestPrivate.getExtensionsInfo(function(s) {
                window.__extensions_info = s.extensions;
            });
        ''')
        self._extensions_info = utils.poll_for_condition(
                lambda: ext.EvaluateJavaScript('window.__extensions_info'))
        if not prev:
            return False
        return len(prev) == len(self._extensions_info)

    def _get_extensions_info(self):
        """
        Calls _get_stable_extensions_info to get a stable list of extensions.
        Filters out extensions that are on the to-be-ignored list.

        @return list of dicts, each representing an extension.
        """
        with chrome.Chrome(logged_in=True, autotest_ext=True) as cr:
            ext = cr.autotest_ext
            if not ext:
                return None

            utils.poll_for_condition(
                    lambda: self._get_stable_extensions_info(ext),
                    sleep_interval=0.5, timeout=30)
            logging.debug("getExtensionsInfo:\n%s", self._extensions_info)
            filtered_info = []
            self._ignored_extension_ids.append(ext.extension_id)
            for rec in self._extensions_info:
                if not rec['id'] in self._ignored_extension_ids:
                    filtered_info.append(rec)
            self._extensions_info = filtered_info
            return filtered_info


    def compare_extensions(self):
        """Compare installed extensions to the expected set.

        Find the set of expected IDs.
        Find the set of observed IDs.
        Do set comparison to find the unexpected, and the expected/missing.

        """
        test_fail = False
        combined_baseline = (self._bundled_crx_baseline +
                             self._component_extension_baseline)
        # Filter out any baseline entries that don't apply to this board.
        # If there is no 'boards' limiter on a given record, the record applies.
        # If there IS a 'boards' limiter, check that it applies.
        board = utils.get_current_board()
        combined_baseline = [x for x in combined_baseline
                             if ((not 'boards' in x) or
                                 ('boards' in x and board in x['boards']))]

        observed_extensions = self._get_extensions_info()
        observed_ids = set([x['id'] for x in observed_extensions])
        expected_ids = set([x['id'] for x in combined_baseline])

        missing_ids = expected_ids - observed_ids
        missing_names = ['%s (%s)' % (x['name'], x['id'])
                         for x in combined_baseline if x['id'] in missing_ids]

        unexpected_ids = observed_ids - expected_ids
        unexpected_names = ['%s (%s)' % (x['name'], x['id'])
                            for x in observed_extensions if
                            x['id'] in unexpected_ids]

        good_ids = expected_ids.intersection(observed_ids)

        if missing_names:
            logging.error('Missing: %s', '; '.join(missing_names))
            test_fail = True
        if unexpected_names:
            logging.error('Unexpected: %s', '; '.join(unexpected_names))
            test_fail = True

        # For those IDs in both the expected-and-observed, ie, "good":
        #   Compare sets of expected-vs-actual API permissions, report diffs.
        #   Do same for host permissions.
        for good_id in good_ids:
            baseline = [x for x in combined_baseline if x['id'] == good_id][0]
            actual = [x for x in observed_extensions if x['id'] == good_id][0]
            # Check the API permissions.
            baseline_apis = set(baseline['apiPermissions'])
            actual_apis = set(actual['apiPermissions'])
            missing_apis = baseline_apis - actual_apis
            unexpected_apis = actual_apis - baseline_apis
            if missing_apis or unexpected_apis:
                test_fail = True
                self._report_attribute_diffs(missing_apis, unexpected_apis,
                                             actual)
            # Check the host permissions.
            baseline_hosts = set(baseline['effectiveHostPermissions'])
            actual_hosts = set(actual['effectiveHostPermissions'])
            missing_hosts = baseline_hosts - actual_hosts
            unexpected_hosts = actual_hosts - baseline_hosts
            if missing_hosts or unexpected_hosts:
                test_fail = True
                self._report_attribute_diffs(missing_hosts, unexpected_hosts,
                                             actual)
        if test_fail:
            # TODO(jorgelo): make this fail again, see crbug.com/343271.
            raise error.TestWarn('Baseline mismatch, see error log.')


    def _report_attribute_diffs(self, missing, unexpected, rec):
        logging.error('Problem with %s (%s):', rec['name'], rec['id'])
        if missing:
            logging.error('It no longer uses: %s', '; '.join(missing))
        if unexpected:
            logging.error('It unexpectedly uses: %s', '; '.join(unexpected))


    def run_once(self, mode=None):
        self.load_baseline()
        self.compare_extensions()
