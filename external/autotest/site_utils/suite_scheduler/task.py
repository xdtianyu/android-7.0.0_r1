# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import re
import subprocess

import base_event
import deduping_scheduler
import driver
import manifest_versions
from distutils import version
from constants import Labels
from constants import Builds

import common
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros.dynamic_suite import constants


class MalformedConfigEntry(Exception):
    """Raised to indicate a failure to parse a Task out of a config."""
    pass


BARE_BRANCHES = ['factory', 'firmware']


def PickBranchName(type, milestone):
    """Pick branch name. If type is among BARE_BRANCHES, return type,
    otherwise, return milestone.

    @param type: type of the branch, e.g., 'release', 'factory', or 'firmware'
    @param milestone: CrOS milestone number
    """
    if type in BARE_BRANCHES:
        return type
    return milestone


class TotMilestoneManager(object):
    """A class capable of converting tot string to milestone numbers.

    This class is used as a cache for the tot milestone, so we don't
    repeatedly hit google storage for all O(100) tasks in suite
    scheduler's ini file.
    """

    __metaclass__ = server_utils.Singleton

    # True if suite_scheduler is running for sanity check. When it's set to
    # True, the code won't make gsutil call to get the actual tot milestone to
    # avoid dependency on the installation of gsutil to run sanity check.
    is_sanity = False


    @staticmethod
    def _tot_milestone():
        """Get the tot milestone, eg: R40

        @returns: A string representing the Tot milestone as declared by
            the LATEST_BUILD_URL, or an empty string if LATEST_BUILD_URL
            doesn't exist.
        """
        if TotMilestoneManager.is_sanity:
            logging.info('suite_scheduler is running for sanity purpose, no '
                         'need to get the actual tot milestone string.')
            return 'R40'

        cmd = ['gsutil', 'cat', constants.LATEST_BUILD_URL]
        proc = subprocess.Popen(
                cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        if proc.poll():
            logging.warning('Failed to get latest build: %s', stderr)
            return ''
        return stdout.split('-')[0]


    def refresh(self):
        """Refresh the tot milestone string managed by this class."""
        self.tot = self._tot_milestone()


    def __init__(self):
        """Initialize a TotMilestoneManager."""
        self.refresh()


    def ConvertTotSpec(self, tot_spec):
        """Converts a tot spec to the appropriate milestone.

        Assume tot is R40:
        tot   -> R40
        tot-1 -> R39
        tot-2 -> R38
        tot-(any other numbers) -> R40

        With the last option one assumes that a malformed configuration that has
        'tot' in it, wants at least tot.

        @param tot_spec: A string representing the tot spec.
        @raises MalformedConfigEntry: If the tot_spec doesn't match the
            expected format.
        """
        tot_spec = tot_spec.lower()
        match = re.match('(tot)[-]?(1$|2$)?', tot_spec)
        if not match:
            raise MalformedConfigEntry(
                    "%s isn't a valid branch spec." % tot_spec)
        tot_mstone = self.tot
        num_back = match.groups()[1]
        if num_back:
            tot_mstone_num = tot_mstone.lstrip('R')
            tot_mstone = tot_mstone.replace(
                    tot_mstone_num, str(int(tot_mstone_num)-int(num_back)))
        return tot_mstone


class Task(object):
    """Represents an entry from the scheduler config.  Can schedule itself.

    Each entry from the scheduler config file maps one-to-one to a
    Task.  Each instance has enough info to schedule itself
    on-demand with the AFE.

    This class also overrides __hash__() and all comparator methods to enable
    correct use in dicts, sets, etc.
    """


    @staticmethod
    def CreateFromConfigSection(config, section):
        """Create a Task from a section of a config file.

        The section to parse should look like this:
        [TaskName]
        suite: suite_to_run  # Required
        run_on: event_on which to run  # Required
        hour: integer of the hour to run, only applies to nightly. # Optional
        branch_specs: factory,firmware,>=R12 or ==R12 # Optional
        pool: pool_of_devices  # Optional
        num: sharding_factor  # int, Optional
        boards: board1, board2  # comma seperated string, Optional

        By default, Tasks run on all release branches, not factory or firmware.

        @param config: a ForgivingConfigParser.
        @param section: the section to parse into a Task.
        @return keyword, Task object pair.  One or both will be None on error.
        @raise MalformedConfigEntry if there's a problem parsing |section|.
        """
        if not config.has_section(section):
            raise MalformedConfigEntry('unknown section %s' % section)

        allowed = set(['suite', 'run_on', 'branch_specs', 'pool', 'num',
                       'boards', 'file_bugs', 'cros_build_spec',
                       'firmware_rw_build_spec', 'test_source', 'job_retry',
                       'hour', 'day'])
        # The parameter of union() is the keys under the section in the config
        # The union merges this with the allowed set, so if any optional keys
        # are omitted, then they're filled in. If any extra keys are present,
        # then they will expand unioned set, causing it to fail the following
        # comparison against the allowed set.
        section_headers = allowed.union(dict(config.items(section)).keys())
        if allowed != section_headers:
            raise MalformedConfigEntry('unknown entries: %s' %
                      ", ".join(map(str, section_headers.difference(allowed))))

        keyword = config.getstring(section, 'run_on')
        hour = config.getstring(section, 'hour')
        suite = config.getstring(section, 'suite')
        branches = config.getstring(section, 'branch_specs')
        pool = config.getstring(section, 'pool')
        boards = config.getstring(section, 'boards')
        file_bugs = config.getboolean(section, 'file_bugs')
        cros_build_spec = config.getstring(section, 'cros_build_spec')
        firmware_rw_build_spec = config.getstring(
                section, 'firmware_rw_build_spec')
        test_source = config.getstring(section, 'test_source')
        job_retry = config.getboolean(section, 'job_retry')
        for klass in driver.Driver.EVENT_CLASSES:
            if klass.KEYWORD == keyword:
                priority = klass.PRIORITY
                timeout = klass.TIMEOUT
                break
        else:
            priority = None
            timeout = None
        try:
            num = config.getint(section, 'num')
        except ValueError as e:
            raise MalformedConfigEntry("Ill-specified 'num': %r" % e)
        if not keyword:
            raise MalformedConfigEntry('No event to |run_on|.')
        if not suite:
            raise MalformedConfigEntry('No |suite|')
        try:
            hour = config.getint(section, 'hour')
        except ValueError as e:
            raise MalformedConfigEntry("Ill-specified 'hour': %r" % e)
        if hour is not None and (hour < 0 or hour > 23):
            raise MalformedConfigEntry(
                    '`hour` must be an integer between 0 and 23.')
        if hour is not None and keyword != 'nightly':
            raise MalformedConfigEntry(
                    '`hour` is the trigger time that can only apply to nightly '
                    'event.')

        try:
            day = config.getint(section, 'day')
        except ValueError as e:
            raise MalformedConfigEntry("Ill-specified 'day': %r" % e)
        if day is not None and (day < 0 or day > 6):
            raise MalformedConfigEntry(
                    '`day` must be an integer between 0 and 6, where 0 is for '
                    'Monday and 6 is for Sunday.')
        if day is not None and keyword != 'weekly':
            raise MalformedConfigEntry(
                    '`day` is the trigger of the day of a week, that can only '
                    'apply to weekly events.')

        specs = []
        if branches:
            specs = re.split('\s*,\s*', branches)
            Task.CheckBranchSpecs(specs)
        return keyword, Task(section, suite, specs, pool, num, boards,
                             priority, timeout,
                             file_bugs=file_bugs if file_bugs else False,
                             cros_build_spec=cros_build_spec,
                             firmware_rw_build_spec=firmware_rw_build_spec,
                             test_source=test_source, job_retry=job_retry,
                             hour=hour, day=day)


    @staticmethod
    def CheckBranchSpecs(branch_specs):
        """Make sure entries in the list branch_specs are correctly formed.

        We accept any of BARE_BRANCHES in |branch_specs|, as
        well as _one_ string of the form '>=RXX' or '==RXX', where 'RXX' is a
        CrOS milestone number.

        @param branch_specs: an iterable of branch specifiers.
        @raise MalformedConfigEntry if there's a problem parsing |branch_specs|.
        """
        have_seen_numeric_constraint = False
        for branch in branch_specs:
            if branch in BARE_BRANCHES:
                continue
            if not have_seen_numeric_constraint:
                #TODO(beeps): Why was <= dropped on the floor?
                if branch.startswith('>=R') or branch.startswith('==R'):
                    have_seen_numeric_constraint = True
                elif 'tot' in branch:
                    TotMilestoneManager().ConvertTotSpec(
                            branch[branch.index('tot'):])
                    have_seen_numeric_constraint = True
                continue
            raise MalformedConfigEntry("%s isn't a valid branch spec." % branch)


    def __init__(self, name, suite, branch_specs, pool=None, num=None,
                 boards=None, priority=None, timeout=None, file_bugs=False,
                 cros_build_spec=None, firmware_rw_build_spec=None,
                 test_source=None, job_retry=False, hour=None, day=None):
        """Constructor

        Given an iterable in |branch_specs|, pre-vetted using CheckBranchSpecs,
        we'll store them such that _FitsSpec() can be used to check whether a
        given branch 'fits' with the specifications passed in here.
        For example, given branch_specs = ['factory', '>=R18'], we'd set things
        up so that _FitsSpec() would return True for 'factory', or 'RXX'
        where XX is a number >= 18. Same check is done for branch_specs = [
        'factory', '==R18'], which limit the test to only one specific branch.

        Given branch_specs = ['factory', 'firmware'], _FitsSpec()
        would pass only those two specific strings.

        Example usage:
          t = Task('Name', 'suite', ['factory', '>=R18'])
          t._FitsSpec('factory')  # True
          t._FitsSpec('R19')  # True
          t._FitsSpec('R17')  # False
          t._FitsSpec('firmware')  # False
          t._FitsSpec('goober')  # False

          t = Task('Name', 'suite', ['factory', '==R18'])
          t._FitsSpec('R19')  # False, branch does not equal to 18
          t._FitsSpec('R18')  # True
          t._FitsSpec('R17')  # False

        cros_build_spec and firmware_rw_build_spec are set for tests require
        firmware update on the dut. Only one of them can be set.
        For example:
        branch_specs: ==tot
        firmware_rw_build_spec: firmware
        test_source: cros
        This will run test using latest build on firmware branch, and the latest
        ChromeOS build on ToT. The test source build is ChromeOS build.

        branch_specs: firmware
        cros_build_spec: ==tot-1
        test_source: firmware_rw
        This will run test using latest build on firmware branch, and the latest
        ChromeOS build on dev channel (ToT-1). The test source build is the
        firmware RW build.

        branch_specs: ==tot
        firmware_rw_build_spec: cros
        test_source: cros
        This will run test using latest ChromeOS and firmware RW build on ToT.
        ChromeOS build on ToT. The test source build is ChromeOS build.

        @param name: name of this task, e.g. 'NightlyPower'
        @param suite: the name of the suite to run, e.g. 'bvt'
        @param branch_specs: a pre-vetted iterable of branch specifiers,
                             e.g. ['>=R18', 'factory']
        @param pool: the pool of machines to use for scheduling purposes.
                     Default: None
        @param num: the number of devices across which to shard the test suite.
                    Type: integer or None
                    Default: None
        @param boards: A comma separated list of boards to run this task on.
                       Default: Run on all boards.
        @param priority: The string name of a priority from
                         client.common_lib.priorities.Priority.
        @param timeout: The max lifetime of the suite in hours.
        @param file_bugs: True if bug filing is desired for the suite created
                          for this task.
        @param cros_build_spec: Spec used to determine the ChromeOS build to
                                test with a firmware build, e.g., tot, R41 etc.
        @param firmware_rw_build_spec: Spec used to determine the firmware build
                                       test with a ChromeOS build.
        @param test_source: The source of test code when firmware will be
                            updated in the test. The value can be `firmware_rw`
                            or `cros`.
        @param job_retry: Set to True to enable job-level retry. Default is
                          False.
        @param hour: An integer specifying the hour that a nightly run should
                     be triggered, default is set to 21.
        @param day: An integer specifying the day of a week that a weekly run
                    should be triggered, default is set to 5, which is Saturday.
        """
        self._name = name
        self._suite = suite
        self._branch_specs = branch_specs
        self._pool = pool
        self._num = num
        self._priority = priority
        self._timeout = timeout
        self._file_bugs = file_bugs
        self._cros_build_spec = cros_build_spec
        self._firmware_rw_build_spec = firmware_rw_build_spec
        self._test_source = test_source
        self._job_retry = job_retry
        self.hour = hour
        self.day = day

        if ((self._firmware_rw_build_spec or cros_build_spec) and
            not self.test_source in [Builds.FIRMWARE_RW, Builds.CROS]):
            raise MalformedConfigEntry(
                    'You must specify the build for test source. It can only '
                    'be `firmware_rw` or `cros`.')
        if self._firmware_rw_build_spec and cros_build_spec:
            raise MalformedConfigEntry(
                    'You cannot specify both firmware_rw_build_spec and '
                    'cros_build_spec. firmware_rw_build_spec is used to specify'
                    ' a firmware build when the suite requires firmware to be '
                    'updated in the dut, its value can only be `firmware` or '
                    '`cros`. cros_build_spec is used to specify a ChromeOS '
                    'build when build_specs is set to firmware.')
        if (self._firmware_rw_build_spec and
            self._firmware_rw_build_spec not in ['firmware', 'cros']):
            raise MalformedConfigEntry(
                    'firmware_rw_build_spec can only be empty, firmware or '
                    'cros. It does not support other build type yet.')

        self._bare_branches = []
        self._version_equal_constraint = False
        self._version_gte_constraint = False
        self._version_lte_constraint = False
        if not branch_specs:
            # Any milestone is OK.
            self._numeric_constraint = version.LooseVersion('0')
        else:
            self._numeric_constraint = None
            for spec in branch_specs:
                if 'tot' in spec.lower():
                    tot_str = spec[spec.index('tot'):]
                    spec = spec.replace(
                            tot_str, TotMilestoneManager().ConvertTotSpec(
                                    tot_str))
                if spec.startswith('>='):
                    self._numeric_constraint = version.LooseVersion(
                            spec.lstrip('>=R'))
                    self._version_gte_constraint = True
                elif spec.startswith('<='):
                    self._numeric_constraint = version.LooseVersion(
                            spec.lstrip('<=R'))
                    self._version_lte_constraint = True
                elif spec.startswith('=='):
                    self._version_equal_constraint = True
                    self._numeric_constraint = version.LooseVersion(
                            spec.lstrip('==R'))
                else:
                    self._bare_branches.append(spec)

        # Since we expect __hash__() and other comparator methods to be used
        # frequently by set operations, and they use str() a lot, pre-compute
        # the string representation of this object.
        if num is None:
            numStr = '[Default num]'
        else:
            numStr = '%d' % num

        if boards is None:
            self._boards = set()
            boardsStr = '[All boards]'
        else:
            self._boards = set([x.strip() for x in boards.split(',')])
            boardsStr = boards

        self._str = ('%s: %s on %s with pool %s, boards [%s], file_bugs = %s '
                     'across %s machines.' % (self.__class__.__name__,
                     suite, branch_specs, pool, boardsStr, self._file_bugs,
                     numStr))


    def _FitsSpec(self, branch):
        """Checks if a branch is deemed OK by this instance's branch specs.

        When called on a branch name, will return whether that branch
        'fits' the specifications stored in self._bare_branches,
        self._numeric_constraint, self._version_equal_constraint,
        self._version_gte_constraint and self._version_lte_constraint.

        @param branch: the branch to check.
        @return True if b 'fits' with stored specs, False otherwise.
        """
        if branch in BARE_BRANCHES:
            return branch in self._bare_branches
        if self._numeric_constraint:
            if self._version_equal_constraint:
                return version.LooseVersion(branch) == self._numeric_constraint
            elif self._version_gte_constraint:
                return version.LooseVersion(branch) >= self._numeric_constraint
            elif self._version_lte_constraint:
                return version.LooseVersion(branch) <= self._numeric_constraint
            else:
                # Default to great or equal constraint.
                return version.LooseVersion(branch) >= self._numeric_constraint
        else:
            return False


    @property
    def name(self):
        """Name of this task, e.g. 'NightlyPower'."""
        return self._name


    @property
    def suite(self):
        """Name of the suite to run, e.g. 'bvt'."""
        return self._suite


    @property
    def branch_specs(self):
        """a pre-vetted iterable of branch specifiers,
        e.g. ['>=R18', 'factory']."""
        return self._branch_specs


    @property
    def pool(self):
        """The pool of machines to use for scheduling purposes."""
        return self._pool


    @property
    def num(self):
        """The number of devices across which to shard the test suite.
        Type: integer or None"""
        return self._num


    @property
    def boards(self):
        """The boards on which to run this suite.
        Type: Iterable of strings"""
        return self._boards


    @property
    def priority(self):
        """The priority of the suite"""
        return self._priority


    @property
    def timeout(self):
        """The maximum lifetime of the suite in hours."""
        return self._timeout


    @property
    def cros_build_spec(self):
        """The build spec of ChromeOS to test with a firmware build."""
        return self._cros_build_spec


    @property
    def firmware_rw_build_spec(self):
        """The build spec of firmware to test with a ChromeOS build."""
        return self._firmware_rw_build_spec


    @property
    def test_source(self):
        """Source of the test code, value can be `firmware_rw` or `cros`."""
        return self._test_source


    def __str__(self):
        return self._str


    def __repr__(self):
        return self._str


    def __lt__(self, other):
        return str(self) < str(other)


    def __le__(self, other):
        return str(self) <= str(other)


    def __eq__(self, other):
        return str(self) == str(other)


    def __ne__(self, other):
        return str(self) != str(other)


    def __gt__(self, other):
        return str(self) > str(other)


    def __ge__(self, other):
        return str(self) >= str(other)


    def __hash__(self):
        """Allows instances to be correctly deduped when used in a set."""
        return hash(str(self))


    def _GetCrOSBuild(self, mv, board):
        """Get the ChromeOS build name to test with firmware build.

        The ChromeOS build to be used is determined by `self.cros_build_spec`.
        Its value can be:
        tot: use the latest ToT build.
        tot-x: use the latest build in x milestone before ToT.
        Rxx: use the latest build on xx milestone.

        @param board: the board against which to run self._suite.
        @param mv: an instance of manifest_versions.ManifestVersions.

        @return: The ChromeOS build name to test with firmware build.

        """
        if not self.cros_build_spec:
            return None
        if self.cros_build_spec.startswith('tot'):
            milestone = TotMilestoneManager().ConvertTotSpec(
                    self.cros_build_spec)[1:]
        elif self.cros_build_spec.startswith('R'):
            milestone = self.cros_build_spec[1:]
        milestone, latest_manifest = mv.GetLatestManifest(
                board, 'release', milestone=milestone)
        latest_build = base_event.BuildName(board, 'release', milestone,
                                            latest_manifest)
        logging.debug('Found latest build of %s for spec %s: %s',
                      board, self.cros_build_spec, latest_build)
        return latest_build


    def _GetFirmwareRWBuild(self, mv, board, build_type):
        """Get the firmware rw build name to test with ChromeOS build.

        The firmware rw build to be used is determined by
        `self.firmware_rw_build_spec`. Its value can be `firmware`, `cros` or
        empty:
        firmware: use the ToT build in firmware branch.
        cros: use the ToT build in release (ChromeOS) branch.

        @param mv: an instance of manifest_versions.ManifestVersions.
        @param board: the board against which to run self._suite.
        @param build_type: Build type of the firmware build, e.g., factory,
                           firmware or release.

        @return: The firmware rw build name to test with ChromeOS build.

        """
        if not self.firmware_rw_build_spec:
            return None
        latest_milestone, latest_manifest = mv.GetLatestManifest(
                board, build_type)
        latest_build = base_event.BuildName(board, build_type, latest_milestone,
                                            latest_manifest)
        logging.debug('Found latest firmware build of %s for spec %s: %s',
                      board, self.firmware_rw_build_spec, latest_build)
        return latest_build


    def AvailableHosts(self, scheduler, board):
        """Query what hosts are able to run a test on a board and pool
        combination.

        @param scheduler: an instance of DedupingScheduler, as defined in
                          deduping_scheduler.py
        @param board: the board against which one wants to run the test.
        @return The list of hosts meeting the board and pool requirements,
                or None if no hosts were found."""
        if self._boards and board not in self._boards:
            return []

        labels = [Labels.BOARD_PREFIX + board]
        if self._pool:
            labels.append(Labels.POOL_PREFIX + self._pool)

        return scheduler.CheckHostsExist(multiple_labels=labels)


    def ShouldHaveAvailableHosts(self):
        """As a sanity check, return true if we know for certain that
        we should be able to schedule this test. If we claim this test
        should be able to run, and it ends up not being scheduled, then
        a warning will be reported.

        @return True if this test should be able to run, False otherwise.
        """
        return self._pool == 'bvt'


    def Run(self, scheduler, branch_builds, board, force=False, mv=None):
        """Run this task.  Returns False if it should be destroyed.

        Execute this task.  Attempt to schedule the associated suite.
        Return True if this task should be kept around, False if it
        should be destroyed.  This allows for one-shot Tasks.

        @param scheduler: an instance of DedupingScheduler, as defined in
                          deduping_scheduler.py
        @param branch_builds: a dict mapping branch name to the build(s) to
                              install for that branch, e.g.
                              {'R18': ['x86-alex-release/R18-1655.0.0'],
                               'R19': ['x86-alex-release/R19-2077.0.0']}
        @param board: the board against which to run self._suite.
        @param force: Always schedule the suite.
        @param mv: an instance of manifest_versions.ManifestVersions.

        @return True if the task should be kept, False if not

        """
        logging.info('Running %s on %s', self._name, board)
        is_firmware_build = 'firmware' in self.branch_specs
        # firmware_rw_build is only needed if firmware_rw_build_spec is given.
        firmware_rw_build = None
        try:
            if is_firmware_build:
                # When build specified in branch_specs is a firmware build,
                # we need a ChromeOS build to test with the firmware build.
                cros_build = self._GetCrOSBuild(mv, board)
            elif self.firmware_rw_build_spec:
                # When firmware_rw_build_spec is specified, the test involves
                # updating the firmware by firmware build specified in
                # firmware_rw_build_spec.
                if self.firmware_rw_build_spec == 'cros':
                    build_type = 'release'
                else:
                    build_type = self.firmware_rw_build_spec
                firmware_rw_build = self._GetFirmwareRWBuild(
                        mv, board, build_type)
        except manifest_versions.QueryException as e:
            logging.error(e)
            logging.error('Running %s on %s is failed. Failed to find build '
                          'required to run the suite.', self._name, board)
            return False

        builds = []
        for branch, build in branch_builds.iteritems():
            logging.info('Checking if %s fits spec %r',
                         branch, self.branch_specs)
            if self._FitsSpec(branch):
                logging.debug('Build %s fits the spec.', build)
                builds.extend(build)
        for build in builds:
            try:
                if is_firmware_build:
                    firmware_rw_build = build
                else:
                    cros_build = build
                if self.test_source == Builds.FIRMWARE_RW:
                    test_source_build = firmware_rw_build
                elif self.test_source == Builds.CROS:
                    test_source_build = cros_build
                else:
                    test_source_build = None
                logging.debug('Schedule %s for builds %s.%s',
                              self._suite, builds,
                              (' Test source build is %s.' % test_source_build)
                              if test_source_build else None)

                if not scheduler.ScheduleSuite(
                        self._suite, board, cros_build, self._pool, self._num,
                        self._priority, self._timeout, force,
                        file_bugs=self._file_bugs,
                        firmware_rw_build=firmware_rw_build,
                        test_source_build=test_source_build,
                        job_retry=self._job_retry):
                    logging.info('Skipping scheduling %s on %s for %s',
                                 self._suite, builds, board)
            except deduping_scheduler.DedupingSchedulerException as e:
                logging.error(e)
        return True


class OneShotTask(Task):
    """A Task that can be run only once.  Can schedule itself."""


    def Run(self, scheduler, branch_builds, board, force=False, mv=None):
        """Run this task.  Returns False, indicating it should be destroyed.

        Run this task.  Attempt to schedule the associated suite.
        Return False, indicating to the caller that it should discard this task.

        @param scheduler: an instance of DedupingScheduler, as defined in
                          deduping_scheduler.py
        @param branch_builds: a dict mapping branch name to the build(s) to
                              install for that branch, e.g.
                              {'R18': ['x86-alex-release/R18-1655.0.0'],
                               'R19': ['x86-alex-release/R19-2077.0.0']}
        @param board: the board against which to run self._suite.
        @param force: Always schedule the suite.
        @param mv: an instance of manifest_versions.ManifestVersions.

        @return False

        """
        super(OneShotTask, self).Run(scheduler, branch_builds, board, force,
                                     mv)
        return False
