#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Infer and spawn a complete set of Chrome OS release autoupdate tests.

By default, this runs against the AFE configured in the global_config.ini->
SERVER->hostname. You can run this on a local AFE by modifying this value in
your shadow_config.ini to localhost.
"""

import logging
import optparse
import os
import re
import sys

import common
from autotest_lib.client.common_lib import priorities
from autotest_lib.server import frontend
from autotest_lib.utils import external_packages

from autotest_lib.site_utils.autoupdate import import_common
from autotest_lib.site_utils.autoupdate import release as release_util
from autotest_lib.site_utils.autoupdate import test_image
from autotest_lib.site_utils.autoupdate.lib import test_control
from autotest_lib.site_utils.autoupdate.lib import test_params

chromite = import_common.download_and_import('chromite',
                                             external_packages.ChromiteRepo())

# Autotest pylint is more restrictive than it should with args.
#pylint: disable=C0111

# Global reference objects.
_release_info = release_util.ReleaseInfo()

_log_debug = 'debug'
_log_normal = 'normal'
_log_verbose = 'verbose'
_valid_log_levels = _log_debug, _log_normal, _log_verbose
_autotest_url_format = r'http://%(host)s/afe/#tab_id=view_job&object_id=%(job)s'
_default_dump_dir = os.path.realpath(
        os.path.join(os.path.dirname(__file__), '..', '..', 'server',
                     'site_tests', test_control.get_test_name()))
_build_version = '%(branch)s-%(release)s'


class FullReleaseTestError(BaseException):
  pass


def get_release_branch(release):
    """Returns the release branch for the given release.

    @param release: release version e.g. 3920.0.0.

    @returns the branch string e.g. R26.
    """
    return _release_info.get_branch(release)


class TestConfigGenerator(object):
    """Class for generating test configs."""

    def __init__(self, board, tested_release, test_nmo, test_npo,
                 archive_url=None):
        """
        @param board: the board under test
        @param tested_release: the tested release version
        @param test_nmo: whether we should infer N-1 tests
        @param test_npo: whether we should infer N+1 tests
        @param archive_url: optional gs url to find payloads.

        """
        self.board = board
        self.tested_release = tested_release
        self.test_nmo = test_nmo
        self.test_npo = test_npo
        if archive_url:
            self.archive_url = archive_url
        else:
            branch = get_release_branch(tested_release)
            build_version = _build_version % dict(branch=branch,
                                                  release=tested_release)
            self.archive_url = test_image.get_default_archive_url(
                    board, build_version)

        # Get the prefix which is an archive_url stripped of its trailing
        # version. We rstrip in the case of any trailing /'s.
        # Use archive prefix for any nmo / specific builds.
        self.archive_prefix = self.archive_url.rstrip('/').rpartition('/')[0]


    def _get_source_uri_from_build_version(self, build_version):
        """Returns the source_url given build version.

        Args:
            build_version: the full build version i.e. R27-3823.0.0-a2.
        """
        # If we're looking for our own image, use the target archive_url if set
        if self.tested_release in build_version:
            archive_url = self.archive_url
        else:
            archive_url = test_image.get_archive_url_from_prefix(
                    self.archive_prefix, build_version)

        return test_image.find_payload_uri(archive_url, single=True)


    def _get_source_uri_from_release(self, release):
        """Returns the source uri for a given release or None if not found.

        Args:
            release: required release number.
        """
        branch = get_release_branch(release)
        return self._get_source_uri_from_build_version(
                _build_version % dict(branch=branch, release=release))


    def generate_test_image_config(self, name, is_delta_update, source_release,
                                   payload_uri, source_uri):
        """Constructs a single test config with given arguments.

        It'll automatically find and populate source/target branches as well as
        the source image URI.

        @param name: a descriptive name for the test
        @param is_delta_update: whether we're testing a delta update
        @param source_release: the version of the source image (before update)
        @param target_release: the version of the target image (after update)
        @param payload_uri: URI of the update payload.
        @param source_uri:  URI of the source image/payload.

        """
        # Extracts just the main version from a version that may contain
        # attempts or a release candidate suffix i.e. 3928.0.0-a2 ->
        # base_version=3928.0.0.
        _version_re = re.compile(
            '(?P<base_version>[0-9.]+)(?:\-[a-z]+[0-9]+])*')

        # Pass only the base versions without any build specific suffixes.
        source_version = _version_re.match(source_release).group('base_version')
        target_version = _version_re.match(self.tested_release).group(
                'base_version')
        return test_params.TestConfig(
                self.board, name, is_delta_update, source_version,
                target_version, source_uri, payload_uri)


    @staticmethod
    def _parse_build_version(build_version):
        """Returns a branch, release tuple from a full build_version.

        Args:
            build_version: build version to parse e.g. 'R27-3905.0.0'
        """
        version = r'[0-9a-z.\-]+'
        # The date portion only appears in non-release builds.
        date = r'([0-9]+_[0-9]+_[0-9]+_[0-9]+)*'
        # Returns groups for branches and release numbers from build version.
        _build_version_re = re.compile(
            '(?P<branch>R[0-9]+)-(?P<release>' + version + date + version + ')')

        match = _build_version_re.match(build_version)
        if not match:
            logging.warning('version %s did not match version format',
                            build_version)
            return None

        return match.group('branch'), match.group('release')


    @staticmethod
    def _parse_delta_filename(filename):
        """Parses a delta payload name into its source/target versions.

        Args:
            filename: Delta filename to parse e.g.
                      'chromeos_R27-3905.0.0_R27-3905.0.0_stumpy_delta_dev.bin'

        Returns: tuple with source_version, and target_version.
        """
        version = r'[0-9a-z.\-]+'
        # The date portion only appears in non-release builds.
        date = r'([0-9]+_[0-9]+_[0-9]+_[0-9]+)*'
        # Matches delta format name and returns groups for source and target
        # versions.
        _delta_re = re.compile(
            'chromeos_'
            '(?P<s_version>R[0-9]+-' + version + date + version + ')'
            '_'
            '(?P<t_version>R[0-9]+-' + version + date + version + ')'
            '_[\w.]+')
        match = _delta_re.match(filename)
        if not match:
            logging.warning('filename %s did not match delta format', filename)
            return None

        return match.group('s_version'), match.group('t_version')


    def generate_npo_nmo_list(self):
        """Generates N+1/N-1 test configurations.

        Computes a list of N+1 (npo) and/or N-1 (nmo) test configurations for a
        given tested release and board. This is done by scanning of the test
        image repository, looking for update payloads; normally, we expect to
        find at most one update payload of each of the aforementioned types.

        @return A list of TestConfig objects corresponding to the N+1 and N-1
                tests.

        @raise FullReleaseTestError if something went wrong

        """
        if not (self.test_nmo or self.test_npo):
            return []

        # Find all test delta payloads involving the release version at hand,
        # then figure out which is which.
        found = set()
        test_list = []
        payload_uri_list = test_image.find_payload_uri(
                self.archive_url, delta=True)
        for payload_uri in payload_uri_list:
            # Infer the source and target release versions. These versions will
            # be something like 'R43-6831.0.0' for release builds and
            # 'R43-6831.0.0-a1' for trybots.
            file_name = os.path.basename(payload_uri)
            source_version, target_version = (
                    self._parse_delta_filename(file_name))
            _, source_release = self._parse_build_version(source_version)

            # The target version should contain the tested release otherwise
            # this is a delta payload to a different version. They are not equal
            # since the tested_release doesn't include the milestone, for
            # example, 940.0.1 release in the R28-940.0.1-a1 version.
            if self.tested_release not in target_version:
                raise FullReleaseTestError(
                        'delta target release %s does not contain %s (%s)',
                        target_version, self.tested_release, self.board)

            # Search for the full payload to the source_version in the
            # self.archive_url directory if the source_version is the tested
            # release (such as in a npo test), or in the standard location if
            # the source is some other build. Note that this function handles
            # both cases.
            source_uri = self._get_source_uri_from_build_version(source_version)

            if not source_uri:
                logging.warning('cannot find source for %s, %s', self.board,
                                source_version)
                continue

            # Determine delta type, make sure it was not already discovered.
            delta_type = 'npo' if source_version == target_version else 'nmo'
            # Only add test configs we were asked to test.
            if (delta_type == 'npo' and not self.test_npo) or (
                delta_type == 'nmo' and not self.test_nmo):
                continue

            if delta_type in found:
                raise FullReleaseTestError(
                        'more than one %s deltas found (%s, %s)' % (
                        delta_type, self.board, self.tested_release))

            found.add(delta_type)

            # Generate test configuration.
            test_list.append(self.generate_test_image_config(
                    delta_type, True, source_release, payload_uri, source_uri))

        return test_list


    def generate_specific_list(self, specific_source_releases, generated_tests):
        """Generates test configurations for a list of specific source releases.

        Returns a list of test configurations from a given list of releases to
        the given tested release and board. Cares to exclude test configurations
        that were already generated elsewhere (e.g. N-1/N+1).

        @param specific_source_releases: list of source release to test
        @param generated_tests: already generated test configuration

        @return List of TestConfig objects corresponding to the specific source
                releases, minus those that were already generated elsewhere.

        """
        generated_source_releases = [
                test_config.source_release for test_config in generated_tests]
        filtered_source_releases = [rel for rel in specific_source_releases
                                    if rel not in generated_source_releases]
        if not filtered_source_releases:
            return []

        # Find the full payload for the target release.
        tested_payload_uri = test_image.find_payload_uri(
                self.archive_url, single=True)
        if not tested_payload_uri:
            logging.warning("cannot find full payload for %s, %s; no specific "
                            "tests generated",
                            self.board, self.tested_release)
            return []

        # Construct test list.
        test_list = []
        for source_release in filtered_source_releases:
            source_uri = self._get_source_uri_from_release(source_release)
            if not source_uri:
                logging.warning('cannot find source for %s, %s', self.board,
                                source_release)
                continue

            test_list.append(self.generate_test_image_config(
                    'specific', False, source_release, tested_payload_uri,
                    source_uri))

        return test_list


def generate_test_list(args):
    """Setup the test environment.

    @param args: execution arguments

    @return A list of test configurations.

    @raise FullReleaseTestError if anything went wrong.

    """
    # Initialize test list.
    test_list = []

    for board in args.tested_board_list:
        test_list_for_board = []
        generator = TestConfigGenerator(
                board, args.tested_release, args.test_nmo, args.test_npo,
                args.archive_url)

        # Configure N-1-to-N and N-to-N+1 tests.
        if args.test_nmo or args.test_npo:
            test_list_for_board += generator.generate_npo_nmo_list()

        # Add tests for specifically provided source releases.
        if args.specific:
            test_list_for_board += generator.generate_specific_list(
                    args.specific, test_list_for_board)

        test_list += test_list_for_board

    return test_list


def run_test_afe(test, env, control_code, afe, dry_run):
    """Run an end-to-end update test via AFE.

    @param test: the test configuration
    @param env: environment arguments for the test
    @param control_code: content of the test control file
    @param afe: instance of server.frontend.AFE to use to create job.
    @param dry_run: If True, don't actually run the test against the afe.

    @return The scheduled job ID or None if dry_run.

    """
    # Parametrize the control script.
    parametrized_control_code = test_control.generate_full_control_file(
            test, env, control_code)

    # Create the job.
    meta_hosts = ['board:%s' % test.board]

    dependencies = ['pool:suites']
    logging.debug('scheduling afe test: meta_hosts=%s dependencies=%s',
                  meta_hosts, dependencies)
    if not dry_run:
        job = afe.create_job(
                parametrized_control_code,
                name=test.get_autotest_name(),
                priority=priorities.Priority.DEFAULT,
                control_type='Server', meta_hosts=meta_hosts,
                dependencies=dependencies)
        return job.id
    else:
        logging.info('Would have run scheduled test %s against afe', test.name)


def get_job_url(server, job_id):
    """Returns the url for a given job status.

    @param server: autotest server.
    @param job_id: job id for the job created.

    @return the url the caller can use to track the job status.
    """
    # Explicitly print as this is what a caller looks for.
    return 'Job submitted to autotest afe. To check its status go to: %s' % (
            _autotest_url_format % dict(host=server, job=job_id))


def parse_args(argv):
    parser = optparse.OptionParser(
            usage='Usage: %prog [options] RELEASE [BOARD...]',
            description='Schedule Chrome OS release update tests on given '
                        'board(s).')

    parser.add_option('--archive_url', metavar='URL',
                      help='Use this archive url to find the target payloads.')
    parser.add_option('--dump', default=False, action='store_true',
                      help='dump control files that would be used in autotest '
                           'without running them. Implies --dry_run')
    parser.add_option('--dump_dir', default=_default_dump_dir,
                      help='directory to dump control files generated')
    parser.add_option('--nmo', dest='test_nmo', action='store_true',
                      help='generate N-1 update tests')
    parser.add_option('--npo', dest='test_npo', action='store_true',
                      help='generate N+1 update tests')
    parser.add_option('--specific', metavar='LIST',
                      help='comma-separated list of source releases to '
                           'generate test configurations from')
    parser.add_option('--skip_boards', dest='skip_boards',
                      help='boards to skip, separated by comma.')
    parser.add_option('--omaha_host', metavar='ADDR',
                      help='Optional host where Omaha server will be spawned.'
                      'If not set, localhost is used.')
    parser.add_option('-n', '--dry_run', action='store_true',
                      help='do not invoke actual test runs')
    parser.add_option('--log', metavar='LEVEL', dest='log_level',
                      default=_log_verbose,
                      help='verbosity level: %s' % ' '.join(_valid_log_levels))

    # Parse arguments.
    opts, args = parser.parse_args(argv)

    # Get positional arguments, adding them as option values.
    if len(args) < 1:
        parser.error('missing arguments')

    opts.tested_release = args[0]
    opts.tested_board_list = args[1:]
    if not opts.tested_board_list:
        parser.error('No boards listed.')

    # Skip specific board.
    if opts.skip_boards:
        opts.skip_boards = opts.skip_boards.split(',')
        opts.tested_board_list = [board for board in opts.tested_board_list
                                  if board not in opts.skip_boards]

    # Sanity check log level.
    if opts.log_level not in _valid_log_levels:
        parser.error('invalid log level (%s)' % opts.log_level)

    if opts.dump:
        opts.dry_run = True

    # Process list of specific source releases.
    opts.specific = opts.specific.split(',') if opts.specific else []

    return opts


def main(argv):
    try:
        # Initialize release config.
        _release_info.initialize()

        # Parse command-line arguments.
        args = parse_args(argv)

        # Set log verbosity.
        if args.log_level == _log_debug:
            logging.basicConfig(level=logging.DEBUG)
        elif args.log_level == _log_verbose:
            logging.basicConfig(level=logging.INFO)
        else:
            logging.basicConfig(level=logging.WARNING)

        # Create test configurations.
        test_list = generate_test_list(args)
        if not test_list:
            raise FullReleaseTestError(
                'no test configurations generated, nothing to do')

        # Construct environment argument, used for all tests.
        env = test_params.TestEnv(args)

        # Obtain the test control file content.
        with open(test_control.get_control_file_name()) as f:
            control_code = f.read()

        # Dump control file(s) to be staged later, or schedule upfront?
        if args.dump:
            # Populate and dump test-specific control files.
            for test in test_list:
                # Control files for the same board are all in the same
                # sub-dir.
                directory = os.path.join(args.dump_dir, test.board)
                test_control_file = test_control.dump_autotest_control_file(
                        test, env, control_code, directory)
                logging.info('dumped control file for test %s to %s',
                             test, test_control_file)
        else:
            # Schedule jobs via AFE.
            afe = frontend.AFE(debug=(args.log_level == _log_debug))
            for test in test_list:
                logging.info('scheduling test %s', test)
                try:
                    job_id = run_test_afe(test, env, control_code,
                                          afe, args.dry_run)
                    if job_id:
                        # Explicitly print as this is what a caller looks
                        # for.
                        print get_job_url(afe.server, job_id)
                except Exception:
                    # Note we don't print the exception here as the afe
                    # will print it out already.
                    logging.error('Failed to schedule test %s. '
                                  'Please check exception and re-run this '
                                  'board manually if needed.', test)


    except FullReleaseTestError, e:
        logging.fatal(str(e))
        return 1
    else:
        return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
