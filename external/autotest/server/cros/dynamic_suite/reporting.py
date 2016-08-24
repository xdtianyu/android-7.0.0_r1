#pylint: disable-msg=W0611
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cgi
import collections
import HTMLParser
import logging
import os
import re

from xml.parsers import expat

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils  import gmail_lib

# Try importing the essential bug reporting libraries.
try:
    from autotest_lib.site_utils import phapi_lib
except ImportError, e:
    fundamental_libs = False
    logging.debug('Bug filing disabled. %s', e)
else:
    fundamental_libs = True

EMAIL_COUNT_KEY = 'emails.test_failure.%s'
BUG_CONFIG_SECTION = 'BUG_REPORTING'

CHROMIUM_EMAIL_ADDRESS = global_config.global_config.get_config_value(
        BUG_CONFIG_SECTION, 'chromium_email_address', default='')
EMAIL_CREDS_FILE = global_config.global_config.get_config_value(
        'NOTIFICATIONS', 'gmail_api_credentials_test_failure', default=None)


class Bug(object):
    """Holds the minimum information needed to make a dedupable bug report."""

    def __init__(self, title, summary, search_marker=None, labels=None,
                 owner='', cc=None):
        """
        Initializes Bug object.

        @param title: The title of the bug.
        @param summary: The summary of the bug.
        @param search_marker: The string used to determine if a bug is a
                              duplicate report or not. All Bugs with the same
                              search_marker are considered to be for the same
                              bug. Make this None if you do not want to dedupe.
        @param labels: The labels that the filed bug will have.
        @param owner: The owner/asignee of this bug. Typically left blank.
        @param cc: Who to cc'd for this bug.
        """
        self._title = title
        self._summary = summary
        self._search_marker = search_marker
        self.owner = owner

        self.labels = labels if labels is not None else []
        self.cc = cc if cc is not None else []


    def title(self):
        """Combines information about this bug into a title string."""
        return self._title


    def summary(self):
        """Combines information about this bug into a summary string."""
        return self._summary


    def search_marker(self):
        """Return an Anchor that we can use to dedupe this exact bug."""
        return self._search_marker


class TestBug(Bug):
    """
    Wrap up all information needed to make an intelligent report about an
    issue. Each TestBug has a search marker associated with it that can be
    used to find similar reports.
    """

    def __init__(self, build, chrome_version, suite, result):
        """
        @param build: The build type, of the form <board>/<milestone>-<release>.
                      eg: x86-mario-release/R25-4321.0.0
        @param chrome_version: The chrome version associated with the build.
                               eg: 28.0.1498.1
        @param suite: The name of the suite that this test run is a part of.
        @param result: The status of the job associated with this issue.
                       This contains the status, job id, test name, hostname
                       and reason for issue.
        """
        self.build = build
        self.chrome_version = chrome_version
        self.suite = suite
        self.name = tools.get_test_name(build, suite, result.test_name)
        self.reason = result.reason
        # The result_owner is used to find results and logs.
        self.result_owner = result.owner
        self.hostname = result.hostname
        self.job_id = result.id

        # Aborts, server/client job failures or a test failure without a
        # reason field need lab attention. Lab bugs for the aborted case
        # are disabled till crbug.com/188217 is resolved.
        self.lab_error = job_status.is_for_infrastructure_fail(result)

        # The owner is who the bug is assigned to.
        self.owner = ''
        self.cc = []

        if result.is_warn():
            self.labels = ['Test-Warning']
            self.status = 'Warning'
        else:
            self.labels = []
            self.status = 'Failure'


    def title(self):
        """Combines information about this bug into a title string."""
        return '[%s] %s %s on %s' % (self.suite, self.name,
                                     self.status, self.build)


    def summary(self):
        """Combines information about this bug into a summary string."""

        links = self._get_links_for_failure()
        template = ('This report is automatically generated to track the '
                    'following %(status)s:\n'
                    'Test: %(test)s.\n'
                    'Suite: %(suite)s.\n'
                    'Chrome Version: %(chrome_version)s.\n'
                    'Build: %(build)s.\n\nReason:\n%(reason)s.\n'
                    'build artifacts: %(build_artifacts)s.\n'
                    'results log: %(results_log)s.\n'
                    'status log: %(status_log)s.\n'
                    'buildbot stages: %(buildbot_stages)s.\n'
                    'job link: %(job)s.\n\n'
                    'You may want to check the test retry dashboard in case '
                    'this is a flakey test: %(retry_url)s\n')

        specifics = {
            'status': self.status,
            'test': self.name,
            'suite': self.suite,
            'build': self.build,
            'chrome_version': self.chrome_version,
            'reason': self.reason,
            'build_artifacts': links.artifacts,
            'results_log': links.results,
            'status_log': links.status_log,
            'buildbot_stages': links.buildbot,
            'job': links.job,
            'retry_url': links.retry_url,
        }

        return template % specifics


    def search_marker(self):
        """Return an Anchor that we can use to dedupe this exact bug."""
        board = ''
        try:
            board = site_utils.ParseBuildName(self.build)[0]
        except site_utils.ParseBuildNameException as e:
            logging.error(str(e))

        # Substitute the board name for a placeholder. We try both build and
        # release board name variants.
        reason = self.reason
        if board:
            for b in (board, board.replace('_', '-')):
                reason = reason.replace(b, 'BOARD_PLACEHOLDER')

        return "%s(%s,%s,%s)" % ('Test%s' % self.status, self.suite,
                                 self.name, reason)


    def _get_links_for_failure(self):
        """Returns a named tuple of links related to this failure."""
        links = collections.namedtuple('links', ('results,'
                                                 'status_log,'
                                                 'artifacts,'
                                                 'buildbot,'
                                                 'job,'
                                                 'retry_url'))
        return links(reporting_utils.link_result_logs(
                         self.job_id, self.result_owner, self.hostname),
                     reporting_utils.link_status_log(
                         self.job_id, self.result_owner, self.hostname),
                     reporting_utils.link_build_artifacts(self.build),
                     reporting_utils.link_buildbot_stages(self.build),
                     reporting_utils.link_job(self.job_id),
                     reporting_utils.link_retry_url(self.name))


class MachineKillerBug(Bug):
    """Wrap up information needed to report a test killing a machine."""

    # Label used by the bug-filer to categorize machine killers
    _MACHINE_KILLER_LABEL = 'machine-killer'
    # Address to which this bug will be cc'd
    _CC_ADDRESS = global_config.global_config.get_config_value(
                            'SCHEDULER', 'notify_email_errors', default='')


    def __init__(self, job_id, job_name, machine):
        """Initialize MachineKillerBug.

        @param job_id: The id of the job, this should be an afe job id.
        @param job_name: the name of the job
        @param machine: The hostname of a machine that has been put
                        in Repair Failed by the job.

        """
        # Name of test job may contain information like build and suite.
        # e.g. lumpy-release/R31-1234.0.0/bvt/dummy_Pass_SERVER_JOB
        # Try to split job_name with '/' and use the last part
        # as test name. Note this assumes test name must not contains '/'.
        self._test_name = job_name.rsplit('/', 1)[-1]
        self._job_id = job_id
        self._machine = machine
        self.owner=''
        self.cc=[self._CC_ADDRESS]
        self.labels=[self._MACHINE_KILLER_LABEL]


    def title(self):
        return ('%s suspected of putting machines in Repair Failed state.'
                 % self._test_name)

    def summary(self):
        """Combines information about this bug into a summary string."""

        template = ('This bug has been automatically filed to track the '
                    'following issue:\n\n'
                    'Test: %(test)s.\n'
                    'Machine: %(machine)s.\n'
                    'Issue: It is suspected that the test has put the '
                    'machine in the Repair Failed State.\n'
                    'Suggested Actions: Investigate to determine if this '
                    'test is at fault and then either fix or disable the '
                    'test if appropriate.\n'
                    'Job link: %(job)s.\n')
        disclaimer = ('\n\nNote that the autofiled count on this bug indicates '
                      'the number of times we have attempted to repair the '
                      'machine, not the number of times it has gone into '
                      'the repair failed state.\n')
        specifics = {
            'test': self._test_name,
            'machine': self._machine,
            'job': reporting_utils.link_job(self._job_id),
        }
        return template % specifics + disclaimer


    def search_marker(self):
        """Returns an Anchor that we can use to dedupe this bug."""
        return 'MachineKiller(%s)' % self._test_name


class PoolHealthBug(Bug):
    """Report information about a critical pool of DUTs in the lab."""

    _POOL_HEALTH_LABELS = ['recoverduts', 'Build-HardwareLab', 'Pri-1']
    _CC_ADDRESS = global_config.global_config.get_config_value(
                            'BUG_REPORTING', 'pool_health_cc',
                            type=list, default=[])


    def __init__(self, pool, board, dead_hostnames):
        """Initialize a PoolHealthBug.

        @param pool: The name of the pool in critical condition.
        @param board: The board in critical condition.
        @param dead_hostnames: A list of unusable machines with the given
            board, in the given pool.
        """
        self._pool = pool
        self._board = board
        self._dead_hostnames = dead_hostnames
        self.owner = ''
        self.cc = self._CC_ADDRESS
        self.labels = self._POOL_HEALTH_LABELS


    def title(self):
        return ('pool: %s, board: %s in a critical state.' %
                (self._pool, self._board))


    def summary(self):
        """Combines information about this bug into a summary string."""

        template = ('This bug has been automatically filed to track the '
                    'following issue:\n'
                    'Pool: %(pool)s.\n'
                    'Board: %(board)s.\n'
                    'Dead hosts: %(dead_hosts)s.\n'
                    'Issue: The pool is in a critical condition and cannot '
                    'complete build verification tests in a timely manner.\n'
                    'Suggested Actions: Recover the devices ASAP.')
        specifics = {
            'pool': self._pool,
            'board': self._board,
            'dead_hosts': ','.join(self._dead_hostnames),
        }
        return template % specifics


    def search_marker(self):
        """Returns an Anchor that we can use to dedupe this bug."""
        return 'PoolHealthBug(%s, %s)' % (self._pool, self._board)

class SuiteSchedulerBug(Bug):
    """Bug filed for suite scheduler."""

    _SUITE_SCHEDULER_LABELS = ['Build-HardwareLab', 'Pri-1', 'suite_scheduler']

    def __init__(self, suite, build, board, control_file_exception):
        self._suite = suite
        self._build = build
        self._board = board
        self._exception = control_file_exception
        # TODO(fdeng): fix get_sheriffs crbug.com/483254
        lab_deputies = site_utils.get_sheriffs(lab_only=True)
        self.owner = lab_deputies[0] if lab_deputies else ''
        self.labels = self._SUITE_SCHEDULER_LABELS
        self.cc = lab_deputies[1:] if lab_deputies else []


    def title(self):
        """Return Title of the bug"""
        if isinstance(self._exception, error.ControlFileNotFound):
            t = 'Missing control file'
        else:
            t = 'Problem with getting control file'
        return '[suite scheduler] %s for suite: "%s", build: %s' % (
                t, self._suite, self._build)


    def summary(self):
        """Combines information about this bug into a summary string."""
        template = ('Suite scheduler could not schedule suite due to '
                    'a control file problem:\n\n'
                    'Suite:\t%(suite)s\n'
                    'Build:\t%(build)s\n'
                    'Board:\t%(board)s (The problem may happen for other '
                    'boards as well, only the first board is reported.)\n'
                    'Diagnose:\n%(diagnose)s\n')

        if isinstance(self._exception, error.ControlFileNotFound):
            diagnose = (
                    '\tThe suite\'s control file does not exist in the build.\n'
                    '\tDo you expect the suite to run for the said build?\n'
                    '\t- If yes, please add/backport the control file to '
                    'the build,\n'
                    '\t- If not, please fix the entry for this suite in '
                    'suite_scheduler.ini so that it specifies the '
                    'right builds to run;\n'
                    '\t  and request a push to prod.')
        else:
            diagnose = ('\tNo suggestion. Please ask infra deputy '
                        'to triage.\n%s\n') % str(self._exception)
        specifics = {'suite': self._suite,
                     'build': self._build,
                     'board': self._board,
                     'error': type(self._exception),
                     'diagnose': diagnose,}
        return template % specifics


    def search_marker(self):
        """Returns an Anchor that we can use to dedupe this bug."""
        # TODO(fdeng): flaky deduping behavior, see crbug.com/486895
        return 'SuiteSchedulerBug(%s, %s)' % (
                self._suite, type(self._exception))


class Reporter(object):
    """
    Files external reports about bugs that happened inside autotest.
    """
    # Credentials for access to the project hosting api
    _project_name = global_config.global_config.get_config_value(
        BUG_CONFIG_SECTION, 'project_name', default='')
    _oauth_credentials = global_config.global_config.get_config_value(
        BUG_CONFIG_SECTION, 'credentials', default='')

    # AUTOFILED_COUNT is a label prefix used to indicate how
    # many times we think we've updated an issue automatically.
    AUTOFILED_COUNT = 'autofiled-count-'
    _PREDEFINED_LABELS = ['autofiled', '%s%d' % (AUTOFILED_COUNT, 1),
                          'OS-Chrome', 'Type-Bug',
                          'Restrict-View-Google']

    _SEARCH_MARKER = 'ANCHOR  '


    @classmethod
    def get_creds_abspath(cls):
        """Returns the abspath of the bug filer credentials file.

        @return: A path to the oauth2 credentials file.
        """
        return site_utils.get_creds_abspath(cls._oauth_credentials)


    def __init__(self):
        if not fundamental_libs:
            logging.warning("Bug filing disabled due to missing imports.")
            return
        try:
            self._phapi_client = phapi_lib.ProjectHostingApiClient(
                    self.get_creds_abspath(), self._project_name)
        except phapi_lib.ProjectHostingApiException as e:
            logging.error('Unable to create project hosting api client: %s', e)
            self._phapi_client = None


    def _check_tracker(self):
        """Returns True if we have a tracker object to use for filing bugs."""
        return fundamental_libs and self._phapi_client


    def get_bug_tracker_client(self):
        """Returns the client used to communicate with the project hosting api.

        @return: The instance of the ProjectHostingApiClient associated with
            this reporter.
        """
        if self._check_tracker():
            return self._phapi_client
        raise phapi_lib.ProjectHostingApiException('Project hosting client not '
                'initialized for project:%s, using auth file: %s' %
                (self._project_name, self.get_creds_abspath()))


    def _get_lab_error_template(self):
        """Return the lab error template.

        @return: A dictionary representing the bug options for an issue that
                 requires investigation from the lab team.
        """
        lab_sheriff = site_utils.get_sheriffs(lab_only=True)
        return {'labels': ['Build-HardwareLab'],
                'owner': lab_sheriff[0] if lab_sheriff else '',}


    def _format_issue_options(self, override, **kwargs):
        """
        Override the default issue configuration with a suite specific
        configuration when one is specified in the suite's bug_template.
        The bug_template is specified in the suite control file. After
        overriding the correct options, format them in a way that's understood
        by the project hosting api.

        @param override: Suite specific dictionary with issue config operations.
        @param kwargs: Keyword args containing the default issue config options.
        @return: A dictionary which contains the suite specific options, and the
                 default option when a suite specific option isn't specified.
        """
        if override:
            kwargs.update((k,v) for k,v in override.iteritems() if v)

        kwargs['labels'] = list(set(kwargs['labels'] + self._PREDEFINED_LABELS))
        kwargs['cc'] = list(map(lambda cc: {'name': cc},
                                set(kwargs['cc'] + kwargs['sheriffs'])))

        # The existence of an owner key will cause the api to try and match
        # the value under the key to a member of the project, resulting in a
        # 404 or 500 Http response when the owner is invalid.
        if (CHROMIUM_EMAIL_ADDRESS not in kwargs['owner']):
            del(kwargs['owner'])
        else:
            kwargs['owner'] = {'name': kwargs['owner']}
        return kwargs


    def _anchor_summary(self, bug):
        """
        Creates the summary that can be used for bug deduplication.

        Only attaches the anchor if the search_marker on the bug is not None.

        @param: The bug to create the anchored summary for.

        @return the summary with the anchor appened if the search marker is not
                None, otherwise return the summary.
        """
        if bug.search_marker() is None:
            return bug.summary()
        else:
            return '%s\n\n%s%s\n' % (bug.summary(), self._SEARCH_MARKER,
                                     bug.search_marker())


    def _create_bug_report(self, bug, bug_template={}, sheriffs=[]):
        """
        Creates a new bug report.

        @param bug: The Bug instance to create the report for.
        @param bug_template: A template of options to use for filing bugs.
        @param sheriffs: A list of chromium email addresses (of sheriffs)
                         to cc on this bug. Since the list of sheriffs is
                         dynamic it needs to be determined at runtime, as
                         opposed to the normal cc list which is available
                         through the bug template.
        @return: id of the created issue, or None if an issue wasn't created.
                 Note that if either the description or title fields are missing
                 we won't be able to create a bug.
        """
        anchored_summary = self._anchor_summary(bug)

        issue = self._format_issue_options(bug_template, title=bug.title(),
            description=anchored_summary, labels=bug.labels,
            status='Untriaged', owner=bug.owner, cc=bug.cc,
            sheriffs=sheriffs)

        try:
            filed_bug = self._phapi_client.create_issue(issue)
        except phapi_lib.ProjectHostingApiException as e:
            logging.error('Unable to create a bug for issue with title: %s and '
                          'description %s and owner: %s. To file a new bug you '
                          'need both a description and a title, and to assign '
                          'it to an owner, that person must be known to the '
                          'bug tracker', bug.title(), anchored_summary,
                          issue.get('owner'))
        else:
            logging.info('Filing new bug %s, with description %s',
                         filed_bug.get('id'), anchored_summary)
            return filed_bug.get('id')


    def modify_bug_report(self, issue_id, comment, label_update, status=''):
        """Modifies an existing bug report with a new comment.

        Adds the given comment and applies the given list of label
        updates.

        @param issue_id     Id of the issue to update with.
        @param comment      Comment to update the issue with.
        @param label_update List with label updates.
        @param status       New status of the issue.
        """
        updates = {
            'content': comment,
            'updates': { 'labels': label_update, 'status': status }
        }
        try:
            self._phapi_client.update_issue(issue_id, updates)
        except phapi_lib.ProjectHostingApiException as e:
            logging.warning('Unable to update issue %s, comment %s, '
                            'labels %r, status %s: %s', issue_id, comment,
                            label_update, status, e)
        else:
            logging.info('Updated issue %s, comment %s, labels %r, status %s.',
                         issue_id, comment, label_update, status)


    def find_issue_by_marker(self, marker):
        """
        Queries the tracker to find if there is a bug filed for this issue.

        1. 'Escape' the string: cgi.escape is the easiest way to achieve this,
           though it doesn't handle all html escape characters.
           eg: replace '"<' with '&quot;&lt;'
        2. Perform an exact search for the escaped string, if this returns an
           empty issue list perform a more relaxed query and finally fall back
           to a query devoid of the reason field. Between these 3 queries we
           should retrieve the super set of all issues that this marker can be
           in. In most cases the first search should return a result, examples
           where this might not be the case are when the reason field contains
           information that varies between test runs. Since the second search
           has raw escape characters it will match comments too, and the last
           should match all similar issues regardless.
        3. Look through the issues for an exact match between clean versions
           of the marker and summary; for now 'clean' means bereft of numbers.
        4. If no match is found look through a list of comments for each issue.

        @param marker The marker string to search for to find a duplicate of
                     this issue.
        @return A phapi_lib.Issue instance of the issue that was found, or
                None if no issue was found. Also returns None if the marker
                is None.
        """

        if marker is None:
            logging.info('No search marker specified, will create new issue.')
            return None

        # Note that this method cannot handle markers which have already been
        # html escaped, as it will try and unescape them by converting the &
        # to &amp again, thereby failing deduplication.
        marker = HTMLParser.HTMLParser().unescape(marker)
        html_escaped_marker = cgi.escape(marker, quote=True)

        # The tracker frontend stores summaries and comments as html elements,
        # specifically, a summary turns into a span and a comment into
        # preformatted text. Eg:
        # 1. A summary of >& would become <span>&gt;&amp;</span>
        # 2. A comment of >& would become <pre>&gt;&amp;</pre>
        # When searching for exact matches in text, the gdata api gets this
        # feed and parses all <pre> tags unescaping html, then matching your
        # exact string to that. However it does not unescape all <span> tags,
        # presumably for reasons of performance. Therefore a search for the
        # exact string ">&" would match issue 2, but not issue 1, and a search
        # for "&gt;&amp;" would match issue 1 but not issue 2. This problem is
        # further exacerbated when we have quotes within our search string,
        # which is common when the reason field contains a python dictionary.
        #
        # Our searching strategy prioritizes exact matches in the summary, since
        # the first bug thats filed will have a summary with the anchor. If we
        # do not find an exact match in any summary we search through all
        # related issues of the same bug/suite in the hope of finding an exact
        # match in the comments. Note that the comments are returned as
        # unescaped text.
        #
        # TODO(beeps): when we start merging issues this could return bloated
        # results, but for now we have to include duplicate issues so that
        # we can find the original one with the hook.
        markers = ['"' + self._SEARCH_MARKER + html_escaped_marker + '"',
                   self._SEARCH_MARKER + marker,
                   self._SEARCH_MARKER + ','.join(marker.split(',')[:2])]
        for decorated_marker in markers:
            issues = self._phapi_client.get_tracker_issues_by_text(
                decorated_marker, include_dupes=True)
            if issues:
                break

        if not issues:
            return

        # Breadth first, since open issues/bugs probably < comments/issue.
        # If we find more than one issue matching a particular anchor assign
        # a mystery bug with all relevent information on the owner and return
        # the first matching issue.
        clean_marker = re.sub('[0-9]+', '', html_escaped_marker)
        all_issues = [issue for issue in issues
                      if clean_marker in re.sub('[0-9]+', '', issue.summary)]

        if len(all_issues) > 1:
            issue_ids = [issue.id for issue in all_issues]
            logging.warning('Multiple results for a specific query. Query: %s, '
                            'results: %s', marker, issue_ids)

        if all_issues:
            return all_issues[0]

        unescaped_clean_marker = re.sub('[0-9]+', '', marker)
        for issue in issues:
            if any(unescaped_clean_marker in re.sub('[0-9]+', '', comment)
                   for comment in issue.comments):
                return issue


    def _dedupe_issue(self, marker):
        """Finds an issue, then checks if it has a parent that's still open.

        @param marker: The marker string to search for to find a duplicate of
                       a issue.
        @return An Issue instance, representing an open issue that is a
                duplicate of the one being searched for.
        """
        issue = self.find_issue_by_marker(marker)
        if not issue or issue.state == constants.ISSUE_OPEN:
            return issue

        # Iterativly look through the chain of parents, until we find one whose
        # state is 'open' or reach the end of the chain.
        # It is possible that the chain forms a circle. Record the visited
        # issues to prevent loop on a circle.
        visited_issues = set([issue.id])
        while issue.merged_into is not None:
            issue = self._phapi_client.get_tracker_issue_by_id(
                issue.merged_into)
            if not issue or issue.id in visited_issues:
                break
            elif issue.state == constants.ISSUE_OPEN:
                logging.debug('Return the active issue %d that duplicated '
                              'issue(s) have been merged into.', issue.id)
                return issue
            else:
                visited_issues.add(issue.id)
        logging.debug('All merged issues %s have been closed, marked '
                      'invalid etc, will create a new issue instead.',
                      list(visited_issues))
        return None


    def _get_count_labels_and_max(self, issue):
        """Read the current autofiled count labels and count.

         Automatically filed issues have a label of the form
        `autofiled-count-<number>` that indicates about how many
        times the autofiling code has updated the issue.  This
        routine goes through the labels for the given issue to find
        the existing count label(s).

        Old bugs may not have a count; this routine implicitly
        assigns those bugs an initial count of one.

        Usually, only one count label should exist. But
        this method is written to take care of the case
        where multiple count labels exist. In such case,
        All the labels and the max count is returned.

        @param issue: Issue whose 'autofiled-count' is to be read.

        @returns: 2-tuple with a list of labels and
                  the max count.
        """
        count_labels = []
        count_max = 1
        is_count_label = lambda l: l.startswith(self.AUTOFILED_COUNT)
        for label in filter(is_count_label, issue.labels):
            try:
                count = int(label[len(self.AUTOFILED_COUNT):])
            except ValueError:
                continue
            count_max = max(count, count_max)
            count_labels.append(label)
        return count_labels, count_max


    def _create_autofiled_count_update(self, issue):
        """Calculate an 'autofiled-count' label update.

        Remove all the existing autofiled count labels
        and calculate a new count label.

        Updates to issues aren't guaranteed to be atomic, so in
        some cases count labels may (in theory at least) be dropped
        or duplicated.

        The return values are a list of label updates and the
        count value of the new count label.  For the label updates,
        all existing count labels will be prefixed with '-' to
        remove them, and a new label with a new count will be added
        to the set.  Labels not related to the count aren't updated.

        @param issue Issue whose 'autofiled-count' is to be updated.
        @return      2-tuple with a list of label updates and the
                     new count value.
        """
        count_labels, count_max = self._get_count_labels_and_max(issue)
        label_updates = []
        for label in count_labels:
            label_updates.append('-%s' % label)
        new_count = count_max + 1
        label_updates.append('%s%d' % (self.AUTOFILED_COUNT, new_count))
        return label_updates, new_count


    @classmethod
    def _get_project_label_from_title(cls, title):
        """Extract a project label for the device being tested from
        provided bug title. If no project is found, return empty string.

        E.g. For the following bug title:

          [stress] platform_BootDevice Failure on rikku-release/R44-7075.0.0

        we extract 'rikku' and return a string 'Proj-rikku'.

        Note1: For certain boards, they contain the reference name as well:

          veyron_minnie-release/R44-7075.0.0

        in these cases, we only extract and use the subboard (minnie) and not
        the whole string (veyron_minnie).

        Note2: some builds have different names like tot-release,
        freon-build, etc. This function needs to handle these cases as well.

        @param title: A string of the bug title, from which to extract
                      the project label for the device being tested.
        @return       '' if no valid label is found, or a label of the
                      form 'proj-samus' if found.
        """
        m = re.search('.* on (?:.*_)?(?P<proj>[^-]*)-[\S]+/.*', title)
        if m and m.group('proj'):
            return 'Proj-%s' % m.group('proj')
        else:
            return ''


    def report(self, bug, bug_template={}, ignore_duplicate=False):
        """Report an issue to the bug tracker.

        If this issue has happened before, post a comment on the
        existing bug about it occurring again, and update the
        'autofiled-count' label.  If this is a new issue, create a
        new bug for it.

        @param bug          A Bug instance about the issue.
        @param bug_template A template dictionary specifying the
                            default bug filing options for an issue
                            with this suite.
        @param ignore_duplicate: If True, when a duplicate is found,
                            simply ignore the new one rather than
                            posting an update.
        @return             A 2-tuple of the issue id of the issue
                            that was either created or modified, and
                            a count of the number of times the bug
                            has been updated.  For a new bug, the
                            count is 1. If we could not file a bug
                            for some reason, the count is 0.
        """
        if not self._check_tracker():
            logging.error("Can't file %s", bug.title())
            return None, 0

        project_label = self._get_project_label_from_title(bug.title())

        issue = None
        try:
            issue = self._dedupe_issue(bug.search_marker())
        except expat.ExpatError as e:
            # If our search string sends python's xml module into a
            # state which it believes will lead to an xml syntax
            # error, it will give up and throw an exception. This
            # might happen with aborted jobs that contain weird
            # escape characters in their reason fields. We'd rather
            # create a new issue than fail in deduplicating such cases.
            logging.warning('Unable to deduplicate, creating new issue: %s',
                            str(e))

        if issue and ignore_duplicate:
            logging.debug('Duplicate found for %s, not filing as requested.',
                          bug.search_marker())
            _, bug_count = self._get_count_labels_and_max(issue)
            return issue.id, bug_count

        if issue:
            comment = '%s\n\n%s' % (bug.title(), self._anchor_summary(bug))
            label_update, bug_count = (
                    self._create_autofiled_count_update(issue))
            if project_label:
                label_update.append(project_label)
            self.modify_bug_report(issue.id, comment, label_update)
            return issue.id, bug_count

        sheriffs = []

        # TODO(beeps): crbug.com/254256
        try:
            if bug.lab_error and bug.suite == 'bvt':
                lab_error_template = self._get_lab_error_template()
                if bug_template.get('labels'):
                    lab_error_template['labels'] += bug_template.get('labels')
                bug_template = lab_error_template
            elif bug.suite == 'bvt':
                sheriffs = site_utils.get_sheriffs()
        except AttributeError:
            pass

        if project_label:
            bug_template.get('labels', []).append(project_label)
        bug_id = self._create_bug_report(bug, bug_template, sheriffs)
        bug_count = 1 if bug_id else 0
        return bug_id, bug_count


# TODO(beeps): Move this to server/site_utils after crbug.com/281906 is fixed.
def submit_generic_bug_report(*args, **kwargs):
    """
    Submit a generic bug report.

    See server.cros.dynamic_suite.reporting.Bug for valid arguments.

    @params args: List of arguments to pass to the Bug creation.
    @params kwargs: Keyword arguments to pass to Bug creation.

    @returns the filed bug's id.
    """
    bug = Bug(*args, **kwargs)
    reporter = Reporter()
    return reporter.report(bug)[0]


def send_email(bug, bug_template):
    """Send email to the owner and cc's to notify the TestBug.

    @param bug: TestBug instance.
    @param bug_template: A template dictionary specifying the default bug
                         filing options for failures in this suite.
    """
    autotest_stats.Counter(EMAIL_COUNT_KEY % 'total').increment()
    to_set = set(bug.cc) if bug.cc else set()
    if bug.owner:
        to_set.add(bug.owner)
    if bug_template.get('cc'):
        to_set = to_set.union(bug_template.get('cc'))
    if bug_template.get('owner'):
        to_set.add(bug_template.get('owner'))
    recipients = ', '.join(to_set)
    try:
        gmail_lib.send_email(
            recipients, bug.title(), bug.summary(), retry=False,
            creds_path=site_utils.get_creds_abspath(EMAIL_CREDS_FILE))
    except Exception:
        autotest_stats.Counter(EMAIL_COUNT_KEY % 'fail').increment()
        raise
