# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Function tests of lxc module. To be able to run this test, following setup
is required:
  1. lxc is installed.
  2. Autotest code exists in /usr/local/autotest, with site-packages installed.
     (run utils/build_externals.py)
  3. The user runs the test should have sudo access. Run the test with sudo.
Note that the test does not require Autotest database and frontend.
"""


import argparse
import logging
import os
import sys
import tempfile
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.site_utils import lxc


TEST_JOB_ID = 123
# Create a temp directory for functional tests. The directory is not under /tmp
# for Moblab to be able to run the test.
TEMP_DIR = tempfile.mkdtemp(dir=lxc.DEFAULT_CONTAINER_PATH,
                            prefix='container_test_')
RESULT_PATH = os.path.join(TEMP_DIR, 'results', str(TEST_JOB_ID))
# Link to download a test package of autotest server package.
# Ideally the test should stage a build on devserver and download the
# autotest_server_package from devserver. This test is focused on testing
# container, so it's prefered to avoid dependency on devserver.
AUTOTEST_SERVER_PKG = ('http://storage.googleapis.com/chromeos-image-archive/'
                       'autotest-containers/autotest_server_package.tar.bz2')

# Test log file to be created in result folder, content is `test`.
TEST_LOG = 'test.log'
# Name of test script file to run in container.
TEST_SCRIPT = 'test.py'
# Test script to run in container to verify autotest code setup.
TEST_SCRIPT_CONTENT = """
import sys

# Test import
import common
import chromite
from autotest_lib.server import utils
from autotest_lib.site_utils import lxc

with open(sys.argv[1], 'w') as f:
    f.write('test')

# Test installing packages
lxc.install_packages(['atop', 'libxslt-dev'], ['selenium', 'numpy'])

"""
# Name of the test control file.
TEST_CONTROL_FILE = 'attach.1'
TEST_DUT = '172.27.213.193'
TEST_RESULT_PATH = lxc.RESULT_DIR_FMT % TEST_JOB_ID
# Test autoserv command.
AUTOSERV_COMMAND = (('/usr/bin/python -u /usr/local/autotest/server/autoserv '
                     '-p -r %(result_path)s/%(test_dut)s -m %(test_dut)s '
                     '-u debug_user -l test -s -P %(job_id)s-debug_user/'
                     '%(test_dut)s -n %(result_path)s/%(test_control_file)s '
                     '--verify_job_repo_url') %
                     {'job_id': TEST_JOB_ID,
                      'result_path': TEST_RESULT_PATH,
                      'test_dut': TEST_DUT,
                      'test_control_file': TEST_CONTROL_FILE})
# Content of the test control file.
TEST_CONTROL_CONTENT = """
def run(machine):
    job.run_test('dummy_PassServer',
                 host=hosts.create_host(machine))

parallel_simple(run, machines)
"""


def setup_logging(log_level=logging.INFO):
    """Direct logging to stdout.

    @param log_level: Level of logging to redirect to stdout, default to INFO.
    """
    logger = logging.getLogger()
    logger.setLevel(log_level)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(log_level)
    formatter = logging.Formatter('%(asctime)s %(message)s')
    handler.setFormatter(formatter)
    logger.handlers = []
    logger.addHandler(handler)


def setup_base(bucket):
    """Test setup base container works.

    @param bucket: ContainerBucket to interact with containers.
    """
    logging.info('Rebuild base container in folder %s.', bucket.container_path)
    bucket.setup_base()
    containers = bucket.get_all()
    logging.info('Containers created: %s', containers.keys())


def setup_test(bucket, name, skip_cleanup):
    """Test container can be created from base container.

    @param bucket: ContainerBucket to interact with containers.
    @param name: Name of the test container.
    @param skip_cleanup: Set to True to skip cleanup, used to troubleshoot
                         container failures.

    @return: A Container object created for the test container.
    """
    logging.info('Create test container.')
    os.makedirs(RESULT_PATH)
    container = bucket.setup_test(name, TEST_JOB_ID, AUTOTEST_SERVER_PKG,
                                  RESULT_PATH, skip_cleanup=skip_cleanup)

    # Inject "AUTOSERV/testing_mode: True" in shadow config to test autoserv.
    container.attach_run('echo $\'[AUTOSERV]\ntesting_mode: True\' >>'
                         ' /usr/local/autotest/shadow_config.ini')
    return container


def test_share(container):
    """Test container can share files with the host.

    @param container: The test container.
    """
    logging.info('Test files written to result directory can be accessed '
                 'from the host running the container..')
    host_test_script = os.path.join(RESULT_PATH, TEST_SCRIPT)
    with open(host_test_script, 'w') as script:
        script.write(TEST_SCRIPT_CONTENT)

    container_result_path = lxc.RESULT_DIR_FMT % TEST_JOB_ID
    container_test_script = os.path.join(container_result_path, TEST_SCRIPT)
    container_test_script_dest = os.path.join('/usr/local/autotest/utils/',
                                              TEST_SCRIPT)
    container_test_log = os.path.join(container_result_path, TEST_LOG)
    host_test_log = os.path.join(RESULT_PATH, TEST_LOG)
    # Move the test script out of result folder as it needs to import common.
    container.attach_run('mv %s %s' % (container_test_script,
                                       container_test_script_dest))
    container.attach_run('python %s %s' % (container_test_script_dest,
                                           container_test_log))
    if not os.path.exists(host_test_log):
        raise Exception('Results created in container can not be accessed from '
                        'the host.')
    with open(host_test_log, 'r') as log:
        if log.read() != 'test':
            raise Exception('Failed to read the content of results in '
                            'container.')


def test_autoserv(container):
    """Test container can run autoserv command.

    @param container: The test container.
    """
    logging.info('Test autoserv command.')
    logging.info('Create test control file.')
    host_control_file = os.path.join(RESULT_PATH, TEST_CONTROL_FILE)
    with open(host_control_file, 'w') as control_file:
        control_file.write(TEST_CONTROL_CONTENT)

    logging.info('Run autoserv command.')
    container.attach_run(AUTOSERV_COMMAND)

    logging.info('Confirm results are available from host.')
    # Read status.log to check the content is not empty.
    container_status_log = os.path.join(TEST_RESULT_PATH, TEST_DUT,
                                        'status.log')
    status_log = container.attach_run(command='cat %s' % container_status_log
                                      ).stdout
    if len(status_log) < 10:
        raise Exception('Failed to read status.log in container.')


def test_package_install(container):
    """Test installing package in container.

    @param container: The test container.
    """
    # Packages are installed in TEST_SCRIPT_CONTENT. Verify the packages in
    # this method.
    container.attach_run('which atop')
    container.attach_run('python -c "import selenium"')


def test_ssh(container, remote):
    """Test container can run ssh to remote server.

    @param container: The test container.
    @param remote: The remote server to ssh to.

    @raise: error.CmdError if container can't ssh to remote server.
    """
    logging.info('Test ssh to %s.', remote)
    container.attach_run('ssh %s -a -x -o StrictHostKeyChecking=no '
                         '-o BatchMode=yes -o UserKnownHostsFile=/dev/null '
                         '-p 22 "true"' % remote)


def parse_options():
    """Parse command line inputs.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--dut', type=str,
                        help='Test device to ssh to.',
                        default=None)
    parser.add_argument('-r', '--devserver', type=str,
                        help='Test devserver to ssh to.',
                        default=None)
    parser.add_argument('-v', '--verbose', action='store_true',
                        default=False,
                        help='Print out ALL entries.')
    parser.add_argument('-s', '--skip_cleanup', action='store_true',
                        default=False,
                        help='Skip deleting test containers.')
    return parser.parse_args()


def main(options):
    """main script.

    @param options: Options to run the script.
    """
    # Force to run the test as superuser.
    # TODO(dshi): crbug.com/459344 Set remove this enforcement when test
    # container can be unprivileged container.
    if utils.sudo_require_password():
        logging.warn('SSP requires root privilege to run commands, please '
                     'grant root access to this process.')
        utils.run('sudo true')

    setup_logging(log_level=(logging.DEBUG if options.verbose
                             else logging.INFO))

    bucket = lxc.ContainerBucket(TEMP_DIR)

    setup_base(bucket)
    container_test_name = (lxc.TEST_CONTAINER_NAME_FMT %
                           (TEST_JOB_ID, time.time(), os.getpid()))
    container = setup_test(bucket, container_test_name, options.skip_cleanup)
    test_share(container)
    test_autoserv(container)
    if options.dut:
        test_ssh(container, options.dut)
    if options.devserver:
        test_ssh(container, options.devserver)
    # Packages are installed in TEST_SCRIPT, verify the packages are installed.
    test_package_install(container)
    logging.info('All tests passed.')


if __name__ == '__main__':
    options = parse_options()
    try:
        main(options)
    finally:
        if not options.skip_cleanup:
            logging.info('Cleaning up temporary directory %s.', TEMP_DIR)
            try:
                lxc.ContainerBucket(TEMP_DIR).destroy_all()
            finally:
                utils.run('sudo rm -rf "%s"' % TEMP_DIR)
