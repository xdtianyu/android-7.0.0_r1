# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import os
import re
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import threading

import logging
# Turn the logging level to INFO before importing other autotest
# code, to avoid having failed import logging messages confuse the
# test_that user.
logging.basicConfig(level=logging.INFO)

import common
from autotest_lib.client.common_lib.cros import dev_server, retry
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.server.cros.dynamic_suite import suite, constants
from autotest_lib.server.cros import provision
from autotest_lib.server.hosts import factory
from autotest_lib.server import autoserv_utils
from autotest_lib.server import server_logging_config
from autotest_lib.server import utils


_autoserv_proc = None
_sigint_handler_lock = threading.Lock()

_AUTOSERV_SIGINT_TIMEOUT_SECONDS = 5
NO_BOARD = 'ad_hoc_board'
NO_BUILD = 'ad_hoc_build'
_SUITE_REGEX = r'suite:(.*)'

_TEST_KEY_FILENAME = 'testing_rsa'
TEST_KEY_PATH = ('/mnt/host/source/src/scripts/mod_for_test_scripts/'
                  'ssh_keys/%s' % _TEST_KEY_FILENAME)

_LATEST_RESULTS_DIRECTORY = '/tmp/test_that_latest'


class TestThatRunError(Exception):
    """Raised if test_that encounters something unexpected while running."""


class TestThatProvisioningError(Exception):
    """Raised when it fails to provision the DUT to the requested build."""


def add_common_args(parser):
    """
    Add common arguments for both test_that and test_droid to their parser.

    @param parser: argparse.ArgumentParser object to add arguments to.
    """
    parser.add_argument('tests', nargs='+', metavar='TEST',
                        help='Run given test(s). Use suite:SUITE to specify '
                             'test suite. Use e:[NAME_PATTERN] to specify a '
                             'NAME-matching regular expression. Use '
                             'f:[FILE_PATTERN] to specify a filename matching '
                             'regular expression. Specified regular '
                             'expressions will be implicitly wrapped in '
                             '^ and $.')
    parser.add_argument('--fast', action='store_true', dest='fast_mode',
                        default=False,
                        help='Enable fast mode.  This will cause test_droid '
                             'to skip time consuming steps like sysinfo and '
                             'collecting crash information.')
    parser.add_argument('--args', metavar='ARGS',
                        help='Whitespace separated argument string to pass '
                             'through to test. Only supported for runs '
                             'against a local DUT.')
    parser.add_argument('--results_dir', metavar='RESULTS_DIR', default=None,
                        help='Instead of storing results in a new subdirectory'
                             ' of /tmp , store results in RESULTS_DIR. If '
                             'RESULTS_DIR already exists, it will be deleted.')
    parser.add_argument('--pretend', action='store_true', default=False,
                        help='Print autoserv commands that would be run, '
                             'rather than running them.')
    parser.add_argument('--no-experimental', action='store_true',
                        default=False, dest='no_experimental',
                        help='When scheduling a suite, skip any tests marked '
                             'as experimental. Applies only to tests scheduled'
                             ' via suite:[SUITE].')
    parser.add_argument('--enforce-deps', action='store_true',
                        default=False, dest='enforce_deps',
                        help='Skip tests whose DEPENDENCIES can not '
                             'be satisfied.')
    parser.add_argument('--debug', action='store_true',
                        help='Include DEBUG level messages in stdout. Note: '
                             'these messages will be included in output log '
                             'file regardless. In addition, turn on autoserv '
                             'verbosity.')
    parser.add_argument('--iterations', action='store', type=int, default=1,
                        help='Number of times to run the tests specified.')
    parser.add_argument('--ssh_verbosity', action='store', type=int,
                        choices=[0, 1, 2, 3], default=0,
                        help='Verbosity level for ssh, between 0 and 3 '
                             'inclusive.')
    parser.add_argument('--ssh_options', action='store', default=None,
                        help='A string giving additional options to be '
                        'added to ssh commands.')



def fetch_local_suite(autotest_path, suite_predicate, afe, test_arg, remote,
                      build=NO_BUILD, board=NO_BOARD,
                      results_directory=None, no_experimental=False,
                      ignore_deps=True):
    """Create a suite from the given suite predicate.

    Satisfaction of dependencies is enforced by Suite.schedule() if
    ignore_deps is False. Note that this method assumes only one host,
    i.e. |remote|, was added to afe. Suite.schedule() will not
    schedule a job if none of the hosts in the afe (in our case,
    just one host |remote|) has a label that matches a requested
    test dependency.

    @param autotest_path: Absolute path to autotest (in sysroot or
                          custom autotest directory set by --autotest_dir).
    @param suite_predicate: callable that takes ControlData objects, and
                            returns True on those that should be in suite
    @param afe: afe object to schedule against (typically a directAFE)
    @param test_arg: String. An individual TEST command line argument, e.g.
                     'login_CryptohomeMounted' or 'suite:smoke'.
    @param remote: String representing the IP of the remote host.
    @param build: Build to schedule suite for.
    @param board: Board to schedule suite for.
    @param results_directory: Absolute path of directory to store results in.
                              (results will be stored in subdirectory of this).
    @param no_experimental: Skip experimental tests when scheduling a suite.
    @param ignore_deps: If True, test dependencies will be ignored.

    @returns: A suite.Suite object.

    """
    fs_getter = suite.Suite.create_fs_getter(autotest_path)
    devserver = dev_server.ImageServer('')
    my_suite = suite.Suite.create_from_predicates([suite_predicate],
            {provision.CROS_VERSION_PREFIX: build},
            constants.BOARD_PREFIX + board,
            devserver, fs_getter, afe=afe,
            ignore_deps=ignore_deps,
            results_dir=results_directory, forgiving_parser=False)
    if len(my_suite.tests) == 0:
        (similarity_predicate, similarity_description) = (
                get_predicate_for_possible_test_arg(test_arg))
        logging.error('No test found, searching for possible tests with %s',
                      similarity_description)
        possible_tests = suite.Suite.find_possible_tests(fs_getter,
                                                         similarity_predicate)
        raise ValueError('Found no tests. Check your suite name, test name, '
                         'or test matching wildcard.\nDid you mean any of '
                         'following tests?\n  %s' % '\n  '.join(possible_tests))

    if not ignore_deps:
        # Log tests whose dependencies can't be satisfied.
        labels = [label.name for label in
                  afe.get_labels(host__hostname=remote)]
        for test in my_suite.tests:
            if test.experimental and no_experimental:
                continue
            unsatisfiable_deps = set(test.dependencies).difference(labels)
            if unsatisfiable_deps:
                logging.warning('%s will be skipped, unsatisfiable '
                             'test dependencies: %s', test.name,
                             unsatisfiable_deps)
    return my_suite


def _run_autoserv(command, pretend=False):
    """Run autoserv command.

    Run the autoserv command and wait on it. Log the stdout.
    Ensure that SIGINT signals are passed along to autoserv.

    @param command: the autoserv command to run.
    @returns: exit code of the command.

    """
    if not pretend:
        logging.debug('Running autoserv command: %s', command)
        global _autoserv_proc
        _autoserv_proc = subprocess.Popen(command,
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.STDOUT)
        # This incantation forces unbuffered reading from stdout,
        # so that autoserv output can be displayed to the user
        # immediately.
        for message in iter(_autoserv_proc.stdout.readline, b''):
            logging.info('autoserv| %s', message.strip())

        _autoserv_proc.wait()
        returncode = _autoserv_proc.returncode
        _autoserv_proc = None
    else:
        logging.info('Pretend mode. Would run autoserv command: %s',
                     command)
        returncode = 0
    return returncode


def run_provisioning_job(provision_label, host, autotest_path,
                         results_directory, fast_mode,
                         ssh_verbosity=0, ssh_options=None,
                         pretend=False, autoserv_verbose=False):
    """Shell out to autoserv to run provisioning job.

    @param provision_label: Label to provision the machine to.
    @param host: Hostname of DUT.
    @param autotest_path: Absolute path of autotest directory.
    @param results_directory: Absolute path of directory to store results in.
                              (results will be stored in subdirectory of this).
    @param fast_mode: bool to use fast mode (disables slow autotest features).
    @param ssh_verbosity: SSH verbosity level, passed along to autoserv_utils
    @param ssh_options: Additional ssh options to be passed to autoserv_utils
    @param pretend: If True, will print out autoserv commands rather than
                    running them.
    @param autoserv_verbose: If true, pass the --verbose flag to autoserv.

    @returns: Absolute path of directory where results were stored.

    """
    # TODO(fdeng): When running against a local DUT, autoserv
    # is still hitting the AFE in the lab.
    # provision_AutoUpdate checks the current build of DUT by
    # retrieving build info from AFE. crosbug.com/295178
    results_directory = os.path.join(results_directory, 'results-provision')
    command = autoserv_utils.autoserv_run_job_command(
            os.path.join(autotest_path, 'server'),
            machines=host, job=None, verbose=autoserv_verbose,
            results_directory=results_directory,
            fast_mode=fast_mode, ssh_verbosity=ssh_verbosity,
            ssh_options=ssh_options,
            extra_args=['--provision', '--job-labels', provision_label],
            no_console_prefix=True)
    if _run_autoserv(command, pretend) != 0:
        raise TestThatProvisioningError('Command returns non-zero code: %s ' %
                                        command)
    return results_directory


def run_job(job, host, autotest_path, results_directory, fast_mode,
            id_digits=1, ssh_verbosity=0, ssh_options=None,
            args=None, pretend=False,
            autoserv_verbose=False, host_attributes={}):
    """
    Shell out to autoserv to run an individual test job.

    @param job: A Job object containing the control file contents and other
                relevent metadata for this test.
    @param host: Hostname of DUT to run test against.
    @param autotest_path: Absolute path of autotest directory.
    @param results_directory: Absolute path of directory to store results in.
                              (results will be stored in subdirectory of this).
    @param fast_mode: bool to use fast mode (disables slow autotest features).
    @param id_digits: The minimum number of digits that job ids should be
                      0-padded to when formatting as a string for results
                      directory.
    @param ssh_verbosity: SSH verbosity level, passed along to autoserv_utils
    @param ssh_options: Additional ssh options to be passed to autoserv_utils
    @param args: String that should be passed as args parameter to autoserv,
                 and then ultimitely to test itself.
    @param pretend: If True, will print out autoserv commands rather than
                    running them.
    @param autoserv_verbose: If true, pass the --verbose flag to autoserv.
    @param host_attributes: Dict of host attributes to pass into autoserv.

    @returns: a tuple, return code of the job and absolute path of directory
              where results were stored.
    """
    with tempfile.NamedTemporaryFile() as temp_file:
        temp_file.write(job.control_file)
        temp_file.flush()
        name_tail = job.name.split('/')[-1]
        results_directory = os.path.join(results_directory,
                                         'results-%0*d-%s' % (id_digits, job.id,
                                                              name_tail))
        # Drop experimental keyval in the keval file in the job result folder.
        os.makedirs(results_directory)
        utils.write_keyval(results_directory,
                           {constants.JOB_EXPERIMENTAL_KEY: job.keyvals[
                                   constants.JOB_EXPERIMENTAL_KEY]})
        extra_args = [temp_file.name]
        if args:
            extra_args.extend(['--args', args])

        command = autoserv_utils.autoserv_run_job_command(
                os.path.join(autotest_path, 'server'),
                machines=host, job=job, verbose=autoserv_verbose,
                results_directory=results_directory,
                fast_mode=fast_mode, ssh_verbosity=ssh_verbosity,
                ssh_options=ssh_options,
                extra_args=extra_args,
                no_console_prefix=True,
                use_packaging=False,
                host_attributes=host_attributes)

        code = _run_autoserv(command, pretend)
        return code, results_directory


def setup_local_afe():
    """
    Setup a local afe database and return a direct_afe object to access it.

    @returns: A autotest_lib.frontend.afe.direct_afe instance.
    """
    # This import statement is delayed until now rather than running at
    # module load time, because it kicks off a local sqlite :memory: backed
    # database, and we don't need that unless we are doing a local run.
    from autotest_lib.frontend import setup_django_lite_environment
    from autotest_lib.frontend.afe import direct_afe
    return direct_afe.directAFE()


def get_predicate_for_test_arg(test):
    """
    Gets a suite predicte function for a given command-line argument.

    @param test: String. An individual TEST command line argument, e.g.
                         'login_CryptohomeMounted' or 'suite:smoke'
    @returns: A (predicate, string) tuple with the necessary suite
              predicate, and a description string of the suite that
              this predicate will produce.
    """
    suitematch = re.match(_SUITE_REGEX, test)
    name_pattern_match = re.match(r'e:(.*)', test)
    file_pattern_match = re.match(r'f:(.*)', test)
    if suitematch:
        suitename = suitematch.group(1)
        return (suite.Suite.name_in_tag_predicate(suitename),
                'suite named %s' % suitename)
    if name_pattern_match:
        pattern = '^%s$' % name_pattern_match.group(1)
        return (suite.Suite.test_name_matches_pattern_predicate(pattern),
                'suite to match name pattern %s' % pattern)
    if file_pattern_match:
        pattern = '^%s$' % file_pattern_match.group(1)
        return (suite.Suite.test_file_matches_pattern_predicate(pattern),
                'suite to match file name pattern %s' % pattern)
    return (suite.Suite.test_name_equals_predicate(test),
            'job named %s' % test)


def get_predicate_for_possible_test_arg(test):
    """
    Gets a suite predicte function to calculate the similarity of given test
    and possible tests.

    @param test: String. An individual TEST command line argument, e.g.
                         'login_CryptohomeMounted' or 'suite:smoke'
    @returns: A (predicate, string) tuple with the necessary suite
              predicate, and a description string of the suite that
              this predicate will produce.
    """
    suitematch = re.match(_SUITE_REGEX, test)
    name_pattern_match = re.match(r'e:(.*)', test)
    file_pattern_match = re.match(r'f:(.*)', test)
    if suitematch:
        suitename = suitematch.group(1)
        return (suite.Suite.name_in_tag_similarity_predicate(suitename),
                'suite name similar to %s' % suitename)
    if name_pattern_match:
        pattern = '^%s$' % name_pattern_match.group(1)
        return (suite.Suite.test_name_similarity_predicate(pattern),
                'job name similar to %s' % pattern)
    if file_pattern_match:
        pattern = '^%s$' % file_pattern_match.group(1)
        return (suite.Suite.test_file_similarity_predicate(pattern),
                'suite to match file name similar to %s' % pattern)
    return (suite.Suite.test_name_similarity_predicate(test),
            'job name similar to %s' % test)


def add_ssh_identity(temp_directory, ssh_private_key=TEST_KEY_PATH):
    """Add an ssh identity to the agent.

    TODO (sbasi) b/26186193: Add support for test_droid and make TEST_KEY_PATH
    not Chrome OS specific.

    @param temp_directory: A directory to copy the |private key| into.
    @param ssh_private_key: Path to the ssh private key to use for testing.
    """
    # Add the testing key to the current ssh agent.
    if os.environ.has_key('SSH_AGENT_PID'):
        # Copy the testing key to the temp directory and make it NOT
        # world-readable. Otherwise, ssh-add complains.
        shutil.copy(ssh_private_key, temp_directory)
        key_copy_path = os.path.join(temp_directory,
                                     os.path.basename(ssh_private_key))
        os.chmod(key_copy_path, stat.S_IRUSR | stat.S_IWUSR)
        p = subprocess.Popen(['ssh-add', key_copy_path],
                             stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        p_out, _ = p.communicate()
        for line in p_out.splitlines():
            logging.info(line)
    else:
        logging.warning('There appears to be no running ssh-agent. Attempting '
                        'to continue without running ssh-add, but ssh commands '
                        'may fail.')


def _auto_detect_labels(afe, remote):
    """Automatically detect host labels and add them to the host in afe.

    Note that the label of board will not be auto-detected.
    This method assumes the host |remote| has already been added to afe.

    @param afe: A direct_afe object used to interact with local afe database.
    @param remote: The hostname of the remote device.

    """
    cros_host = factory.create_host(remote)
    labels_to_create = [label for label in cros_host.get_labels()
                        if not label.startswith(constants.BOARD_PREFIX)]
    labels_to_add_to_afe_host = []
    for label in labels_to_create:
        new_label = afe.create_label(label)
        labels_to_add_to_afe_host.append(new_label.name)
    hosts = afe.get_hosts(hostname=remote)
    if not hosts:
        raise TestThatRunError('Unexpected error: %s has not '
                               'been added to afe.' % remote)
    afe_host = hosts[0]
    afe_host.add_labels(labels_to_add_to_afe_host)


def perform_local_run(afe, autotest_path, tests, remote, fast_mode,
                      build=NO_BUILD, board=NO_BOARD, args=None,
                      pretend=False, no_experimental=False,
                      ignore_deps=True,
                      results_directory=None, ssh_verbosity=0,
                      ssh_options=None,
                      autoserv_verbose=False,
                      iterations=1,
                      host_attributes={}):
    """Perform local run of tests.

    This method enforces satisfaction of test dependencies for tests that are
    run as a part of a suite.

    @param afe: A direct_afe object used to interact with local afe database.
    @param autotest_path: Absolute path of autotest installed in sysroot or
                          custom autotest path set by --autotest_dir.
    @param tests: List of strings naming tests and suites to run. Suite strings
                  should be formed like "suite:smoke".
    @param remote: Remote hostname.
    @param fast_mode: bool to use fast mode (disables slow autotest features).
    @param build: String specifying build for local run.
    @param board: String specifyinb board for local run.
    @param args: String that should be passed as args parameter to autoserv,
                 and then ultimitely to test itself.
    @param pretend: If True, will print out autoserv commands rather than
                    running them.
    @param no_experimental: Skip experimental tests when scheduling a suite.
    @param ignore_deps: If True, test dependencies will be ignored.
    @param results_directory: Directory to store results in. Defaults to None,
                              in which case results will be stored in a new
                              subdirectory of /tmp
    @param ssh_verbosity: SSH verbosity level, passed through to
                          autoserv_utils.
    @param ssh_options: Additional ssh options to be passed to autoserv_utils
    @param autoserv_verbose: If true, pass the --verbose flag to autoserv.
    @param iterations: int number of times to schedule tests.
    @param host_attributes: Dict of host attributes to pass into autoserv.

    @returns: A list of return codes each job that has run.
    """
    # Create host in afe, add board and build labels.
    cros_version_label = provision.cros_version_to_label(build)
    build_label = afe.create_label(cros_version_label)
    board_label = afe.create_label(constants.BOARD_PREFIX + board)
    new_host = afe.create_host(remote)
    new_host.add_labels([build_label.name, board_label.name])
    if not ignore_deps:
        logging.info('Auto-detecting labels for %s', remote)
        _auto_detect_labels(afe, remote)
    # Provision the host to |build|.
    if build != NO_BUILD:
        logging.info('Provisioning %s...', cros_version_label)
        try:
            run_provisioning_job(cros_version_label, remote, autotest_path,
                                 results_directory, fast_mode,
                                 ssh_verbosity, ssh_options,
                                 pretend, autoserv_verbose)
        except TestThatProvisioningError as e:
            logging.error('Provisioning %s to %s failed, tests are aborted, '
                          'failure reason: %s',
                          remote, cros_version_label, e)
            return

    # Create suites that will be scheduled.
    suites_and_descriptions = []
    for test in tests:
        (predicate, description) = get_predicate_for_test_arg(test)
        logging.info('Fetching suite for %s...', description)
        suite = fetch_local_suite(autotest_path, predicate, afe, test_arg=test,
                                  remote=remote,
                                  build=build, board=board,
                                  results_directory=results_directory,
                                  no_experimental=no_experimental,
                                  ignore_deps=ignore_deps)
        suites_and_descriptions.append((suite, description))

    # Schedule the suites, looping over iterations if necessary.
    for iteration in range(iterations):
        if iteration > 0:
            logging.info('Repeating scheduling for iteration %d:', iteration)

        for suite, description in suites_and_descriptions:
            logging.info('Scheduling suite for %s...', description)
            ntests = suite.schedule(
                    lambda log_entry, log_in_subdir=False: None,
                    add_experimental=not no_experimental)
            logging.info('... scheduled %s job(s).', ntests)

    if not afe.get_jobs():
        logging.info('No jobs scheduled. End of local run.')
        return

    last_job_id = afe.get_jobs()[-1].id
    job_id_digits = len(str(last_job_id))
    codes = []
    for job in afe.get_jobs():
        code, _ = run_job(job, remote, autotest_path, results_directory,
                fast_mode, job_id_digits, ssh_verbosity, ssh_options, args,
                pretend, autoserv_verbose, host_attributes)
        codes.append(code)
    return codes


def sigint_handler(signum, stack_frame):
    #pylint: disable-msg=C0111
    """Handle SIGINT or SIGTERM to a local test_that run.

    This handler sends a SIGINT to the running autoserv process,
    if one is running, giving it up to 5 seconds to clean up and exit. After
    the timeout elapses, autoserv is killed. In either case, after autoserv
    exits then this process exits with status 1.
    """
    # If multiple signals arrive before handler is unset, ignore duplicates
    if not _sigint_handler_lock.acquire(False):
        return
    try:
        # Ignore future signals by unsetting handler.
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        signal.signal(signal.SIGTERM, signal.SIG_IGN)

        logging.warning('Received SIGINT or SIGTERM. Cleaning up and exiting.')
        if _autoserv_proc:
            logging.warning('Sending SIGINT to autoserv process. Waiting up '
                            'to %s seconds for cleanup.',
                            _AUTOSERV_SIGINT_TIMEOUT_SECONDS)
            _autoserv_proc.send_signal(signal.SIGINT)
            timed_out, _ = retry.timeout(_autoserv_proc.wait,
                    timeout_sec=_AUTOSERV_SIGINT_TIMEOUT_SECONDS)
            if timed_out:
                _autoserv_proc.kill()
                logging.warning('Timed out waiting for autoserv to handle '
                                'SIGINT. Killed autoserv.')
    finally:
        _sigint_handler_lock.release() # this is not really necessary?
        sys.exit(1)


def create_results_directory(results_directory=None):
    """Create a results directory.

    If no directory is specified this method will create and return a
    temp directory to hold results. If a directory name is specified this
    method will create a directory at the given path, provided it doesn't
    already exist.

    @param results_directory: The path to the results_directory to create.

    @return results_directory: A path to the results_directory, ready for use.
    """
    if results_directory is None:
        # Create a results_directory as subdir of /tmp
        results_directory = tempfile.mkdtemp(prefix='test_that_results_')
    else:
        # Delete results_directory if it already exists.
        try:
            shutil.rmtree(results_directory)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise

        # Create results_directory if it does not exist
        try:
            os.makedirs(results_directory)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
    return results_directory


def perform_run_from_autotest_root(autotest_path, argv, tests, remote,
                                   build=NO_BUILD, board=NO_BOARD, args=None,
                                   pretend=False, no_experimental=False,
                                   ignore_deps=True,
                                   results_directory=None, ssh_verbosity=0,
                                   ssh_options=None,
                                   iterations=1, fast_mode=False, debug=False,
                                   whitelist_chrome_crashes=False,
                                   host_attributes={}):
    """
    Perform a test_that run, from the |autotest_path|.

    This function is to be called from test_that/test_droid's main() script,
    when tests are executed from the |autotest_path|. It handles all stages
    of a test run that come after the bootstrap into |autotest_path|.

    @param autotest_path: Full absolute path to the autotest root directory.
    @param argv: The arguments list, as passed to main(...)
    @param tests: List of strings naming tests and suites to run. Suite strings
                  should be formed like "suite:smoke".
    @param remote: Remote hostname.
    @param build: String specifying build for local run.
    @param board: String specifyinb board for local run.
    @param args: String that should be passed as args parameter to autoserv,
                 and then ultimitely to test itself.
    @param pretend: If True, will print out autoserv commands rather than
                    running them.
    @param no_experimental: Skip experimental tests when scheduling a suite.
    @param ignore_deps: If True, test dependencies will be ignored.
    @param results_directory: Directory to store results in. Defaults to None,
                              in which case results will be stored in a new
                              subdirectory of /tmp
    @param ssh_verbosity: SSH verbosity level, passed through to
                          autoserv_utils.
    @param ssh_options: Additional ssh options to be passed to autoserv_utils
    @param autoserv_verbose: If true, pass the --verbose flag to autoserv.
    @param iterations: int number of times to schedule tests.
    @param fast_mode: bool to use fast mode (disables slow autotest features).
    @param debug: Logging and autoserv verbosity.
    @param whitelist_chrome_crashes: If True, whitelist chrome crashes.
    @param host_attributes: Dict of host attributes to pass into autoserv.

    @returns: A return code that test_that should exit with.
    """
    if results_directory is None or not os.path.exists(results_directory):
        raise ValueError('Expected valid results directory, got %s' %
                          results_directory)

    logging_manager.configure_logging(
            server_logging_config.ServerLoggingConfig(),
            results_dir=results_directory,
            use_console=True,
            verbose=debug,
            debug_log_name='test_that')
    logging.info('Began logging to %s', results_directory)

    logging.debug('test_that command line was: %s', argv)

    signal.signal(signal.SIGINT, sigint_handler)
    signal.signal(signal.SIGTERM, sigint_handler)

    afe = setup_local_afe()
    codes = perform_local_run(afe, autotest_path, tests, remote, fast_mode,
                      build, board,
                      args=args,
                      pretend=pretend,
                      no_experimental=no_experimental,
                      ignore_deps=ignore_deps,
                      results_directory=results_directory,
                      ssh_verbosity=ssh_verbosity,
                      ssh_options=ssh_options,
                      autoserv_verbose=debug,
                      iterations=iterations,
                      host_attributes=host_attributes)
    if pretend:
        logging.info('Finished pretend run. Exiting.')
        return 0

    test_report_command = [os.path.join(os.path.dirname(__file__),
                                        'generate_test_report')]
    # Experimental test results do not influence the exit code.
    test_report_command.append('--ignore_experimental_tests')
    if whitelist_chrome_crashes:
        test_report_command.append('--whitelist_chrome_crashes')
    test_report_command.append(results_directory)
    final_result = subprocess.call(test_report_command)
    with open(os.path.join(results_directory, 'test_report.log'),
              'w') as report_log:
        subprocess.call(test_report_command, stdout=report_log)
    try:
        os.unlink(_LATEST_RESULTS_DIRECTORY)
    except OSError:
        pass
    link_target = os.path.relpath(results_directory,
                                  os.path.dirname(_LATEST_RESULTS_DIRECTORY))
    if any(codes):
        logging.error('Autoserv encountered unexpected errors '
                      'when executing jobs.')
        final_result = final_result or 1
    os.symlink(link_target, _LATEST_RESULTS_DIRECTORY)
    logging.info('Finished running tests. Results can be found in %s or %s',
                 results_directory, _LATEST_RESULTS_DIRECTORY)
    return final_result
