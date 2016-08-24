#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Tool for running suites of tests and waiting for completion.

The desired test suite will be scheduled with autotest. By default,
this tool will block until the job is complete, printing a summary
at the end.  Error conditions result in exceptions.

This is intended for use only with Chrome OS test suits that leverage the
dynamic suite infrastructure in server/cros/dynamic_suite.py.

This script exits with one of the following codes:
0 - OK: Suite finished successfully
1 - ERROR: Test(s) failed, or hits its own timeout
2 - WARNING: Test(s) raised a warning or passed on retry, none failed/timed out.
3 - INFRA_FAILURE: Infrastructure related issues, e.g.
    * Lab is down
    * Too many duts (defined as a constant) in repair failed status
    * Suite job issues, like bug in dynamic suite,
      user aborted the suite, lose a drone/all devservers/rpc server,
      0 tests ran, etc.
    * provision failed
      TODO(fdeng): crbug.com/413918, reexamine treating all provision
                   failures as INFRA failures.
4 - SUITE_TIMEOUT: Suite timed out, some tests ran,
    none failed by the time the suite job was aborted. This will cover,
    but not limited to, the following cases:
    * A devserver failure that manifests as a timeout
    * No DUTs available midway through a suite
    * Provision/Reset/Cleanup took longer time than expected for new image
    * A regression in scheduler tick time.
5- BOARD_NOT_AVAILABLE: If there is no host for the requested board/pool.
6- INVALID_OPTIONS: If options are not valid.
"""


import datetime as datetime_base
import ast, getpass, json, logging, optparse, os, re, sys, time
from datetime import datetime

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config, enum
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils import diagnosis_utils
from autotest_lib.site_utils import job_overhead


CONFIG = global_config.global_config

WMATRIX_RETRY_URL = CONFIG.get_config_value('BUG_REPORTING',
                                            'wmatrix_retry_url')

# Return code that will be sent back to autotest_rpc_server.py
RETURN_CODES = enum.Enum(
        'OK', 'ERROR', 'WARNING', 'INFRA_FAILURE', 'SUITE_TIMEOUT',
        'BOARD_NOT_AVAILABLE', 'INVALID_OPTIONS')
# The severity of return code. If multiple codes
# apply, the script should always return the severest one.
# E.g. if we have a test failure and the suite also timed out,
# we should return 'ERROR'.
SEVERITY = {RETURN_CODES.OK: 0,
            RETURN_CODES.WARNING: 1,
            RETURN_CODES.SUITE_TIMEOUT: 2,
            RETURN_CODES.INFRA_FAILURE: 3,
            RETURN_CODES.ERROR: 4}
ANDROID_BUILD_REGEX = r'.+/.+/[0-9]+'


def get_worse_code(code1, code2):
    """Compare the severity of two codes and return the worse code.

    @param code1: An enum value of RETURN_CODES
    @param code2: An enum value of RETURN_CODES

    @returns: the more severe one between code1 and code2.

    """
    return code1 if SEVERITY[code1] >= SEVERITY[code2] else code2


def parse_options():
    #pylint: disable-msg=C0111
    usage = "usage: %prog [options]"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("-b", "--board", dest="board")
    parser.add_option("-i", "--build", dest="build")
    parser.add_option("-w", "--web", dest="web", default=None,
                      help="Address of a webserver to receive suite requests.")
    parser.add_option('--firmware_rw_build', dest='firmware_rw_build',
                      default=None,
                      help='Firmware build to be installed in dut RW firmware.')
    parser.add_option('--firmware_ro_build', dest='firmware_ro_build',
                      default=None,
                      help='Firmware build to be installed in dut RO firmware.')
    parser.add_option('--test_source_build', dest='test_source_build',
                      default=None,
                      help=('Build that contains the test code, '
                            'e.g., it can be the value of `--build`, '
                            '`--firmware_rw_build` or `--firmware_ro_build` '
                            'arguments. Default is None, that is, use the test '
                            'code from `--build` (CrOS image)'))
    #  This should just be a boolean flag, but the autotest "proxy" code
    #  can't handle flags that don't take arguments.
    parser.add_option("-n", "--no_wait", dest="no_wait", default="False",
                      help='Must pass "True" or "False" if used.')
    # If you really want no pool, --pool="" will do it. USE WITH CARE.
    parser.add_option("-p", "--pool", dest="pool", default="suites")
    parser.add_option("-s", "--suite_name", dest="name")
    parser.add_option("-a", "--afe_timeout_mins", type="int",
                      dest="afe_timeout_mins", default=30)
    parser.add_option("-t", "--timeout_mins", type="int",
                      dest="timeout_mins", default=1440)
    parser.add_option("-x", "--max_runtime_mins", type="int",
                      dest="max_runtime_mins", default=1440)
    parser.add_option("-d", "--delay_sec", type="int",
                      dest="delay_sec", default=10)
    parser.add_option("-m", "--mock_job_id", dest="mock_job_id",
                      help="Attach to existing job id for already running "
                           "suite, and creates report.")
    # NOTE(akeshet): This looks similar to --no_wait, but behaves differently.
    # --no_wait is passed in to the suite rpc itself and affects the suite,
    # while this does not.
    parser.add_option("-c", "--create_and_return", dest="create_and_return",
                      action="store_true",
                      help="Create the suite and print the job id, then "
                           "finish immediately.")
    parser.add_option("-u", "--num", dest="num", type="int", default=None,
                      help="Run on at most NUM machines.")
    #  Same boolean flag issue applies here.
    parser.add_option("-f", "--file_bugs", dest="file_bugs", default='False',
                      help='File bugs on test failures. Must pass "True" or '
                           '"False" if used.')
    parser.add_option("-l", "--bypass_labstatus", dest="bypass_labstatus",
                      action="store_true", help='Bypass lab status check.')
    # We allow either a number or a string for the priority.  This way, if you
    # know what you're doing, one can specify a custom priority level between
    # other levels.
    parser.add_option("-r", "--priority", dest="priority",
                      default=priorities.Priority.DEFAULT,
                      action="store", help="Priority of suite")
    parser.add_option('--retry', dest='retry', default='False',
                      action='store', help='Enable test retry. '
                      'Must pass "True" or "False" if used.')
    parser.add_option('--max_retries', dest='max_retries', default=None,
                      type='int', action='store', help='Maximum retries'
                      'allowed at suite level. No limit if not specified.')
    parser.add_option('--minimum_duts', dest='minimum_duts', type=int,
                      default=0, action='store',
                      help='Check that the pool has at least such many '
                           'healthy machines, otherwise suite will not run. '
                           'Default to 0.')
    parser.add_option('--suite_min_duts', dest='suite_min_duts', type=int,
                      default=0, action='store',
                      help='Preferred minimum number of machines. Scheduler '
                           'will prioritize on getting such many machines for '
                           'the suite when it is competing with another suite '
                           'that has a higher priority but already got minimum '
                           'machines it needs. Default to 0.')
    parser.add_option("--suite_args", dest="suite_args",
                      default=None, action="store",
                      help="Argument string for suite control file.")
    parser.add_option('--offload_failures_only', dest='offload_failures_only',
                      action='store', default='False',
                      help='Only enable gs_offloading for failed tests. '
                           'Successful tests will be deleted. Must pass "True"'
                           ' or "False" if used.')
    parser.add_option('--use_suite_attr', dest='use_suite_attr',
                      action='store_true', default=False,
                      help='Advanced. Run the suite based on ATTRIBUTES of '
                      'control files, rather than SUITE.')
    parser.add_option('--json_dump', dest='json_dump', action='store_true',
                      default=False,
                      help='Dump the output of run_suite to stdout.')
    parser.add_option('--run_prod_code', dest='run_prod_code',
                      action='store_true', default=False,
                      help='Run the test code that lives in prod aka the test '
                           'code currently on the lab servers.')
    options, args = parser.parse_args()
    return parser, options, args


def verify_options_and_args(options, args):
    """Verify the validity of options and args.

    @param options: The parsed options to verify.
    @param args: The parsed args to verify.

    @returns: True if verification passes, False otherwise.

    """
    if args:
        print 'Unknown arguments: ' + str(args)
        return False

    if options.mock_job_id and (
            not options.build or not options.name or not options.board):
        print ('When using -m, need to specify build, board and suite '
               'name which you have used for creating the original job')
        return False
    else:
        if not options.build:
            print 'Need to specify which build to use'
            return False
        if not options.board:
            print 'Need to specify board'
            return False
        if not options.name:
            print 'Need to specify suite name'
            return False
    if options.num is not None and options.num < 1:
        print 'Number of machines must be more than 0, if specified.'
        return False
    if options.no_wait != 'True' and options.no_wait != 'False':
        print 'Please specify "True" or "False" for --no_wait.'
        return False
    if options.file_bugs != 'True' and options.file_bugs != 'False':
        print 'Please specify "True" or "False" for --file_bugs.'
        return False
    if options.retry != 'True' and options.retry != 'False':
        print 'Please specify "True" or "False" for --retry'
        return False
    if options.retry == 'False' and options.max_retries is not None:
        print 'max_retries can only be used with --retry=True'
        return False
    if options.use_suite_attr and options.suite_args is not None:
        print ('The new suite control file cannot parse the suite_args: %s.'
               'Please not specify any suite_args here.' % options.suite_args)
        return False
    if options.no_wait == 'True' and options.retry == 'True':
        print 'Test retry is not available when using --no_wait=True'
    # Default to use the test code in CrOS build.
    if not options.test_source_build and options.build:
        options.test_source_build = options.build
    return True


def change_options_for_suite_attr(options):
    """Change options to be prepared to run the suite_attr_wrapper.

    If specify 'use_suite_attr' from the cmd line, it indicates to run the
    new style suite control file, suite_attr_wrapper. Then, change the
    options.suite_name to 'suite_attr_wrapper', change the options.suite_args to
    include the arguments needed by suite_attr_wrapper.

    @param options: The verified options.

    @returns: The changed options.

    """
    # Convert the suite_name to attribute boolean expression.
    if type(options.name) is str:
        attr_filter_val = 'suite:%s' % options.name
    else:
        attr_filter_val = ' or '.join(['suite:%s' % x for x in options.name])

    # change the suite_args to be a dict of arguments for suite_attr_wrapper
    # if suite_args is not None, store the values in 'other_args' of the dict
    args_dict = {}
    args_dict['attr_filter'] = attr_filter_val
    options.suite_args = str(args_dict)
    options.name = 'suite_attr_wrapper'

    return options


def get_pretty_status(status):
    """
    Converts a status string into a pretty-for-printing string.

    @param status: Status to convert.

    @return: Returns pretty string.
             GOOD    -> [ PASSED ]
             TEST_NA -> [ INFO ]
             other   -> [ FAILED ]
    """
    if status == 'GOOD':
        return '[ PASSED ]'
    elif status == 'TEST_NA':
        return '[  INFO  ]'
    return '[ FAILED ]'


def get_original_suite_name(suite_name, suite_args):
    """Get the original suite name when running suite_attr_wrapper.

    @param suite_name: the name of the suite launched in afe. When it is
                       suite_attr_wrapper, the suite that actually running is
                       specified in the suite_args.
    @param suite_args: the parsed option which contains the original suite name.

    @returns: the original suite name.

    """
    if suite_name == 'suite_attr_wrapper':
        attrs = ast.literal_eval(suite_args).get('attr_filter', '')
        suite_list = ([x[6:] for x in re.split('[() ]', attrs)
                       if x and x.startswith('suite:')])
        return suite_list[0] if suite_list else suite_name
    return suite_name


def GetBuildbotStepLink(anchor_text, url):
    """Generate a buildbot formatted link.

    @param anchor_text    The link text.
    @param url            The url to link to.
    """
    return '@@@STEP_LINK@%s@%s@@@' % (anchor_text, url)


class LogLink(object):
    """Information needed to record a link in the logs.

    Depending on context and the information provided at
    construction time, the link may point to either to log files for
    a job, or to a bug filed for a failure in the job.

    @var anchor  The link text.
    @var url     The link url.
    @var bug_id  Id of a bug to link to, or None.
    """

    _BUG_URL_PREFIX = CONFIG.get_config_value('BUG_REPORTING',
                                              'tracker_url')
    _URL_PATTERN = CONFIG.get_config_value('CROS',
                                           'log_url_pattern', type=str)


    @classmethod
    def get_bug_link(cls, bug_id):
        """Generate a bug link for the given bug_id.

        @param bug_id: The id of the bug.
        @return: A link, eg: https://crbug.com/<bug_id>.
        """
        return '%s%s' % (cls._BUG_URL_PREFIX, bug_id)


    def __init__(self, anchor, server, job_string, bug_info=None, reason=None,
                 retry_count=0, testname=None):
        """Initialize the LogLink by generating the log URL.

        @param anchor      The link text.
        @param server      The hostname of the server this suite ran on.
        @param job_string  The job whose logs we'd like to link to.
        @param bug_info    Info about the bug, if one was filed.
        @param reason      A string representing the reason of failure if any.
        @param retry_count How many times the test has been retried.
        @param testname    Optional Arg that supplies the testname.
        """
        self.anchor = anchor
        self.url = self._URL_PATTERN % (server, job_string)
        self.reason = reason
        self.retry_count = retry_count
        self.testname = testname
        if bug_info:
            self.bug_id, self.bug_count = bug_info
        else:
            self.bug_id = None
            self.bug_count = None


    def GenerateBuildbotLink(self):
        """Generate a link formatted to meet buildbot expectations.

        If there is a bug associated with this link, report that;
        otherwise report a link to the job logs.

        @return A link formatted for the buildbot log annotator.
        """
        info_strings = []
        if self.retry_count > 0:
            info_strings.append('retry_count: %d' % self.retry_count)

        if self.bug_id:
            url = self.get_bug_link(self.bug_id)
            if self.bug_count is None:
                bug_info = 'unknown number of reports'
            elif self.bug_count == 1:
                bug_info = 'new report'
            else:
                bug_info = '%s reports' % self.bug_count
            info_strings.append(bug_info)
        else:
            url = self.url

        if self.reason:
            info_strings.append(self.reason.strip())

        if info_strings:
            info = ', '.join(info_strings)
            anchor_text = '%(anchor)s: %(info)s' % {
                    'anchor': self.anchor.strip(), 'info': info}
        else:
            anchor_text = self.anchor.strip()

        return GetBuildbotStepLink(anchor_text, url)


    def GenerateTextLink(self):
        """Generate a link to the job's logs, for consumption by a human.

        @return A link formatted for human readability.
        """
        return '%s%s' % (self.anchor, self.url)


    def GenerateWmatrixRetryLink(self):
        """Generate a link to the wmatrix retry dashboard.

        @return A link formatted for the buildbot log annotator.
        """
        if not self.testname:
            return None

        return GetBuildbotStepLink(
                'Flaky test dashboard view for test %s' %
                self.testname, WMATRIX_RETRY_URL % self.testname)


class Timings(object):
    """Timings for important events during a suite.

    All timestamps are datetime.datetime objects.

    @var suite_job_id: the afe job id of the suite job for which
                       we are recording the timing for.
    @var download_start_time: the time the devserver starts staging
                              the build artifacts. Recorded in create_suite_job.
    @var payload_end_time: the time when the artifacts only necessary to start
                           installsing images onto DUT's are staged.
                           Recorded in create_suite_job.
    @var artifact_end_time: the remaining artifacts are downloaded after we kick
                            off the reimaging job, at which point we record
                            artifact_end_time. Recorded in dynamic_suite.py.
    @var suite_start_time: the time the suite started.
    @var tests_start_time: the time the first test started running.
    @var tests_end_time: the time the last test finished running.
    """

    def __init__(self, suite_job_id):
        self.suite_job_id = suite_job_id
        # Timings related to staging artifacts on devserver.
        self.download_start_time = None
        self.payload_end_time = None
        self.artifact_end_time = None

        # The test_start_time, but taken off the view that corresponds to the
        # suite instead of an individual test.
        self.suite_start_time = None

        # Earliest and Latest tests in the set of TestViews passed to us.
        self.tests_start_time = None
        self.tests_end_time = None


    def RecordTiming(self, view):
        """Given a test report view, extract and record pertinent time info.

        get_detailed_test_views() returns a list of entries that provide
        info about the various parts of a suite run.  This method can take
        any one of these entries and look up timestamp info we might want
        and record it.

        If timestamps are unavailable, datetime.datetime.min/max will be used.

        @param view: A TestView object.
        """
        start_candidate = datetime.min
        end_candidate = datetime.max
        if view['test_started_time']:
            start_candidate = time_utils.time_string_to_datetime(
                    view['test_started_time'])
        if view['test_finished_time']:
            end_candidate = time_utils.time_string_to_datetime(
                    view['test_finished_time'])

        if view.get_testname() == TestView.SUITE_PREP:
            self.suite_start_time = start_candidate
        else:
            self._UpdateFirstTestStartTime(start_candidate)
            self._UpdateLastTestEndTime(end_candidate)
        if view['afe_job_id'] == self.suite_job_id and 'job_keyvals' in view:
            keyvals = view['job_keyvals']
            self.download_start_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.DOWNLOAD_STARTED_TIME),
                    handle_type_error=True)

            self.payload_end_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.PAYLOAD_FINISHED_TIME),
                    handle_type_error=True)

            self.artifact_end_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.ARTIFACT_FINISHED_TIME),
                    handle_type_error=True)


    def _UpdateFirstTestStartTime(self, candidate):
        """Update self.tests_start_time, iff candidate is an earlier time.

        @param candidate: a datetime.datetime object.
        """
        if not self.tests_start_time or candidate < self.tests_start_time:
            self.tests_start_time = candidate


    def _UpdateLastTestEndTime(self, candidate):
        """Update self.tests_end_time, iff candidate is a later time.

        @param candidate: a datetime.datetime object.
        """
        if not self.tests_end_time or candidate > self.tests_end_time:
            self.tests_end_time = candidate


    def __str__(self):
        return ('\n'
                'Suite timings:\n'
                'Downloads started at %s\n'
                'Payload downloads ended at %s\n'
                'Suite started at %s\n'
                'Artifact downloads ended (at latest) at %s\n'
                'Testing started at %s\n'
                'Testing ended at %s\n' % (self.download_start_time,
                                           self.payload_end_time,
                                           self.suite_start_time,
                                           self.artifact_end_time,
                                           self.tests_start_time,
                                           self.tests_end_time))


    def SendResultsToStatsd(self, suite, build, board):
        """
        Sends data to statsd.

        1. Makes a data_key of the form: run_suite.$board.$branch.$suite
            eg: stats/gauges/<hostname>/run_suite/<board>/<branch>/<suite>/
        2. Computes timings for several start and end event pairs.
        3. Sends all timing values to statsd.

        @param suite: scheduled suite that we want to record the results of.
        @param build: the build that this suite ran on.
                      eg: 'lumpy-release/R26-3570.0.0'
        @param board: the board that this suite ran on.
        """
        if sys.version_info < (2, 7):
            logging.error('Sending run_suite perf data to statsd requires'
                          'python 2.7 or greater.')
            return

        # Constructs the key used for logging statsd timing data.
        data_key = utils.get_data_key('run_suite', suite, build, board)

        # Since we don't want to try subtracting corrupted datetime values
        # we catch TypeErrors in time_utils.time_string_to_datetime and insert
        # None instead. This means that even if, say,
        # keyvals.get(constants.ARTIFACT_FINISHED_TIME) returns a corrupt
        # value the member artifact_end_time is set to None.
        if self.download_start_time:
            if self.payload_end_time:
                autotest_stats.Timer(data_key).send('payload_download_time',
                        (self.payload_end_time -
                         self.download_start_time).total_seconds())

            if self.artifact_end_time:
                autotest_stats.Timer(data_key).send('artifact_download_time',
                        (self.artifact_end_time -
                         self.download_start_time).total_seconds())

        if self.tests_end_time:
            if self.suite_start_time:
                autotest_stats.Timer(data_key).send('suite_run_time',
                        (self.tests_end_time -
                         self.suite_start_time).total_seconds())

            if self.tests_start_time:
                autotest_stats.Timer(data_key).send('tests_run_time',
                        (self.tests_end_time -
                         self.tests_start_time).total_seconds())


_DEFAULT_AUTOTEST_INSTANCE = CONFIG.get_config_value(
        'SERVER', 'hostname', type=str)


def instance_for_pool(pool_name):
    """
    Return the hostname of the server that should be used to service a suite
    for the specified pool.

    @param pool_name: The pool (without 'pool:' to schedule the suite against.
    @return: The correct host that should be used to service this suite run.
    """
    return CONFIG.get_config_value(
            'POOL_INSTANCE_SHARDING', pool_name,
            default=_DEFAULT_AUTOTEST_INSTANCE)


class TestView(object):
    """Represents a test view and provides a set of helper functions."""


    SUITE_PREP = 'Suite prep'
    INFRA_TESTS = ['provision']


    def __init__(self, view, afe_job, suite_name, build, user,
                 solo_test_run=False):
        """Init a TestView object representing a tko test view.

        @param view: A dictionary representing a tko test view.
        @param afe_job: An instance of frontend.afe.models.Job
                        representing the job that kicked off the test.
        @param suite_name: The name of the suite
                           that the test belongs to.
        @param build: The build for which the test is run.
        @param user: The user for which the test is run.
        @param solo_test_run: This is a solo test run not part of a suite.
        """
        self.view = view
        self.afe_job = afe_job
        self.suite_name = suite_name
        self.build = build
        self.is_suite_view = afe_job.parent_job is None and not solo_test_run
        # This is the test name that will be shown in the output.
        self.testname = None
        self.user = user

        # The case that a job was aborted before it got a chance to run
        # usually indicates suite has timed out (unless aborted by user).
        # In this case, the abort reason will be None.
        # Update the reason with proper information.
        if (self.is_relevant_suite_view() and
                not self.get_testname() == self.SUITE_PREP and
                self.view['status'] == 'ABORT' and
                not self.view['reason']):
            self.view['reason'] = 'Timed out, did not run.'


    def __getitem__(self, key):
        """Overload __getitem__ so that we can still use []

        @param key: A key of the tko test view.

        @returns: The value of an attribute in the view.

        """
        return self.view[key]


    def __iter__(self):
        """Overload __iter__ so that it supports 'in' operator."""
        return iter(self.view)


    def get_testname(self):
        """Get test name that should be shown in the output.

        Formalize the test_name we got from the test view.

        Remove 'build/suite' prefix if any. And append 'experimental' prefix
        for experimental tests if their names do not start with 'experimental'.

        If one runs a test in control file via the following code,
           job.runtest('my_Test', tag='tag')
        for most of the cases, view['test_name'] would look like 'my_Test.tag'.
        If this is the case, this method will just return the original
        test name, i.e. 'my_Test.tag'.

        There are four special cases.
        1) A test view is for the suite job's SERVER_JOB.
           In this case, this method will return 'Suite prep'.

        2) A test view is of a child job or a solo test run not part of a
           suite, and for a SERVER_JOB or CLIENT_JOB.
           In this case, we will take the job name, remove the build/suite
           prefix from the job name, and append the rest to 'SERVER_JOB'
           or 'CLIENT_JOB' as a prefix. So the names returned by this
           method will look like:
             'experimental_Telemetry Smoothness Measurement_SERVER_JOB'
             'experimental_dummy_Pass_SERVER_JOB'
             'dummy_Fail_SERVER_JOB'

        3) A test view is of a suite job and its status is ABORT.
           In this case, the view['test_name'] is the child job's name.
           If it is an experimental test, 'experimental' will be part
           of the name. For instance,
             'lumpy-release/R35-5712.0.0/perf_v2/
                   experimental_Telemetry Smoothness Measurement'
             'lumpy-release/R35-5712.0.0/dummy/experimental_dummy_Pass'
             'lumpy-release/R35-5712.0.0/dummy/dummy_Fail'
           The above names will be converted to the following:
             'experimental_Telemetry Smoothness Measurement'
             'experimental_dummy_Pass'
             'dummy_Fail'

        4) A test view's status is of a suite job and its status is TEST_NA.
           In this case, the view['test_name'] is the NAME field of the control
           file. If it is an experimental test, 'experimental' will part of
           the name. For instance,
             'experimental_Telemetry Smoothness Measurement'
             'experimental_dummy_Pass'
             'dummy_Fail'
           This method will not modify these names.

        @returns: Test name after normalization.

        """
        if self.testname is not None:
            return self.testname

        if (self.is_suite_view and
                self.view['test_name'].startswith('SERVER_JOB')):
            # Rename suite job's SERVER_JOB to 'Suite prep'.
            self.testname = self.SUITE_PREP
            return self.testname

        if (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB')):
            # Append job name as a prefix for SERVER_JOB and CLIENT_JOB
            testname= '%s_%s' % (self.view['job_name'], self.view['test_name'])
        else:
            testname = self.view['test_name']
        experimental =  self.is_experimental()
        # Remove the build and suite name from testname if any.
        testname = tools.get_test_name(
                self.build, self.suite_name, testname)
        # If an experimental test was aborted, testname
        # would include the 'experimental' prefix already.
        prefix = constants.EXPERIMENTAL_PREFIX if (
                experimental and not
                testname.startswith(constants.EXPERIMENTAL_PREFIX)) else ''
        self.testname = prefix + testname
        return self.testname


    def is_relevant_suite_view(self):
        """Checks whether this is a suite view we should care about.

        @returns: True if it is relevant. False otherwise.
        """
        return (self.get_testname() == self.SUITE_PREP or
                (self.is_suite_view and
                    not self.view['test_name'].startswith('CLIENT_JOB') and
                    not self.view['subdir']))


    def is_test(self):
        """Return whether the view is for an actual test.

        @returns True if the view is for an actual test.
                 False if the view is for SERVER_JOB or CLIENT_JOB.

        """
        return not (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB'))


    def is_retry(self):
        """Check whether the view is for a retry.

        @returns: True, if the view is for a retry; False otherwise.

        """
        return self.view['job_keyvals'].get('retry_original_job_id') is not None


    def is_experimental(self):
        """Check whether a test view is for an experimental test.

        @returns: True if it is for an experimental test, False otherwise.

        """
        return (self.view['job_keyvals'].get('experimental') == 'True' or
                tools.get_test_name(self.build, self.suite_name,
                        self.view['test_name']).startswith('experimental'))


    def hit_timeout(self):
        """Check whether the corresponding job has hit its own timeout.

        Note this method should not be called for those test views
        that belongs to a suite job and are determined as irrelevant
        by is_relevant_suite_view.  This is because they are associated
        to the suite job, whose job start/finished time make no sense
        to an irrelevant test view.

        @returns: True if the corresponding afe job has hit timeout.
                  False otherwise.
        """
        if (self.is_relevant_suite_view() and
                self.get_testname() != self.SUITE_PREP):
            # Any relevant suite test view except SUITE_PREP
            # did not hit its own timeout because it was not ever run.
            return False
        start = (datetime.strptime(
                self.view['job_started_time'], time_utils.TIME_FMT)
                if self.view['job_started_time'] else None)
        end = (datetime.strptime(
                self.view['job_finished_time'], time_utils.TIME_FMT)
                if self.view['job_finished_time'] else None)
        if not start or not end:
            return False
        else:
            return ((end - start).total_seconds()/60.0
                        > self.afe_job.max_runtime_mins)


    def is_aborted(self):
        """Check if the view was aborted.

        For suite prep and child job test views, we check job keyval
        'aborted_by' and test status.

        For relevant suite job test views, we only check test status
        because the suite job keyval won't make sense to individual
        test views.

        @returns: True if the test was as aborted, False otherwise.

        """

        if (self.is_relevant_suite_view() and
                self.get_testname() != self.SUITE_PREP):
            return self.view['status'] == 'ABORT'
        else:
            return (bool(self.view['job_keyvals'].get('aborted_by')) and
                    self.view['status'] in ['ABORT', 'RUNNING'])


    def is_in_fail_status(self):
        """Check if the given test's status corresponds to a failure.

        @returns: True if the test's status is FAIL or ERROR. False otherwise.

        """
        # All the statuses tests can have when they fail.
        return self.view['status'] in ['FAIL', 'ERROR', 'ABORT']


    def is_infra_test(self):
        """Check whether this is a test that only lab infra is concerned.

        @returns: True if only lab infra is concerned, False otherwise.

        """
        return self.get_testname() in self.INFRA_TESTS


    def get_buildbot_link_reason(self):
        """Generate the buildbot link reason for the test.

        @returns: A string representing the reason.

        """
        return ('%s: %s' % (self.view['status'], self.view['reason'])
                if self.view['reason'] else self.view['status'])


    def get_job_id_owner_str(self):
        """Generate the job_id_owner string for a test.

        @returns: A string which looks like 135036-username

        """
        return '%s-%s' % (self.view['afe_job_id'], self.user)


    def get_bug_info(self, suite_job_keyvals):
        """Get the bug info from suite_job_keyvals.

        If a bug has been filed for the test, its bug info (bug id and counts)
        will be stored in the suite job's keyvals. This method attempts to
        retrieve bug info of the test from |suite_job_keyvals|. It will return
        None if no bug info is found. No need to check bug info if the view is
        SUITE_PREP.

        @param suite_job_keyvals: The job keyval dictionary of the suite job.
                All the bug info about child jobs are stored in
                suite job's keyvals.

        @returns: None if there is no bug info, or a pair with the
                  id of the bug, and the count of the number of
                  times the bug has been seen.

        """
        if self.get_testname() == self.SUITE_PREP:
            return None
        if (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB')):
            # Append job name as a prefix for SERVER_JOB and CLIENT_JOB
            testname= '%s_%s' % (self.view['job_name'], self.view['test_name'])
        else:
            testname = self.view['test_name']

        return tools.get_test_failure_bug_info(
                suite_job_keyvals, self.view['afe_job_id'],
                testname)


    def should_display_buildbot_link(self):
        """Check whether a buildbot link should show for this view.

        For suite prep view, show buildbot link if it fails.
        For normal test view,
            show buildbot link if it is a retry
            show buildbot link if it hits its own timeout.
            show buildbot link if it fails. This doesn't
            include the case where it was aborted but has
            not hit its own timeout (most likely it was aborted because
            suite has timed out).

        @returns: True if we should show the buildbot link.
                  False otherwise.
        """
        is_bad_status = (self.view['status'] != 'GOOD' and
                         self.view['status'] != 'TEST_NA')
        if self.get_testname() == self.SUITE_PREP:
            return is_bad_status
        else:
            if self.is_retry():
                return True
            if is_bad_status:
                return not self.is_aborted() or self.hit_timeout()


    def get_control_file_attributes(self):
        """Get the attributes from the control file of the test.

        @returns: A list of test attribute or None.
        """
        control_file = self.afe_job.control_file
        attributes = None
        if control_file:
            cd = control_data.parse_control_string(control_file)
            attributes = list(cd.attributes)
        return attributes


class ResultCollector(object):
    """Collect test results of a suite or a single test run.

    Once a suite job has finished, use this class to collect test results.
    `run` is the core method that is to be called first. Then the caller
    could retrieve information like return code, return message, is_aborted,
    and timings by accessing the collector's public attributes. And output
    the test results and links by calling the 'output_*' methods.

    Here is a overview of what `run` method does.

    1) Collect the suite job's results from tko_test_view_2.
    For the suite job, we only pull test views without a 'subdir'.
    A NULL subdir indicates that the test was _not_ executed. This could be
    that no child job was scheduled for this test or the child job got
    aborted before starts running.
    (Note 'SERVER_JOB'/'CLIENT_JOB' are handled specially)

    2) Collect the child jobs' results from tko_test_view_2.
    For child jobs, we pull all the test views associated with them.
    (Note 'SERVER_JOB'/'CLIENT_JOB' are handled speically)

    3) Generate web and buildbot links.
    4) Compute timings of the suite run.
    5) Compute the return code based on test results.

    @var _instance_server: The hostname of the server that is used
                           to service the suite.
    @var _afe: The afe rpc client.
    @var _tko: The tko rpc client.
    @var _build: The build for which the suite is run,
                 e.g. 'lumpy-release/R35-5712.0.0'
    @var _board: The target board for which the suite is run,
                 e.g., 'lumpy', 'link'.
    @var _suite_name: The suite name, e.g. 'bvt', 'dummy'.
    @var _suite_job_id: The job id of the suite for which we are going to
                        collect results.
    @var _original_suite_name: The suite name we record timing would be
                               different from _suite_name when running
                               suite_attr_wrapper.
    @var _suite_views: A list of TestView objects, representing relevant
                       test views of the suite job.
    @var _child_views: A list of TestView objects, representing test views
                       of the child jobs.
    @var _test_views: A list of TestView objects, representing all test views
                      from _suite_views and _child_views.
    @var _web_links: A list of web links pointing to the results of jobs.
    @var _buildbot_links: A list of buildbot links for non-passing tests.
    @var _max_testname_width: Max width of all test names.
    @var _solo_test_run: True if this is a single test run.
    @var return_code: The exit code that should be returned by run_suite.
    @var return_message: Any message that should be displayed to explain
                         the return code.
    @var is_aborted: Whether the suite was aborted or not.
                     True, False or None (aborting status is unknown yet)
    @var timings: A Timing object that records the suite's timings.

    """


    def __init__(self, instance_server, afe, tko, build, board,
                 suite_name, suite_job_id, original_suite_name=None,
                 user=None, solo_test_run=False):
        self._instance_server = instance_server
        self._afe = afe
        self._tko = tko
        self._build = build
        self._board = board
        self._suite_name = suite_name
        self._suite_job_id = suite_job_id
        self._original_suite_name = original_suite_name or suite_name
        self._suite_views = []
        self._child_views = []
        self._test_views = []
        self._retry_counts = {}
        self._web_links = []
        self._buildbot_links = []
        self._max_testname_width = 0
        self._num_child_jobs = 0
        self.return_code = None
        self.return_message = ''
        self.is_aborted = None
        self.timings = None
        self._user = user or getpass.getuser()
        self._solo_test_run = solo_test_run


    def _fetch_relevant_test_views_of_suite(self):
        """Fetch relevant test views of the suite job.

        For the suite job, there will be a test view for SERVER_JOB, and views
        for results of its child jobs. For example, assume we've ceated
        a suite job (afe_job_id: 40) that runs dummy_Pass, dummy_Fail,
        dummy_Pass.bluetooth. Assume dummy_Pass was aborted before running while
        dummy_Path.bluetooth got TEST_NA as no duts have bluetooth.
        So the suite job's test views would look like
        _____________________________________________________________________
        test_idx| job_idx|test_name           |subdir      |afe_job_id|status
        10      | 1000   |SERVER_JOB          |----        |40        |GOOD
        11      | 1000   |dummy_Pass          |NULL        |40        |ABORT
        12      | 1000   |dummy_Fail.Fail     |41-onwer/...|40        |FAIL
        13      | 1000   |dummy_Fail.Error    |42-owner/...|40        |ERROR
        14      | 1000   |dummy_Pass.bluetooth|NULL        |40        |TEST_NA

        For a suite job, we only care about
        a) The test view for the suite job's SERVER_JOB
        b) The test views for real tests without a subdir. A NULL subdir
           indicates that a test didn't get executed.
        So, for the above example, we only keep test views whose test_idxs
        are 10, 11, 14.

        @returns: A list of TestView objects, representing relevant
                  test views of the suite job.

        """
        suite_job = self._afe.get_jobs(id=self._suite_job_id)[0]
        views = self._tko.run(call='get_detailed_test_views',
                              afe_job_id=self._suite_job_id)
        relevant_views = []
        for v in views:
            v = TestView(v, suite_job, self._suite_name, self._build, self._user,
                         solo_test_run=self._solo_test_run)
            if v.is_relevant_suite_view():
                relevant_views.append(v)
        return relevant_views


    def _compute_retry_count(self, view):
        """Return how many times the test has been retried.

        @param view: A TestView instance.
        @returns: An int value indicating the retry count.

        """
        old_job = view['job_keyvals'].get('retry_original_job_id')
        count = 0
        while old_job:
            count += 1
            views = self._tko.run(
                call='get_detailed_test_views', afe_job_id=old_job)
            old_job = (views[0]['job_keyvals'].get('retry_original_job_id')
                       if views else None)
        return count


    def _fetch_test_views_of_child_jobs(self, jobs=None):
        """Fetch test views of child jobs.

        @returns: A tuple (child_views, retry_counts)
                  child_views is list of TestView objects, representing
                  all valid views. retry_counts is a dictionary that maps
                  test_idx to retry counts. It only stores retry
                  counts that are greater than 0.

        """
        child_views = []
        retry_counts = {}
        child_jobs = jobs or self._afe.get_jobs(parent_job_id=self._suite_job_id)
        if child_jobs:
            self._num_child_jobs = len(child_jobs)
        for job in child_jobs:
            views = [TestView(v, job, self._suite_name, self._build, self._user)
                     for v in self._tko.run(
                         call='get_detailed_test_views', afe_job_id=job.id,
                         invalid=0)]
            contains_test_failure = any(
                    v.is_test() and v['status'] != 'GOOD' for v in views)
            for v in views:
                if (v.is_test() or
                        v['status'] != 'GOOD' and not contains_test_failure):
                    # For normal test view, just keep it.
                    # For SERVER_JOB or CLIENT_JOB, only keep it
                    # if it fails and no other test failure.
                    child_views.append(v)
                    retry_count = self._compute_retry_count(v)
                    if retry_count > 0:
                        retry_counts[v['test_idx']] = retry_count
        return child_views, retry_counts


    def _generate_web_and_buildbot_links(self):
        """Generate web links and buildbot links."""
        # TODO(fdeng): If a job was aborted before it reaches Running
        # state, we read the test view from the suite job
        # and thus this method generates a link pointing to the
        # suite job's page for the aborted job. Need a fix.
        self._web_links = []
        self._buildbot_links = []
        # Bug info are stored in the suite job's keyvals.
        if self._solo_test_run:
            suite_job_keyvals = {}
        else:
            suite_job_keyvals = self._suite_views[0]['job_keyvals']
        for v in self._test_views:
            retry_count = self._retry_counts.get(v['test_idx'], 0)
            bug_info = v.get_bug_info(suite_job_keyvals)
            job_id_owner = v.get_job_id_owner_str()
            link = LogLink(
                    anchor=v.get_testname().ljust(
                            self._max_testname_width),
                    server=self._instance_server,
                    job_string=job_id_owner,
                    bug_info=bug_info, retry_count=retry_count,
                    testname=v.get_testname())
            self._web_links.append(link)

            if v.should_display_buildbot_link():
                link.reason = v.get_buildbot_link_reason()
                self._buildbot_links.append(link)


    def _record_timings(self):
        """Record suite timings."""
        self.timings = Timings(self._suite_job_id)
        for v in self._test_views:
            self.timings.RecordTiming(v)


    def _get_return_msg(self, code, tests_passed_after_retry):
        """Return the proper message for a given return code.

        @param code: An enum value of RETURN_CODES
        @param test_passed_after_retry: True/False, indicating
            whether there are test(s) that have passed after retry.

        @returns: A string, representing the message.

        """
        if code == RETURN_CODES.INFRA_FAILURE:
            return 'Suite job failed or provisioning failed.'
        elif code == RETURN_CODES.SUITE_TIMEOUT:
            return ('Some test(s) was aborted before running,'
                    ' suite must have timed out.')
        elif code == RETURN_CODES.WARNING:
            if tests_passed_after_retry:
                return 'Some test(s) passed after retry.'
            else:
                return 'Some test(s) raised a warning.'
        elif code == RETURN_CODES.ERROR:
            return 'Some test(s) failed.'
        else:
            return ''


    def _compute_return_code(self):
        """Compute the exit code based on test results."""
        code = RETURN_CODES.OK
        tests_passed_after_retry = False

        for v in self._test_views:
            # The order of checking each case is important.
            if v.is_experimental():
                continue
            if v.get_testname() == TestView.SUITE_PREP:
                if v.is_aborted() and v.hit_timeout():
                    current_code = RETURN_CODES.SUITE_TIMEOUT
                elif v.is_in_fail_status():
                    current_code = RETURN_CODES.INFRA_FAILURE
                elif v['status'] == 'WARN':
                    current_code = RETURN_CODES.WARNING
                else:
                    current_code = RETURN_CODES.OK
            else:
                if v.is_aborted() and v.is_relevant_suite_view():
                    # The test was aborted before started
                    # This gurantees that the suite has timed out.
                    current_code = RETURN_CODES.SUITE_TIMEOUT
                elif v.is_aborted() and not v.hit_timeout():
                    # The test was aborted, but
                    # not due to a timeout. This is most likely
                    # because the suite has timed out, but may
                    # also because it was aborted by the user.
                    # Since suite timing out is determined by checking
                    # the suite prep view, we simply ignore this view here.
                    current_code = RETURN_CODES.OK
                elif v.is_in_fail_status():
                    # The test job failed.
                    if v.is_infra_test():
                        current_code = RETURN_CODES.INFRA_FAILURE
                    else:
                        current_code = RETURN_CODES.ERROR
                elif v['status'] == 'WARN':
                    # The test/suite job raised a wanrning.
                    current_code = RETURN_CODES.WARNING
                elif v.is_retry():
                    # The test is a passing retry.
                    current_code = RETURN_CODES.WARNING
                    tests_passed_after_retry = True
                else:
                    current_code = RETURN_CODES.OK
            code = get_worse_code(code, current_code)

        self.return_code = code
        self.return_message = self._get_return_msg(
                code, tests_passed_after_retry)


    def output_results(self):
        """Output test results, timings and web links."""
        # Output test results
        for v in self._test_views:
            display_name = v.get_testname().ljust(self._max_testname_width)
            logging.info('%s%s', display_name,
                         get_pretty_status(v['status']))
            if v['status'] != 'GOOD':
                logging.info('%s  %s: %s', display_name, v['status'],
                             v['reason'])
            if v.is_retry():
                retry_count = self._retry_counts.get(v['test_idx'], 0)
                logging.info('%s  retry_count: %s',
                             display_name, retry_count)
        # Output suite timings
        logging.info(self.timings)
        # Output links to test logs
        logging.info('\nLinks to test logs:')
        for link in self._web_links:
            logging.info(link.GenerateTextLink())
        logging.info('\n')


    def get_results_dict(self):
        """Write test results, timings and web links into a dict.

        @returns: A dict of results in the format like:
                  {
                  'tests': {
                        'test_1': {'status': 'PASSED', 'attributes': [1,2], ...}
                        'test_2': {'status': 'FAILED', 'attributes': [1],...}
                  }
                  'suite_timings': {
                        'download_start': '1998-07-17 00:00:00',
                        'payload_download_end': '1998-07-17 00:00:05',
                        ...
                  }
                  }
        """
        output_dict = {}
        tests_dict = output_dict.setdefault('tests', {})
        for v in self._test_views:
          test_name = v.get_testname()
          test_info = tests_dict.setdefault(test_name, {})
          test_info.update({
              'status': v['status'],
              'attributes': v.get_control_file_attributes() or list(),
              'reason': v['reason'],
              'retry_count': self._retry_counts.get(v['test_idx'], 0),
              })

        # Write the links to test logs into the |tests_dict| of |output_dict|.
        # For test whose status is not 'GOOD', the link is also buildbot_link.
        for link in self._web_links:
          test_name = link.anchor.strip()
          test_info = tests_dict.get(test_name)
          if test_info:
            test_info['link_to_logs'] = link.url
            # Write the wmatrix link into the dict.
            if link in self._buildbot_links and link.testname:
              test_info['wmatrix_link'] = WMATRIX_RETRY_URL % link.testname
            # Write the bug url into the dict.
            if link.bug_id:
              test_info['bug_url'] = link.get_bug_link(link.bug_id)

        # Write the suite timings into |output_dict|
        time_dict = output_dict.setdefault('suite_timings', {})
        time_dict.update({
            'download_start' : str(self.timings.download_start_time),
            'payload_download_end' : str(self.timings.payload_end_time),
            'suite_start' : str(self.timings.suite_start_time),
            'artifact_download_end' : str(self.timings.artifact_end_time),
            'tests_start' : str(self.timings.tests_start_time),
            'tests_end' : str(self.timings.tests_end_time),
            })

        output_dict['suite_job_id'] = self._suite_job_id

        return output_dict


    def output_buildbot_links(self):
        """Output buildbot links."""
        for link in self._buildbot_links:
            logging.info(link.GenerateBuildbotLink())
            wmatrix_link = link.GenerateWmatrixRetryLink()
            if wmatrix_link:
                logging.info(wmatrix_link)


    def run(self):
        """Collect test results.

        This method goes through the following steps:
            Fetch relevent test views of the suite job.
            Fetch test views of child jobs
            Check whether the suite was aborted.
            Generate links.
            Calculate suite timings.
            Compute return code based on the test result.

        """
        if self._solo_test_run:
            self._test_views, self.retry_count = (
                  self._fetch_test_views_of_child_jobs(
                          jobs=self._afe.get_jobs(id=self._suite_job_id)))
        else:
            self._suite_views = self._fetch_relevant_test_views_of_suite()
            self._child_views, self._retry_counts = (
                    self._fetch_test_views_of_child_jobs())
            self._test_views = self._suite_views + self._child_views
        # For hostless job in Starting status, there is no test view associated.
        # This can happen when a suite job in Starting status is aborted. When
        # the scheduler hits some limit, e.g., max_hostless_jobs_per_drone,
        # max_jobs_started_per_cycle, a suite job can stays in Starting status.
        if not self._test_views:
            self.return_code = RETURN_CODES.INFRA_FAILURE
            self.return_message = 'No test view was found.'
            return
        self.is_aborted = any([view['job_keyvals'].get('aborted_by')
                               for view in self._suite_views])
        self._max_testname_width = max(
                [len(v.get_testname()) for v in self._test_views]) + 3
        self._generate_web_and_buildbot_links()
        self._record_timings()
        self._compute_return_code()


    def gather_timing_stats(self):
        """Collect timing related statistics."""
        # Send timings to statsd.
        self.timings.SendResultsToStatsd(
                self._original_suite_name, self._build, self._board)

        # Record suite runtime in metadata db.
        # Some failure modes can leave times unassigned, report sentinel value
        # in that case.
        runtime_in_secs = -1
        if (self.timings.tests_end_time is not None and
            self.timings.suite_start_time is not None):
            runtime_in_secs = (self.timings.tests_end_time -
                    self.timings.suite_start_time).total_seconds()

        job_overhead.record_suite_runtime(self._suite_job_id, self._suite_name,
                self._board, self._build, self._num_child_jobs, runtime_in_secs)


@retry.retry(error.StageControlFileFailure, timeout_min=10)
def create_suite(afe, options):
    """Create a suite with retries.

    @param afe: The afe object to insert the new suite job into.
    @param options: The options to use in creating the suite.

    @return: The afe_job_id of the new suite job.
    """
    builds = {}
    if options.build:
        if re.match(ANDROID_BUILD_REGEX, options.build):
            builds[provision.ANDROID_BUILD_VERSION_PREFIX] = options.build
        else:
            builds[provision.CROS_VERSION_PREFIX] = options.build
    if options.firmware_rw_build:
        builds[provision.FW_RW_VERSION_PREFIX] = options.firmware_rw_build
    if options.firmware_ro_build:
        builds[provision.FW_RO_VERSION_PREFIX] = options.firmware_ro_build
    wait = options.no_wait == 'False'
    file_bugs = options.file_bugs == 'True'
    retry = options.retry == 'True'
    offload_failures_only = options.offload_failures_only == 'True'
    try:
        priority = int(options.priority)
    except ValueError:
        try:
            priority = priorities.Priority.get_value(options.priority)
        except AttributeError:
            print 'Unknown priority level %s.  Try one of %s.' % (
                  options.priority, ', '.join(priorities.Priority.names))
            raise
    logging.info('%s Submitted create_suite_job rpc',
                 diagnosis_utils.JobTimer.format_time(datetime.now()))
    return afe.run('create_suite_job', name=options.name,
                   board=options.board, build=options.build,
                   builds=builds, test_source_build=options.test_source_build,
                   check_hosts=wait, pool=options.pool,
                   num=options.num,
                   file_bugs=file_bugs, priority=priority,
                   suite_args=options.suite_args,
                   wait_for_results=wait,
                   timeout_mins=options.timeout_mins,
                   max_runtime_mins=options.max_runtime_mins,
                   job_retry=retry, max_retries=options.max_retries,
                   suite_min_duts=options.suite_min_duts,
                   offload_failures_only=offload_failures_only,
                   run_prod_code=options.run_prod_code)


def main_without_exception_handling(options):
    """
    run_suite script without exception handling.

    @param options: The parsed options.

    @returns: A tuple contains the return_code of run_suite and the dictionary
              of the output.

    """
    # If indicate to use the new style suite control file, convert the args
    if options.use_suite_attr:
        options = change_options_for_suite_attr(options)

    log_name = 'run_suite-default.log'
    if options.build:
        # convert build name from containing / to containing only _
        log_name = 'run_suite-%s.log' % options.build.replace('/', '_')
        log_dir = os.path.join(common.autotest_dir, 'logs')
        if os.path.exists(log_dir):
            log_name = os.path.join(log_dir, log_name)

    utils.setup_logging(logfile=log_name)

    if not options.bypass_labstatus:
        utils.check_lab_status(options.build)
    instance_server = (options.web if options.web else
                       instance_for_pool(options.pool))
    afe = frontend_wrappers.RetryingAFE(server=instance_server,
                                        timeout_min=options.afe_timeout_mins,
                                        delay_sec=options.delay_sec)
    logging.info('Autotest instance: %s', instance_server)

    rpc_helper = diagnosis_utils.RPCHelper(afe)
    is_real_time = True
    if options.mock_job_id:
        job_id = int(options.mock_job_id)
        existing_job = afe.get_jobs(id=job_id, finished=True)
        if existing_job:
            is_real_time = False
        else:
            existing_job = afe.get_jobs(id=job_id)
        if existing_job:
            job_created_on = time_utils.date_string_to_epoch_time(
                    existing_job[0].created_on)
        else:
            raise utils.TestLabException('Failed to retrieve job: %d' % job_id)
    else:
        try:
            rpc_helper.check_dut_availability(options.board, options.pool,
                                              options.minimum_duts)
            job_id = create_suite(afe, options)
            job_created_on = time.time()
        except diagnosis_utils.NotEnoughDutsError:
            logging.info(GetBuildbotStepLink(
                    'Pool Health Bug', LogLink.get_bug_link(rpc_helper.bug)))
            raise
        except (error.CrosDynamicSuiteException,
                error.RPCException, proxy.JSONRPCException) as e:
            logging.warning('Error Message: %s', e)
            return (RETURN_CODES.INFRA_FAILURE, {'return_message': e})
        except AttributeError:
            return (RETURN_CODES.INVALID_OPTIONS, {})

    job_timer = diagnosis_utils.JobTimer(
            job_created_on, float(options.timeout_mins))
    job_url = reporting_utils.link_job(job_id,
                                       instance_server=instance_server)
    logging.info('%s Created suite job: %s',
                 job_timer.format_time(job_timer.job_created_time),
                 job_url)
    # TODO(akeshet): Move this link-printing to chromite.
    logging.info(GetBuildbotStepLink('Suite created', job_url))

    if options.create_and_return:
        msg = '--create_and_return was specified, terminating now.'
        logging.info(msg)
        return (RETURN_CODES.OK, {'return_message':msg})

    TKO = frontend_wrappers.RetryingTKO(server=instance_server,
                                        timeout_min=options.afe_timeout_mins,
                                        delay_sec=options.delay_sec)
    code = RETURN_CODES.OK
    wait = options.no_wait == 'False'
    output_dict = {}
    if wait:
        while not afe.get_jobs(id=job_id, finished=True):
            # Note that this call logs output, preventing buildbot's
            # 9000 second silent timeout from kicking in. Let there be no
            # doubt, this is a hack. The timeout is from upstream buildbot and
            # this is the easiest work around.
            if job_timer.first_past_halftime():
                rpc_helper.diagnose_job(job_id, instance_server)
            if job_timer.debug_output_timer.poll():
                logging.info('The suite job has another %s till timeout.',
                             job_timer.timeout_hours - job_timer.elapsed_time())
            time.sleep(10)
        # For most cases, ResultCollector should be able to determine whether
        # a suite has timed out by checking information in the test view.
        # However, occationally tko parser may fail on parsing the
        # job_finished time from the job's keyval file. So we add another
        # layer of timeout check in run_suite. We do the check right after
        # the suite finishes to make it as accurate as possible.
        # There is a minor race condition here where we might have aborted
        # for some reason other than a timeout, and the job_timer thinks
        # it's a timeout because of the jitter in waiting for results.
        # The consequence would be that run_suite exits with code
        # SUITE_TIMEOUT while it should  have returned INFRA_FAILURE
        # instead, which should happen very rarely.
        # Note the timeout will have no sense when using -m option.
        is_suite_timeout = job_timer.is_suite_timeout()

        # Extract the original suite name to record timing.
        original_suite_name = get_original_suite_name(options.name,
                                                      options.suite_args)
        # Start collecting test results.
        collector = ResultCollector(instance_server=instance_server,
                                    afe=afe, tko=TKO, build=options.build,
                                    board=options.board,
                                    suite_name=options.name,
                                    suite_job_id=job_id,
                                    original_suite_name=original_suite_name)
        collector.run()
        # Dump test outputs into json.
        output_dict = collector.get_results_dict()
        output_dict['autotest_instance'] = instance_server
        if not options.json_dump:
          collector.output_results()
        code = collector.return_code
        return_message = collector.return_message
        if is_real_time:
            # Do not record stats if the suite was aborted (either by a user
            # or through the golo rpc).
            # Also do not record stats if is_aborted is None, indicating
            # aborting status is unknown yet.
            if collector.is_aborted == False:
                collector.gather_timing_stats()

            if collector.is_aborted == True and is_suite_timeout:
                # There are two possible cases when a suite times out.
                # 1. the suite job was aborted due to timing out
                # 2. the suite job succeeded, but some child jobs
                #    were already aborted before the suite job exited.
                # The case 2 was handled by ResultCollector,
                # here we handle case 1.
                old_code = code
                code = get_worse_code(
                        code, RETURN_CODES.SUITE_TIMEOUT)
                if old_code != code:
                    return_message = 'Suite job timed out.'
                    logging.info('Upgrade return code from %s to %s '
                                 'because suite job has timed out.',
                                 RETURN_CODES.get_string(old_code),
                                 RETURN_CODES.get_string(code))
            if is_suite_timeout:
                logging.info('\nAttempting to diagnose pool: %s', options.pool)
                try:
                    # Add some jitter to make up for any latency in
                    # aborting the suite or checking for results.
                    cutoff = (job_timer.timeout_hours +
                              datetime_base.timedelta(hours=0.3))
                    rpc_helper.diagnose_pool(
                            options.board, options.pool, cutoff)
                except proxy.JSONRPCException as e:
                    logging.warning('Unable to diagnose suite abort.')

        # And output return message.
        if return_message:
            logging.info('Reason: %s', return_message)
            output_dict['return_message'] = return_message

        logging.info('\nOutput below this line is for buildbot consumption:')
        collector.output_buildbot_links()
    else:
        logging.info('Created suite job: %r', job_id)
        link = LogLink(options.name, instance_server,
                       '%s-%s' % (job_id, getpass.getuser()))
        logging.info(link.GenerateBuildbotLink())
        output_dict['return_message'] = '--no_wait specified; Exiting.'
        logging.info('--no_wait specified; Exiting.')
    return (code, output_dict)


def main():
    """Entry point."""
    utils.verify_not_root_user()
    code = RETURN_CODES.OK
    output_dict = {}

    try:
        parser, options, args = parse_options()
        # Silence the log when dumping outputs into json
        if options.json_dump:
            logging.disable(logging.CRITICAL)

        if not verify_options_and_args(options, args):
            parser.print_help()
            code = RETURN_CODES.INVALID_OPTIONS
        else:
            (code, output_dict) = main_without_exception_handling(options)
    except diagnosis_utils.BoardNotAvailableError as e:
        output_dict['return_message'] = 'Skipping testing: %s' % e.message
        code = RETURN_CODES.BOARD_NOT_AVAILABLE
        logging.info(output_dict['return_message'])
    except utils.TestLabException as e:
        output_dict['return_message'] = 'TestLabException: %s' % e
        code = RETURN_CODES.INFRA_FAILURE
        logging.exception(output_dict['return_message'])
    except Exception as e:
        output_dict['return_message'] = 'Unhandled run_suite exception: %s' % e
        code = RETURN_CODES.INFRA_FAILURE
        logging.exception(output_dict['return_message'])

    # Dump test outputs into json.
    output_dict['return_code'] = code
    output_json = json.dumps(output_dict, sort_keys=True)
    if options.json_dump:
        output_json_marked = '#JSON_START#%s#JSON_END#' % output_json.strip()
        sys.stdout.write(output_json_marked)

    logging.info('Will return from run_suite with status: %s',
                  RETURN_CODES.get_string(code))
    autotest_stats.Counter('run_suite.%s' %
                           RETURN_CODES.get_string(code)).increment()
    return code


if __name__ == "__main__":
    sys.exit(main())
