# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, logging, os, time


from autotest_lib.client.common_lib import base_job, global_config, log
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.host_queue_entry_states \
    import IntStatus as HqeIntStatus

DEFAULT_POLL_INTERVAL_SECONDS = 10

HQE_MAXIMUM_ABORT_RATE_FLOAT = global_config.global_config.get_config_value(
            'SCHEDULER', 'hqe_maximum_abort_rate_float', type=float,
            default=0.5)


def view_is_relevant(view):
    """
    Indicates whether the view of a given test is meaningful or not.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this is a test result worth looking at further.
    """
    return not view['test_name'].startswith('CLIENT_JOB')


def view_is_for_suite_prep(view):
    """
    Indicates whether the given test view is the view of Suite prep.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this is view of suite preparation.
    """
    return view['test_name'] == 'SERVER_JOB'


def view_is_for_infrastructure_fail(view):
    """
    Indicates whether the given test view is from an infra fail.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this view indicates an infrastructure-side issue during
                 a test.
    """
    return view['test_name'].endswith('SERVER_JOB')


def is_for_infrastructure_fail(status):
    """
    Indicates whether the given Status is from an infra fail.

    @param status: the Status object to look at.
    @return True if this Status indicates an infrastructure-side issue during
                 a test.
    """
    return view_is_for_infrastructure_fail({'test_name': status.test_name})


def gather_job_hostnames(afe, job):
    """
    Collate and return names of hosts used in |job|.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param job: the job to poll on.
    @return iterable of hostnames on which |job| was run, using None as
            placeholders.
    """
    hosts = []
    for e in afe.run('get_host_queue_entries', job=job.id):
        # If the host queue entry has not yet made it into or past the running
        # stage, we should skip it for now.
        if (HqeIntStatus.get_value(e['status']) <
            HqeIntStatus.get_value(HqeIntStatus.RUNNING)):
            hosts.append(None)
        elif not e['host']:
            logging.warning('Job %s (%s) has an entry with no host!',
                         job.name, job.id)
            hosts.append(None)
        else:
            hosts.append(e['host']['hostname'])
    return hosts


def check_job_abort_status(afe, jobs):
    """
    Checks the abort status of all the jobs in jobs and if any have too many
    aborted HostQueueEntries, return True.

    In the case that any job in jobs has too many aborted host queue entries,
    it will raise an exception.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param jobs: an iterable of Running frontend.Jobs

    @returns True if a job in job has too many host queue entries aborted.
             False otherwise.
    """
    for job in jobs:
        entries = afe.run('get_host_queue_entries', job=job.id)
        num_aborted = 0
        for hqe in entries:
            if hqe['aborted']:
                num_aborted = num_aborted + 1
        if num_aborted > len(entries) * HQE_MAXIMUM_ABORT_RATE_FLOAT:
            # This job was not successful, returning True.
            logging.error('Too many host queue entries were aborted for job: '
                          '%s.', job.id)
            return True
    return False


def _abort_jobs_if_timedout(afe, jobs, start_time, timeout_mins):
    """
    Abort all of the jobs in jobs if the running time has past the timeout.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param jobs: an iterable of Running frontend.Jobs
    @param start_time: Time to compare to the current time to see if a timeout
                       has occurred.
    @param timeout_mins: Time in minutes to wait before aborting the jobs we
                         are waiting on.

    @returns True if we there was a timeout, False if not.
    """
    if datetime.datetime.utcnow() < (start_time +
                                     datetime.timedelta(minutes=timeout_mins)):
        return False
    for job in jobs:
        logging.debug('Job: %s has timed out after %s minutes. Aborting job.',
                      job.id, timeout_mins)
        afe.run('abort_host_queue_entries', job=job.id)
    return True


def wait_for_jobs_to_start(afe, jobs, interval=DEFAULT_POLL_INTERVAL_SECONDS,
                           start_time=None, wait_timeout_mins=None):
    """
    Wait for the job specified by |job.id| to start.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param jobs: the jobs to poll on.
    @param interval: polling interval in seconds.
    @param start_time: Time to compare to the current time to see if a timeout
                       has occurred.
    @param wait_timeout_mins: Time in minutes to wait before aborting the jobs
                               we are waiting on.

    @returns True if the jobs have started, False if they get aborted.
    """
    if not start_time:
        start_time = datetime.datetime.utcnow()
    job_ids = [j.id for j in jobs]
    while job_ids:
        if wait_timeout_mins and _abort_jobs_if_timedout(afe, jobs, start_time,
                    wait_timeout_mins):
            # The timeout parameter is not None and we have indeed timed out.
            return False
        for job_id in list(job_ids):
            if len(afe.get_jobs(id=job_id, not_yet_run=True)) > 0:
                continue
            job_ids.remove(job_id)
            logging.debug('Re-imaging job %d running.', job_id)
        if job_ids:
            logging.debug('Waiting %ds before polling again.', interval)
            time.sleep(interval)
    return True


def wait_for_jobs_to_finish(afe, jobs, interval=DEFAULT_POLL_INTERVAL_SECONDS,
                            start_time=None, wait_timeout_mins=None):
    """
    Wait for the jobs specified by each |job.id| to finish.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param interval: polling interval in seconds.
    @param jobs: the jobs to poll on.
    @param start_time: Time to compare to the current time to see if a timeout
                       has occurred. Defaults to now.
    @param wait_timeout_mins: Time in minutes to wait before aborting the jobs
                               we are waiting on. Defaults to no timeout.

    @returns True if the jobs have finished, False if they get aborted.
    """
    if not start_time:
        start_time = datetime.datetime.utcnow()
    job_ids = [j.id for j in jobs]
    while job_ids:
        if wait_timeout_mins and _abort_jobs_if_timedout(afe, jobs, start_time,
                    wait_timeout_mins):
            # The timeout parameter is not None and we have indeed timed out.
            return False
        for job_id in list(job_ids):
            if not afe.get_jobs(id=job_id, finished=True):
                continue
            job_ids.remove(job_id)
            logging.debug('Re-imaging job %d finished.', job_id)
        if job_ids:
            logging.debug('Waiting %ds before polling again.', interval)
            time.sleep(interval)
    return True


def wait_for_and_lock_job_hosts(afe, jobs, manager,
                                interval=DEFAULT_POLL_INTERVAL_SECONDS,
                                start_time=None, wait_timeout_mins=None):
    """
    Poll until devices have begun reimaging, locking them as we go.

    Gather the hosts chosen for |job| -- which must be in the Running
    state itself -- and as they each individually come online and begin
    Running, lock them.  Poll until all chosen hosts have gone to Running
    and been locked using |manager|.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param jobs: an iterable of Running frontend.Jobs
    @param manager: a HostLockManager instance.  Hosts will be added to it
                    as they start Running, and it will be used to lock them.
    @param start_time: Time to compare to the current time to see if a timeout
                       has occurred.
    @param interval: polling interval.
    @param wait_timeout_mins: Time in minutes to wait before aborting the jobs
                              we are waiting on.

    @return iterable of the hosts that were locked or None if all the jobs in
            jobs have been aborted.
    """
    def get_all_hosts(my_jobs):
        """
        Returns a list of all hosts for jobs in my_jobs.

        @param my_jobs: a list of all the jobs we need hostnames for.
        @return: a list of hostnames that correspond to my_jobs.
        """
        all_hosts = []
        for job in my_jobs:
            all_hosts.extend(gather_job_hostnames(afe, job))
        return all_hosts

    if not start_time:
        start_time = datetime.datetime.utcnow()
    locked_hosts = set()
    expected_hosts = set(get_all_hosts(jobs))
    logging.debug('Initial expected hosts: %r', expected_hosts)

    while locked_hosts != expected_hosts:
        if wait_timeout_mins and _abort_jobs_if_timedout(afe, jobs, start_time,
                                                         wait_timeout_mins):
            # The timeout parameter is not None and we have timed out.
            return locked_hosts
        hosts_to_check = [e for e in expected_hosts if e]
        if hosts_to_check:
            logging.debug('Checking to see if %r are Running.', hosts_to_check)
            running_hosts = afe.get_hosts(hosts_to_check, status='Running')
            hostnames = [h.hostname for h in running_hosts]
            if set(hostnames) - locked_hosts != set():
                # New hosts to lock!
                logging.debug('Locking %r.', hostnames)
                manager.lock(hostnames)
            locked_hosts = locked_hosts.union(hostnames)
        time.sleep(interval)
        # 'None' in expected_hosts means we had entries in the job with no
        # host yet assigned, or which weren't Running yet.  We need to forget
        # that across loops, though, and remember only hosts we really used.
        expected_hosts = expected_hosts.difference([None])

        # get_all_hosts() returns only hosts that are currently Running a
        # job we care about.  By unioning with other hosts that we already
        # saw, we get the set of all the hosts that have run a job we care
        # about.
        expected_hosts = expected_hosts.union(get_all_hosts(jobs))
        logging.debug('Locked hosts: %r', locked_hosts)
        logging.debug('Expected hosts: %r', expected_hosts)


    return locked_hosts


def _collate_aborted(current_value, entry):
    """
    reduce() over a list of HostQueueEntries for a job; True if any aborted.

    Functor that can be reduced()ed over a list of
    HostQueueEntries for a job.  If any were aborted
    (|entry.aborted| exists and is True), then the reduce() will
    return True.

    Ex:
      entries = AFE.run('get_host_queue_entries', job=job.id)
      reduce(_collate_aborted, entries, False)

    @param current_value: the current accumulator (a boolean).
    @param entry: the current entry under consideration.
    @return the value of |entry.aborted| if it exists, False if not.
    """
    return current_value or ('aborted' in entry and entry['aborted'])


def _status_for_test(status):
    """
    Indicates whether the status of a given test is meaningful or not.

    @param status: frontend.TestStatus object to look at.
    @return True if this is a test result worth looking at further.
    """
    return not (status.test_name.startswith('SERVER_JOB') or
                status.test_name.startswith('CLIENT_JOB'))


def _yield_job_results(afe, tko, job):
    """
    Yields the results of an individual job.

    Yields one Status object per test.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param job: Job object to get results from, as defined in
                server/frontend.py
    @yields an iterator of Statuses, one per test.
    """
    entries = afe.run('get_host_queue_entries', job=job.id)

    # This query uses the job id to search through the tko_test_view_2
    # table, for results of a test with a similar job_tag. The job_tag
    # is used to store results, and takes the form job_id-owner/host.
    # Many times when a job aborts during a test, the job_tag actually
    # exists and the results directory contains valid logs. If the job
    # was aborted prematurely i.e before it had a chance to create the
    # job_tag, this query will return no results. When statuses is not
    # empty it will contain frontend.TestStatus' with fields populated
    # using the results of the db query.
    statuses = tko.get_job_test_statuses_from_db(job.id)
    if not statuses:
        yield Status('ABORT', job.name)

    # We only care about the SERVER and CLIENT job failures when there
    # are no test failures.
    contains_test_failure = any(_status_for_test(s) and s.status != 'GOOD'
                                for s in statuses)
    for s in statuses:
        # TKO parser uniquelly identifies a test run by
        # (test_name, subdir). In dynamic suite, we need to emit
        # a subdir for each status and make sure (test_name, subdir)
        # in the suite job's status log is unique.
        # For non-test status (i.e.SERVER_JOB, CLIENT_JOB),
        # we use 'job_tag' from tko_test_view_2, which looks like
        # '1246-owner/172.22.33.44'
        # For normal test status, we use 'job_tag/subdir'
        # which looks like '1246-owner/172.22.33.44/my_DummyTest.tag.subdir_tag'
        if _status_for_test(s):
            yield Status(s.status, s.test_name, s.reason,
                         s.test_started_time, s.test_finished_time,
                         job.id, job.owner, s.hostname, job.name,
                         subdir=os.path.join(s.job_tag, s.subdir))
        else:
            if s.status != 'GOOD' and not contains_test_failure:
                yield Status(s.status,
                             '%s_%s' % (entries[0]['job']['name'],
                                        s.test_name),
                             s.reason, s.test_started_time,
                             s.test_finished_time, job.id,
                             job.owner, s.hostname, job.name,
                             subdir=s.job_tag)


def wait_for_child_results(afe, tko, parent_job_id):
    """
    Wait for results of all tests in jobs with given parent id.

    New jobs could be added by calling send(new_jobs) on the generator.
    Currently polls for results every 5s.  Yields one Status object per test
    as results become available.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param parent_job_id: Parent job id for the jobs to wait on.
    @yields an iterator of Statuses, one per test.
    """
    remaining_child_jobs = set(job.id for job in
                               afe.get_jobs(parent_job_id=parent_job_id))
    while remaining_child_jobs:
        new_finished_jobs = [job for job in
                             afe.get_jobs(parent_job_id=parent_job_id,
                                          finished=True)
                             if job.id in remaining_child_jobs]

        for job in new_finished_jobs:

            remaining_child_jobs.remove(job.id)
            for result in _yield_job_results(afe, tko, job):
                # To figure out what new jobs (like retry jobs) have been
                # created since last iteration, we could re-poll for
                # the set of child jobs in each iteration and
                # calculate the set difference against the set we got in
                # last iteration. As an alternative, we could just make
                # the caller 'send' new jobs to this generator. We go
                # with the latter to avoid unnecessary overhead.
                new_child_jobs = (yield result)
                if new_child_jobs:
                    remaining_child_jobs.update([new_job.id for new_job in
                                                 new_child_jobs])
                    # Return nothing if 'send' is called
                    yield None

        time.sleep(5)


def wait_for_results(afe, tko, jobs):
    """
    Wait for results of all tests in all jobs in |jobs|.

    New jobs could be added by calling send(new_jobs) on the generator.
    Currently polls for results every 5s.  Yields one Status object per test
    as results become available.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param jobs: a list of Job objects, as defined in server/frontend.py.
    @yields an iterator of Statuses, one per test.
    """
    local_jobs = list(jobs)
    while local_jobs:
        for job in list(local_jobs):
            if not afe.get_jobs(id=job.id, finished=True):
                continue

            local_jobs.remove(job)
            for result in _yield_job_results(afe, tko, job):
                # The caller can 'send' new jobs (i.e. retry jobs)
                # to this generator at any time.
                new_jobs = (yield result)
                if new_jobs:
                    local_jobs.extend(new_jobs)
                    # Return nothing if 'send' is called
                    yield None

        time.sleep(5)


def gather_per_host_results(afe, tko, jobs, name_prefix=''):
    """
    Gather currently-available results for all |jobs|, aggregated per-host.

    For each job in |jobs|, gather per-host results and summarize into a single
    log entry.  For example, a FAILed SERVER_JOB and successful actual test
    is reported as a FAIL.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param jobs: a list of Job objects, as defined in server/frontend.py.
    @param name_prefix: optional string to prepend to Status object names.
    @return a dict mapping {hostname: Status}, one per host used in a Job.
    """
    to_return = {}
    for job in jobs:
        for s in tko.get_job_test_statuses_from_db(job.id):
            candidate = Status(s.status,
                               name_prefix+s.hostname,
                               s.reason,
                               s.test_started_time,
                               s.test_finished_time)
            if (s.hostname not in to_return or
                candidate.is_worse_than(to_return[s.hostname])):
                to_return[s.hostname] = candidate

        # If we didn't find more specific data above for a host, fill in here.
        # For jobs that didn't even make it to finding a host, just collapse
        # into a single log entry.
        for e in afe.run('get_host_queue_entries', job=job.id):
            host = e['host']['hostname'] if e['host'] else 'hostless' + job.name
            if host not in to_return:
                to_return[host] = Status(Status.STATUS_MAP[e['status']],
                                         job.name,
                                         'Did not run',
                                         begin_time_str=job.created_on)

    return to_return


def check_and_record_reimage_results(per_host_statuses, group, record_entry):
    """
    Record all Statuses in results and return True if at least one was GOOD.

    @param per_host_statuses: dict mapping {hostname: Status}, one per host
                              used in a Job.
    @param group: the HostGroup used for the Job whose results we're reporting.
    @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
    @return True if at least one of the Statuses are good.
    """
    failures = []
    for hostname, status in per_host_statuses.iteritems():
        if status.is_good():
            group.mark_host_success(hostname)
            status.record_all(record_entry)
        else:
            failures.append(status)

    success = group.enough_hosts_succeeded()
    if success:
        for failure in failures:
            logging.warning("%s failed to reimage.", failure.test_name)
            failure.override_status('WARN')
            failure.record_all(record_entry)
    else:
        for failure in failures:
            # No need to log warnings; the job is failing.
            failure.record_all(record_entry)

    return success


class Status(object):
    """
    A class representing a test result.

    Stores all pertinent info about a test result and, given a callable
    to use, can record start, result, and end info appropriately.

    @var _status: status code, e.g. 'INFO', 'FAIL', etc.
    @var _test_name: the name of the test whose result this is.
    @var _reason: message explaining failure, if any.
    @var _begin_timestamp: when test started (int, in seconds since the epoch).
    @var _end_timestamp: when test finished (int, in seconds since the epoch).
    @var _id: the ID of the job that generated this Status.
    @var _owner: the owner of the job that generated this Status.

    @var STATUS_MAP: a dict mapping host queue entry status strings to canonical
                     status codes; e.g. 'Aborted' -> 'ABORT'
    """
    _status = None
    _test_name = None
    _reason = None
    _begin_timestamp = None
    _end_timestamp = None

    # Queued status can occur if the try job just aborted due to not completing
    # reimaging for all machines. The Queued corresponds to an 'ABORT'.
    STATUS_MAP = {'Failed': 'FAIL', 'Aborted': 'ABORT', 'Completed': 'GOOD',
                  'Queued' : 'ABORT'}

    class sle(base_job.status_log_entry):
        """
        Thin wrapper around status_log_entry that supports stringification.
        """
        def __str__(self):
            return self.render()

        def __repr__(self):
            return self.render()


    def __init__(self, status, test_name, reason='', begin_time_str=None,
                 end_time_str=None, job_id=None, owner=None, hostname=None,
                 job_name='', subdir=None):
        """
        Constructor

        @param status: status code, e.g. 'INFO', 'FAIL', etc.
        @param test_name: the name of the test whose result this is.
        @param reason: message explaining failure, if any; Optional.
        @param begin_time_str: when test started (in time_utils.TIME_FMT);
                               now() if None or 'None'.
        @param end_time_str: when test finished (in time_utils.TIME_FMT);
                             now() if None or 'None'.
        @param job_id: the ID of the job that generated this Status.
        @param owner: the owner of the job that generated this Status.
        @param hostname: The name of the host the test that generated this
                         result ran on.
        @param job_name: The job name; Contains the test name with/without the
                         experimental prefix, the tag and the build.
        @param subdir: The result directory of the test. It will be recorded
                       as the subdir in the status.log file.
        """
        self._status = status
        self._test_name = test_name
        self._reason = reason
        self._id = job_id
        self._owner = owner
        self._hostname = hostname
        self._job_name = job_name
        self._subdir = subdir
        # Autoserv drops a keyval of the started time which eventually makes its
        # way here.  Therefore, if we have a starting time, we may assume that
        # the test reached Running and actually began execution on a drone.
        self._test_executed = begin_time_str and begin_time_str != 'None'

        if begin_time_str and begin_time_str != 'None':
            self._begin_timestamp = int(time.mktime(
                datetime.datetime.strptime(
                    begin_time_str, time_utils.TIME_FMT).timetuple()))
        else:
            self._begin_timestamp = int(time.time())

        if end_time_str and end_time_str != 'None':
            self._end_timestamp = int(time.mktime(
                datetime.datetime.strptime(
                    end_time_str, time_utils.TIME_FMT).timetuple()))
        else:
            self._end_timestamp = int(time.time())


    def is_good(self):
        """ Returns true if status is good. """
        return self._status == 'GOOD'


    def is_warn(self):
        """ Returns true if status is warn. """
        return self._status == 'WARN'


    def is_testna(self):
        """ Returns true if status is TEST_NA """
        return self._status == 'TEST_NA'


    def is_worse_than(self, candidate):
        """
        Return whether |self| represents a "worse" failure than |candidate|.

        "Worse" is defined the same as it is for log message purposes in
        common_lib/log.py.  We also consider status with a specific error
        message to represent a "worse" failure than one without.

        @param candidate: a Status instance to compare to this one.
        @return True if |self| is "worse" than |candidate|.
        """
        if self._status != candidate._status:
            return (log.job_statuses.index(self._status) <
                    log.job_statuses.index(candidate._status))
        # else, if the statuses are the same...
        if self._reason and not candidate._reason:
            return True
        return False


    def record_start(self, record_entry):
        """
        Use record_entry to log message about start of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle('START', self._subdir,
                                self._test_name, '',
                                None, self._begin_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_result(self, record_entry):
        """
        Use record_entry to log message about result of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle(self._status, self._subdir,
                                self._test_name, self._reason, None,
                                self._end_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_end(self, record_entry):
        """
        Use record_entry to log message about end of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle('END %s' % self._status, self._subdir,
                               self._test_name, '', None, self._end_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_all(self, record_entry):
        """
        Use record_entry to log all messages about test results.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        self.record_start(record_entry)
        self.record_result(record_entry)
        self.record_end(record_entry)


    def override_status(self, override):
        """
        Override the _status field of this Status.

        @param override: value with which to override _status.
        """
        self._status = override


    @property
    def test_name(self):
        """ Name of the test this status corresponds to. """
        return self._test_name


    @test_name.setter
    def test_name(self, value):
        """
        Test name setter.

        @param value: The test name.
        """
        self._test_name = value


    @property
    def id(self):
        """ Id of the job that corresponds to this status. """
        return self._id


    @property
    def owner(self):
        """ Owner of the job that corresponds to this status. """
        return self._owner


    @property
    def hostname(self):
        """ Host the job corresponding to this status ran on. """
        return self._hostname


    @property
    def reason(self):
        """ Reason the job corresponding to this status failed. """
        return self._reason


    @property
    def test_executed(self):
        """ If the test reached running an autoserv instance or not. """
        return self._test_executed

    @property
    def subdir(self):
        """Subdir of test this status corresponds to."""
        return self._subdir
