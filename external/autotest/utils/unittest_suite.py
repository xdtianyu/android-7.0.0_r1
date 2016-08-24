#!/usr/bin/python -u

import os, sys, unittest, optparse
import common
from autotest_lib.utils import parallel
from autotest_lib.client.common_lib.test_utils import unittest as custom_unittest

parser = optparse.OptionParser()
parser.add_option("-r", action="store", type="string", dest="start",
                  default='',
                  help="root directory to start running unittests")
parser.add_option("--full", action="store_true", dest="full", default=False,
                  help="whether to run the shortened version of the test")
parser.add_option("--debug", action="store_true", dest="debug", default=False,
                  help="run in debug mode")
parser.add_option("--skip-tests", dest="skip_tests",  default=[],
                  help="A space separated list of tests to skip")

parser.set_defaults(module_list=None)

# Following sets are used to define a collection of modules that are optional
# tests and do not need to be executed in unittest suite for various reasons.
# Each entry can be file name or relative path that's relative to the parent
# folder of the folder containing this file (unittest_suite.py). The list
# will be used to filter any test file with matching name or matching full
# path. If a file's name is too general and has a chance to collide with files
# in other folder, it is recommended to specify its relative path here, e.g.,
# using 'mirror/trigger_unittest.py', instead of 'trigger_unittest.py' only.

REQUIRES_DJANGO = set((
        'monitor_db_unittest.py',
        'monitor_db_functional_test.py',
        'monitor_db_cleanup_test.py',
        'frontend_unittest.py',
        'csv_encoder_unittest.py',
        'rpc_interface_unittest.py',
        'models_test.py',
        'scheduler_models_unittest.py',
        'rpc_utils_unittest.py',
        'site_rpc_utils_unittest.py',
        'execution_engine_unittest.py',
        'service_proxy_lib_test.py',
        'rdb_integration_tests.py',
        'rdb_unittest.py',
        'rdb_hosts_unittest.py',
        'rdb_cache_unittests.py',
        'scheduler_lib_unittest.py',
        'host_scheduler_unittests.py',
        'site_parse_unittest.py',
        'shard_client_integration_tests.py',
        'server_manager_unittest.py',
        ))

REQUIRES_MYSQLDB = set((
        'migrate_unittest.py',
        'db_utils_unittest.py',
        ))

REQUIRES_GWT = set((
        'client_compilation_unittest.py',
        ))

REQUIRES_SIMPLEJSON = set((
        'resources_test.py',
        'serviceHandler_unittest.py',
        ))

REQUIRES_AUTH = set ((
    'trigger_unittest.py',
    ))

REQUIRES_HTTPLIB2 = set((
        ))

REQUIRES_PROTOBUFS = set((
        'job_serializer_unittest.py',
        ))

REQUIRES_SELENIUM = set((
        'ap_configurator_factory_unittest.py',
        'ap_batch_locker_unittest.py'
    ))

LONG_RUNTIME = set((
    'base_barrier_unittest.py',
    'logging_manager_test.py',
    'task_loop_unittest.py'  # crbug.com/254030
    ))

# Unitests that only work in chroot. The names are for module name, thus no
# file extension of ".py".
REQUIRES_CHROOT = set((
    'mbim_channel_unittest',
    ))

SKIP = set((
    # This particular KVM autotest test is not a unittest
    'guest_test.py',
    'ap_configurator_test.py',
    'chaos_base_test.py',
    'chaos_interop_test.py',
    'atomic_group_unittests.py',
    # crbug.com/251395
    'dev_server_test.py',
    'full_release_test.py',
    'scheduler_lib_unittest.py',
    'webstore_test.py',
    # crbug.com/432621 These files are not tests, and will disappear soon.
    'des_01_test.py',
    'des_02_test.py',
    # Rquire lxc to be installed
    'lxc_functional_test.py',
    ))

LONG_TESTS = (REQUIRES_MYSQLDB |
              REQUIRES_GWT |
              REQUIRES_HTTPLIB2 |
              REQUIRES_AUTH |
              REQUIRES_PROTOBUFS |
              REQUIRES_SELENIUM |
              LONG_RUNTIME)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

# The set of files in LONG_TESTS with its full path
LONG_TESTS_FULL_PATH = {os.path.join(ROOT, t) for t in LONG_TESTS}

class TestFailure(Exception):
    """Exception type for any test failure."""
    pass


def run_test(mod_names, options):
    """
    @param mod_names: A list of individual parts of the module name to import
            and run as a test suite.
    @param options: optparse options.
    """
    if not options.debug:
        parallel.redirect_io()

    print "Running %s" % '.'.join(mod_names)
    mod = common.setup_modules.import_module(mod_names[-1],
                                             '.'.join(mod_names[:-1]))
    for ut_module in [unittest, custom_unittest]:
        test = ut_module.defaultTestLoader.loadTestsFromModule(mod)
        suite = ut_module.TestSuite(test)
        runner = ut_module.TextTestRunner(verbosity=2)
        result = runner.run(suite)
        if result.errors or result.failures:
            msg = '%s had %d failures and %d errors.'
            msg %= '.'.join(mod_names), len(result.failures), len(result.errors)
            raise TestFailure(msg)


def scan_for_modules(start, options):
    """Scan folders and find all test modules that are not included in the
    blacklist (defined in LONG_TESTS).

    @param start: The absolute directory to look for tests under.
    @param options: optparse options.
    @return a list of modules to be executed.
    """
    modules = []

    skip_tests = SKIP
    if options.skip_tests:
        skip_tests.update(options.skip_tests.split())
    skip_tests_full_path = {os.path.join(ROOT, t) for t in skip_tests}

    for dir_path, sub_dirs, file_names in os.walk(start):
        # Only look in and below subdirectories that are python modules.
        if '__init__.py' not in file_names:
            if options.full:
                for file_name in file_names:
                    if file_name.endswith('.pyc'):
                        os.unlink(os.path.join(dir_path, file_name))
            # Skip all subdirectories below this one, it is not a module.
            del sub_dirs[:]
            if options.debug:
                print 'Skipping', dir_path
            continue  # Skip this directory.

        # Look for unittest files.
        for file_name in file_names:
            if (file_name.endswith('_unittest.py') or
                file_name.endswith('_test.py')):
                file_path = os.path.join(dir_path, file_name)
                if (not options.full and
                    (file_name in LONG_TESTS or
                     file_path in LONG_TESTS_FULL_PATH)):
                    continue
                if (file_name in skip_tests or
                    file_path in skip_tests_full_path):
                    continue
                path_no_py = os.path.join(dir_path, file_name).rstrip('.py')
                assert path_no_py.startswith(ROOT)
                names = path_no_py[len(ROOT)+1:].split('/')
                modules.append(['autotest_lib'] + names)
                if options.debug:
                    print 'testing', path_no_py
    return modules


def is_inside_chroot():
    """Check if the process is running inside the chroot.

    @return: True if the process is running inside the chroot, False otherwise.
    """
    try:
        # chromite may not be setup, e.g., in vm, therefore the ImportError
        # needs to be handled.
        from chromite.lib import cros_build_lib
        return cros_build_lib.IsInsideChroot()
    except ImportError:
        return False


def find_and_run_tests(start, options):
    """
    Find and run Python unittest suites below the given directory.  Only look
    in subdirectories of start that are actual importable Python modules.

    @param start: The absolute directory to look for tests under.
    @param options: optparse options.
    """
    if options.module_list:
        modules = []
        for m in options.module_list:
            modules.append(m.split('.'))
    else:
        modules = scan_for_modules(start, options)

    if options.debug:
        print 'Number of test modules found:', len(modules)

    chroot = is_inside_chroot()
    functions = {}
    for module_names in modules:
        if not chroot and module_names[-1] in REQUIRES_CHROOT:
            if options.debug:
                print ('Test %s requires to run in chroot, skipped.' %
                       module_names[-1])
            continue
        # Create a function that'll test a particular module.  module=module
        # is a hack to force python to evaluate the params now.  We then
        # rename the function to make error reporting nicer.
        run_module = lambda module=module_names: run_test(module, options)
        name = '.'.join(module_names)
        run_module.__name__ = name
        functions[run_module] = set()

    try:
        dargs = {}
        if options.debug:
            dargs['max_simultaneous_procs'] = 1
        pe = parallel.ParallelExecute(functions, **dargs)
        pe.run_until_completion()
    except parallel.ParallelError, err:
        return err.errors
    return []


def main():
    """Entry point for unittest_suite.py"""
    options, args = parser.parse_args()
    if args:
        options.module_list = args

    # Strip the arguments off the command line, so that the unit tests do not
    # see them.
    del sys.argv[1:]

    absolute_start = os.path.join(ROOT, options.start)
    errors = find_and_run_tests(absolute_start, options)
    if errors:
        print "%d tests resulted in an error/failure:" % len(errors)
        for error in errors:
            print "\t%s" % error
        print "Rerun", sys.argv[0], "--debug to see the failure details."
        sys.exit(1)
    else:
        print "All passed!"
        sys.exit(0)


if __name__ == "__main__":
    main()
