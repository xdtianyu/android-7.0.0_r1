# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file lets us test the repair supporting code.
# We could not easily unit test it if it was in the repair file as it makes
# a function call that is not protected by a __name__ == ??? guard.

import datetime, getpass, logging, operator, smtplib, urllib2, xmlrpclib

import common

from autotest_lib.client.common_lib import global_config, mail, logging_config
from autotest_lib.server import frontend
from autotest_lib.server.cros.dynamic_suite import reporting


# Receiver and sender information, if we need to send an email
_NOTIFY_ADDRESS = global_config.global_config.get_config_value(
    'SCHEDULER', 'notify_email_errors', default='')
_SENDER_ADDRESS = global_config.global_config.get_config_value(
    'SCHEDULER', "notify_email_from", default=getpass.getuser())

# Ignore any jobs that were ran more than this many mins past the max job
# timeout.
_CUTOFF_AFTER_TIMEOUT_MINS = 60
_DEFAULT_TEST_TIMEOUT_MINS = global_config.global_config.get_config_value(
    'AUTOTEST_WEB', 'job_max_runtime_mins_default', type=int,
    default=0)


class MachineDeathLogger(logging_config.LoggingConfig):
    """
    Used to log information about a machine going into the Repair Failed state.

    We use this so that if the default log location ever changes it will also
    change for this logger and to keep this information separate from the
    other logs.

    """
    file_formatter = logging.Formatter(fmt='%(asctime)s | %(message)s',
                                       datefmt='%m/%d %H:%M:%S')
    LOGFILE_NAME = 'machine_death.log'

    def __init__(self):
        super(MachineDeathLogger, self).__init__(False)
        self.logger = logging.getLogger('machine_death')

        super(MachineDeathLogger, self).configure_logging(use_console=False)
        log_dir = self.get_server_log_dir()
        self.add_file_handler(self.LOGFILE_NAME, logging.ERROR,
                              log_dir=log_dir)


def _find_problem_test(machine, rpc):
    """
    Find the last job that ran on the machine.

    Go as far back as _DEFAULT_TEST_TIMEOUT_MINS + _CUTOFF_AFTER_TIMEOUT_MINS.
    If global_config doesn't have a job_max_runtime_mins_default we will search
    only as far as _CUTOFF_AFTER_TIMEOUT_MINS.

    @param machine: The hostname (e.g. IP address) of the machine to find the
        last ran job on it.

    @param rpc: The rpc object to contact the server with.

    @return the job status dictionary for the job that last ran on the machine
        or None if there is no such job.
    """

    # Going through the RPC interface means we cannot use the latest() django
    # QuerySet function. So we will instead look at the past
    # job_max_runtime_mins_default plus _CUTOFF_AFTER_TIMEOUT_MINS
    # and pick the most recent run from there.
    cutoff = (datetime.datetime.today() -
              datetime.timedelta(minutes=_DEFAULT_TEST_TIMEOUT_MINS) -
              datetime.timedelta(minutes=_CUTOFF_AFTER_TIMEOUT_MINS))

    results = rpc.run('get_host_queue_entries', host__hostname=machine,
                      started_on__gte=str(cutoff))

    if results:
        return max(results, key=operator.itemgetter('started_on'))
    else:
        return None


def flag_problem_test(machine):
    """
    Notify people about the last job that ran on a machine.

    This method is invoked everytime a machine fails to repair, and attempts
    to identify the last test that ran on the machine. If successfull, it files
    a bug, or sends out an email, or just logs the fact.

    @param machine: The hostname (e.g. IP address) of the machine to find the
        last job ran on it.

    """
    rpc = frontend.AFE()
    logger = MachineDeathLogger()

    try:
        problem_test = _find_problem_test(machine, rpc)
    except (urllib2.URLError, xmlrpclib.ProtocolError):
        logger.logger.error('%s | ERROR: Could not contact RPC server'
                            % machine)
        return

    if problem_test:
        job_id = problem_test['job']['id']
        job_name = problem_test['job']['name']
        bug = reporting.MachineKillerBug(job_id=job_id,
                                         job_name=job_name,
                                         machine=machine)
        reporter = reporting.Reporter()
        bug_id = reporter.report(bug)[0]

        if bug_id is None:
            try:
                email_prefix = ('The following test is killing a machine, '
                                'could not file a bug to report this:\n\n')
                mail.send(_SENDER_ADDRESS, _NOTIFY_ADDRESS, '',
                          bug.title(), email_prefix + bug.summary())
            except smtplib.SMTPDataError:
                logger.logger.error('%s | %d | %s'
                                    % (machine, job_id, job_name))
