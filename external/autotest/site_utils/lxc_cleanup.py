#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Cleanup orphaned containers.

If an autoserv process dies without being able to call handler of SIGTERM, the
container used to run the test will be orphaned. This adds overhead to the
drone. This script is used to clean up such containers.

This module also checks if the test job associated with a container has
finished. If so, kill the autoserv process for the test job and destroy the
container. To avoid racing condition, this only applies to job finished at least
1 hour ago.

"""

import argparse
import datetime
import logging
import os
import re
import signal
import socket

import common
from autotest_lib.client.common_lib import logging_config
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.site_utils import lxc


AFE = frontend_wrappers.RetryingAFE(timeout_min=0.1, delay_sec=10)
# The cutoff time to declare a test job is completed and container is orphaned.
# This is to avoid a race condition that scheduler aborts a job and autoserv
# is still in the process of destroying the container it used.
FINISHED_JOB_CUTOFF_TIME = datetime.datetime.now() - datetime.timedelta(hours=1)

def get_info(container_name):
    """Get job id and autoserv process id from container name.

    @param container: Name of the container.

    @return: job id and autoserv process id for the given container name.

    """
    match = re.match('test_(\d+)_(\d+)_(\d+)', container_name)
    if not match:
        # Container is not created for test, e.g., the base container.
        return None, None
    job_id = int(match.groups()[0])
    pid = match.groups()[2]
    return job_id, pid


def is_container_orphaned(container):
    """Check if a container is orphaned.

    A container is orphaned if any of these condition is True:
    1. The autoserv process created the container is no longer running.
    2. The test job is finished at least 1 hour ago.

    @param container: A Container object.

    @return: True if the container is orphaned.

    """
    logging.debug('Checking if container is orphaned: %s', container.name)
    job_id, pid = get_info(container.name)
    if not job_id:
        logging.debug('Container %s is not created for test.', container.name)
        return False

    if pid and not utils.pid_is_alive(pid):
        logging.debug('Process with PID %s is not alive, container %s is '
                      'orphaned.', pid, container.name)
        return True

    try:
        hqes = AFE.get_host_queue_entries(job_id=job_id)
    except Exception as e:
        logging.error('Failed to get hqe for job %s. Error: %s.', job_id, e)
        return False

    if not hqes:
        # The job has not run yet.
        return False
    for hqe in hqes:
        if hqe.active or not hqe.complete:
            logging.debug('Test job %s is not completed yet, container %s is '
                          'not orphaned.', job_id, container.name)
            return False
        if (hqe.finished_on and
            (time_utils.time_string_to_datetime(hqes.finished_on) >
             FINISHED_JOB_CUTOFF_TIME)):
            logging.debug('Test job %s was completed less than an hour ago.',
                          job_id)
            return False

    logging.debug('Test job %s was completed, container %s is orphaned.',
                  job_id, container.name)
    return True


def cleanup(container, options):
    """Cleanup orphaned container.

    @param container: A Container object to be cleaned up.
    @param options: Options to do cleanup.

    @return: True if cleanup is successful. False otherwise.

    """
    if not options.execute:
        logging.info('dryrun: Cleanup container %s', container.name)
        return False

    try:
        _, pid = get_info(container.name)
        # Kill autoserv process
        if pid and utils.pid_is_alive(pid):
            logging.info('Stopping process %s...', pid)
            utils.nuke_pid(int(pid), (signal.SIGKILL,))

        # Destroy container
        logging.info('Destroying container %s...', container.name)
        container.destroy()
        return True
    except Exception as e:
        logging.error('Failed to cleanup container %s. Error: %s',
                      container.name, e)
        return False


def parse_options():
    """Parse command line inputs.

    @return: Options to run the script.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true',
                        default=False,
                        help='Print out ALL entries.')
    parser.add_argument('-x', '--execute', action='store_true',
                        default=False,
                        help=('Execute the actions to kill autoserv processes '
                              'and destroy containers. Default is False to do '
                              'dry run'))
    # TODO(dshi): Consider to adopt the scheduler log model:
    # 1. Create one log per run.
    # 2. Create a symlink to the latest log.
    parser.add_argument('-l', '--logfile', type=str,
                        default=None,
                        help='Path to the log file to save logs.')
    return parser.parse_args()


def main(options):
    """Main script.

    @param options: Options to run the script.
    """
    config = logging_config.LoggingConfig()
    if options.logfile:
        config.add_file_handler(
                file_path=os.path.abspath(options.logfile),
                level=logging.DEBUG if options.verbose else logging.INFO)

    bucket = lxc.ContainerBucket()
    logging.info('')
    logging.info('Cleaning container bucket %s', bucket.container_path)
    success_count = 0
    failure_count = 0
    for container in bucket.get_all().values():
        if is_container_orphaned(container):
            if cleanup(container, options):
                success_count += 1
            else:
                failure_count += 1
    if options.execute:
        key = 'container_cleanup.%s' % socket.gethostname().replace('.', '_')
        autotest_stats.Gauge(key).send('success', success_count)
        autotest_stats.Gauge(key).send('failure', failure_count)
    logging.info('Cleanup finished.')


if __name__ == '__main__':
    options = parse_options()
    main(options)
