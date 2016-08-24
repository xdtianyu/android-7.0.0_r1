#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys
import tempfile
import time

import common
try:
    # Ensure the chromite site-package is installed.
    from chromite.lib import *
except ImportError:
    import subprocess
    build_externals_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
            'utils', 'build_externals.py')
    subprocess.check_call([build_externals_path, 'chromiterepo'])
    # Restart the script so python now finds the autotest site-packages.
    sys.exit(os.execv(__file__, sys.argv))
from autotest_lib.client.common_lib import control_data
from autotest_lib.server import utils
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.hosts import moblab_host
from autotest_lib.site_utils import brillo_common
from autotest_lib.site_utils import run_suite


_AFE_JOB_PAGE_TEMPLATE = ('http://%(moblab)s/afe/#tab_id=view_job&'
                          'object_id=%(job_id)s')
_AFE_HOST_PAGE_TEMPLATE = ('http://%(moblab)s/afe/#tab_id=view_host&'
                           'object_id=%(host_id)s')
_QUICKMERGE_LIST = ('client/',
                    'global_config.ini',
                    'server/',
                    'site_utils/',
                    'test_suites/',
                    'utils/')


class BrilloTestExecutionError(brillo_common.BrilloTestError):
    """An error while launching and running a test."""


def setup_parser(parser):
    """Add parser options.

    @param parser: argparse.ArgumentParser of the script.
    """
    parser.add_argument('-t', '--test_name',
                        help="Name of the test to run. This is either the "
                             "name in the test's default control file e.g. "
                             "brillo_Gtests or a specific control file's "
                             "filename e.g. control.brillo_GtestsWhitelist.")
    parser.add_argument('-A', '--test_arg', metavar='NAME=VAL',
                        dest='test_args', default=[], action='append',
                        help='An argument to pass to the test.')


def quickmerge(moblab):
    """Transfer over a subset of Autotest directories.

    Quickmerge allows developers to do basic editting of tests and test
    libraries on their workstation without requiring them to emerge and cros
    deploy the autotest-server package.

    @param moblab: MoblabHost representing the MobLab being used to launch the
                   testing.
    """
    autotest_rootdir = os.path.dirname(
            os.path.dirname(os.path.realpath(__file__)))
    # We use rsync -R to copy a bunch of sources in a single run, adding a dot
    # to pinpoint the relative path root.
    rsync_cmd = ['rsync', '-aR', '--exclude', '*.pyc']
    ssh_cmd = 'ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null'
    if int(moblab.port) != 22:
        ssh_cmd += ' -p %s' % moblab.port
    rsync_cmd += ['-e', ssh_cmd]
    rsync_cmd += [os.path.join(autotest_rootdir, '.', path)
                  for path in _QUICKMERGE_LIST]
    rsync_cmd.append('moblab@%s:%s' %
                     (moblab.hostname, moblab_host.AUTOTEST_INSTALL_DIR))
    utils.run(rsync_cmd, timeout=240)


def add_adb_host(moblab, adb_hostname):
    """Add the ADB host to the MobLab's host list.

    @param moblab: MoblabHost representing the MobLab being used to launch the
                   tests.
    @param adb_hostname: Hostname of the ADB Host.

    @returns The adb host to use for launching tests.
    """
    if not adb_hostname:
        adb_hostname = 'localhost'
        moblab.enable_adb_testing()
    if all([host.hostname != adb_hostname for host in moblab.afe.get_hosts()]):
        moblab.add_dut(adb_hostname)
    return adb_hostname


def schedule_test(moblab, host, test, test_args):
    """Schedule a Brillo test.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param host: Hostname of the DUT.
    @param test: Test name.
    @param test_args: Iterable of 'NAME=VAL' (strings) encoding argument
                      assignments for the test.

    @returns autotest_lib.server.frontend.Job object representing the scheduled
             job.
    """
    getter = control_file_getter.FileSystemGetter(
            [os.path.dirname(os.path.dirname(os.path.realpath(__file__)))])
    controlfile_conts = getter.get_control_file_contents_by_name(test)

    # TODO(garnold) This should be removed and arguments injected by feeding
    # args=test_args to create_jobs() directly once crbug.com/545572 is fixed.
    if test_args:
        controlfile_conts = tools.inject_vars({'args': test_args},
                                              controlfile_conts)

    job = moblab.afe.create_job(
            controlfile_conts, name=test,
            control_type=control_data.CONTROL_TYPE_NAMES.SERVER,
            hosts=[host], require_ssp=False)
    logging.info('Tests Scheduled. Please wait for results.')
    job_page = _AFE_JOB_PAGE_TEMPLATE % dict(moblab=moblab.web_address,
                                             job_id=job.id)
    logging.info('Progress can be monitored at %s', job_page)
    logging.info('Please note tests that launch other tests (e.g. sequences) '
                 'might complete quickly, but links to child jobs will appear '
                 'shortly at the bottom on the page (Hit Refresh).')
    return job


def get_all_jobs(moblab, parent_job):
    """Generate a list of the parent_job and it's subjobs.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param host: Hostname of the DUT.
    @param parent_job: autotest_lib.server.frontend.Job object representing the
                       parent job.

    @returns list of autotest_lib.server.frontend.Job objects.
    """
    jobs_list = moblab.afe.get_jobs(id=parent_job.id)
    jobs_list.extend(moblab.afe.get_jobs(parent_job=parent_job.id))
    return jobs_list


def wait_for_test_completion(moblab, host, parent_job):
    """Wait for the parent job and it's subjobs to complete.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param host: Hostname of the DUT.
    @param parent_job: autotest_lib.server.frontend.Job object representing the
                       test job.
    """
    # Wait for the sequence job and it's sub-jobs to finish, while monitoring
    # the DUT state. As long as the DUT does not go into 'Repair Failed' the
    # tests will complete.
    while (moblab.afe.get_jobs(id=parent_job.id, not_yet_run=True,
                               running=True)
           or moblab.afe.get_jobs(parent_job=parent_job.id, not_yet_run=True,
                                  running=True)):
        afe_host = moblab.afe.get_hosts(hostnames=(host,))[0]
        if afe_host.status == 'Repair Failed':
            moblab.afe.abort_jobs(
                [j.id for j in get_all_jobs(moblab, parent_job)])
            host_page = _AFE_HOST_PAGE_TEMPLATE % dict(
                    moblab=moblab.web_address, host_id=afe_host.id)
            raise BrilloTestExecutionError(
                    'ADB dut %s has become Repair Failed. More information '
                    'can be found at %s' % (host, host_page))
        time.sleep(10)


def copy_results(moblab, parent_job):
    """Copy job results locally.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param parent_job: autotest_lib.server.frontend.Job object representing the
                       parent job.

    @returns Temporary directory path.
    """
    tempdir = tempfile.mkdtemp(prefix='brillo_test_results')
    for job in get_all_jobs(moblab, parent_job):
        moblab.get_file('/usr/local/autotest/results/%d-moblab' % job.id,
                        tempdir)
    return tempdir


def output_results(moblab, parent_job):
    """Output the Brillo PTS and it's subjobs results.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param parent_job: autotest_lib.server.frontend.Job object representing the
                       test job.
    """
    solo_test_run = len(moblab.afe.get_jobs(parent_job=parent_job.id)) == 0
    rc = run_suite.ResultCollector(moblab.web_address, moblab.afe, moblab.tko,
                                   None, None, parent_job.name, parent_job.id,
                                   user='moblab', solo_test_run=solo_test_run)
    rc.run()
    rc.output_results()


def main(args):
    """The main function."""
    args = brillo_common.parse_args('Launch a Brillo test using Moblab.',
                                    setup_parser=setup_parser)
    moblab, _ = brillo_common.get_moblab_and_devserver_port(args.moblab_host)

    if args.quickmerge:
        quickmerge(moblab)

    # Add the adb host object to the MobLab.
    adb_host = add_adb_host(moblab, args.adb_host)

    # Schedule the test job.
    test_job = schedule_test(moblab, adb_host, args.test_name, args.test_args)
    wait_for_test_completion(moblab, adb_host, test_job)

    # Gather and report the test results.
    local_results_folder = copy_results(moblab, test_job)
    output_results(moblab, test_job)
    logging.info('Results have also been copied locally to %s',
                 local_results_folder)


if __name__ == '__main__':
    try:
        main(sys.argv)
        sys.exit(0)
    except brillo_common.BrilloTestError as e:
        logging.error('Error: %s', e)

    sys.exit(1)
