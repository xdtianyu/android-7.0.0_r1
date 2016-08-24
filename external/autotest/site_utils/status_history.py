# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Services relating to DUT status and job history.

The central abstraction of this module is the `HostJobHistory`
class.  This class provides two related pieces of information
regarding a single DUT:
  * A history of tests and special tasks that have run on
    the DUT in a given time range.
  * Whether the DUT was "working" or "broken" at a given
    time.

The "working" or "broken" status of a DUT is determined by
the DUT's special task history.  At the end of any job or
task, the status is indicated as follows:
  * After any successful special task, the DUT is considered
    "working".
  * After any failed Repair task, the DUT is considered "broken".
  * After any other special task or after any regular test job, the
    DUT's status is considered unchanged.

Definitions for terms used in the code below:
  * status task - Any special task that determines the DUT's
    status; that is, any successful task, or any failed Repair.
  * diagnosis interval - A time interval during which DUT status
    changed either from "working" to "broken", or vice versa.  The
    interval starts with the last status task with the old status,
    and ends after the first status task with the new status.

Diagnosis intervals are interesting because they normally contain
the logs explaining a failure or repair event.

"""

import common
from autotest_lib.frontend import setup_django_environment
from django.db import models as django_models

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import time_utils
from autotest_lib.frontend.afe import models as afe_models
from autotest_lib.site_utils.suite_scheduler import constants


# Values used to describe the diagnosis of a DUT.  These values are
# used to indicate both DUT status after a job or task, and also
# diagnosis of whether the DUT was working at the end of a given
# time interval.
#
# UNUSED:  Used when there are no events recorded in a given
#     time interval.
# UNKNOWN:  For an individual event, indicates that the DUT status
#     is unchanged from the previous event.  For a time interval,
#     indicates that the DUT's status can't be determined from the
#     DUT's history.
# WORKING:  Indicates that the DUT was working normally after the
#     event, or at the end of the time interval.
# BROKEN:  Indicates that the DUT needed manual repair after the
#     event, or at the end of the time interval.
#
UNUSED = 0
UNKNOWN = 1
WORKING = 2
BROKEN = 3


def parse_time(time_string):
    """Parse time according to a canonical form.

    The "canonical" form is the form in which date/time
    values are stored in the database.

    @param time_string Time to be parsed.
    """
    return int(time_utils.to_epoch_time(time_string))


class _JobEvent(object):
    """Information about an event in host history.

    This remembers the relevant data from a single event in host
    history.  An event is any change in DUT state caused by a job
    or special task.  The data captured are the start and end times
    of the event, the URL of logs to the job or task causing the
    event, and a diagnosis of whether the DUT was working or failed
    afterwards.

    This class is an adapter around the database model objects
    describing jobs and special tasks.  This is an abstract
    superclass, with concrete subclasses for `HostQueueEntry` and
    `SpecialTask` objects.

    @property start_time  Time the job or task began execution.
    @property end_time    Time the job or task finished execution.
    @property job_url     URL to the logs for the event's job.
    @property diagnosis   Working status of the DUT after the event.

    """

    get_config_value = global_config.global_config.get_config_value
    _LOG_URL_PATTERN = get_config_value('CROS', 'log_url_pattern')

    @classmethod
    def get_log_url(cls, afe_hostname, logdir):
        """Return a URL to job results.

        The URL is constructed from a base URL determined by the
        global config, plus the relative path of the job's log
        directory.

        @param afe_hostname Hostname for autotest frontend
        @param logdir Relative path of the results log directory.

        @return A URL to the requested results log.

        """
        return cls._LOG_URL_PATTERN % (afe_hostname, logdir)


    def __init__(self, start_time, end_time):
        self.start_time = parse_time(start_time)
        self.end_time = parse_time(end_time)


    def __cmp__(self, other):
        """Compare two jobs by their start time.

        This is a standard Python `__cmp__` method to allow sorting
        `_JobEvent` objects by their times.

        @param other The `_JobEvent` object to compare to `self`.

        """
        return self.start_time - other.start_time


    @property
    def job_url(self):
        """Return the URL for this event's job logs."""
        raise NotImplemented()


    @property
    def diagnosis(self):
        """Return the status of the DUT after this event.

        The diagnosis is interpreted as follows:
          UNKNOWN - The DUT status was the same before and after
              the event.
          WORKING - The DUT appeared to be working after the event.
          BROKEN - The DUT likely required manual intervention
              after the event.

        @return A valid diagnosis value.

        """
        raise NotImplemented()


class _SpecialTaskEvent(_JobEvent):
    """`_JobEvent` adapter for special tasks.

    This class wraps the standard `_JobEvent` interface around a row
    in the `afe_special_tasks` table.

    """

    @classmethod
    def get_tasks(cls, afe, host_id, start_time, end_time):
        """Return special tasks for a host in a given time range.

        Return a list of `_SpecialTaskEvent` objects representing all
        special tasks that ran on the given host in the given time
        range.  The list is ordered as it was returned by the query
        (i.e. unordered).

        @param afe         Autotest frontend
        @param host_id     Database host id of the desired host.
        @param start_time  Start time of the range of interest.
        @param end_time    End time of the range of interest.

        @return A list of `_SpecialTaskEvent` objects.

        """
        query_start = time_utils.epoch_time_to_date_string(start_time)
        query_end = time_utils.epoch_time_to_date_string(end_time)
        tasks = afe.get_host_special_tasks(
                host_id,
                time_started__gte=query_start,
                time_finished__lte=query_end,
                is_complete=1)
        return [cls(afe.server, t) for t in tasks]


    @classmethod
    def get_status_task(cls, afe, host_id, end_time):
        """Return the task indicating a host's status at a given time.

        The task returned determines the status of the DUT; the
        diagnosis on the task indicates the diagnosis for the DUT at
        the given `end_time`.

        @param afe         Autotest frontend
        @param host_id     Database host id of the desired host.
        @param end_time    Find status as of this time.

        @return A `_SpecialTaskEvent` object for the requested task,
                or `None` if no task was found.

        """
        query_end = time_utils.epoch_time_to_date_string(end_time)
        task = afe.get_host_status_task(host_id, query_end)
        return cls(afe.server, task) if task else None


    def __init__(self, afe_hostname, afetask):
        self._afe_hostname = afe_hostname
        self._afetask = afetask
        super(_SpecialTaskEvent, self).__init__(
                afetask.time_started, afetask.time_finished)


    @property
    def job_url(self):
        logdir = ('hosts/%s/%s-%s' %
                  (self._afetask.host.hostname, self._afetask.id,
                   self._afetask.task.lower()))
        return _SpecialTaskEvent.get_log_url(self._afe_hostname, logdir)


    @property
    def diagnosis(self):
        if self._afetask.success:
            return WORKING
        elif self._afetask.task == 'Repair':
            return BROKEN
        else:
            return UNKNOWN


class _TestJobEvent(_JobEvent):
    """`_JobEvent` adapter for regular test jobs.

    This class wraps the standard `_JobEvent` interface around a row
    in the `afe_host_queue_entries` table.

    """

    @classmethod
    def get_hqes(cls, afe, host_id, start_time, end_time):
        """Return HQEs for a host in a given time range.

        Return a list of `_TestJobEvent` objects representing all the
        HQEs of all the jobs that ran on the given host in the given
        time range.  The list is ordered as it was returned by the
        query (i.e. unordered).

        @param afe         Autotest frontend
        @param host_id     Database host id of the desired host.
        @param start_time  Start time of the range of interest.
        @param end_time    End time of the range of interest.

        @return A list of `_TestJobEvent` objects.

        """
        query_start = time_utils.epoch_time_to_date_string(start_time)
        query_end = time_utils.epoch_time_to_date_string(end_time)
        hqelist = afe.get_host_queue_entries(
                host_id=host_id,
                start_time=query_start,
                end_time=query_end,
                complete=1)
        return [cls(afe.server, hqe) for hqe in hqelist]


    def __init__(self, afe_hostname, hqe):
        self._afe_hostname = afe_hostname
        self._hqe = hqe
        super(_TestJobEvent, self).__init__(
                hqe.started_on, hqe.finished_on)


    @property
    def job_url(self):
        logdir = '%s-%s' % (self._hqe.job.id, self._hqe.job.owner)
        return _TestJobEvent.get_log_url(self._afe_hostname, logdir)


    @property
    def diagnosis(self):
        return UNKNOWN


class HostJobHistory(object):
    """Class to query and remember DUT execution and status history.

    This class is responsible for querying the database to determine
    the history of a single DUT in a time interval of interest, and
    for remembering the query results for reporting.

    @property hostname    Host name of the DUT.
    @property start_time  Start of the requested time interval.
                          This field may be `None`.
    @property end_time    End of the requested time interval.
    @property _afe        Autotest frontend for queries.
    @property _host       Database host object for the DUT.
    @property _history    A list of jobs and special tasks that
                          ran on the DUT in the requested time
                          interval, ordered in reverse, from latest
                          to earliest.

    @property _status_interval   A list of all the jobs and special
                                 tasks that ran on the DUT in the
                                 last diagnosis interval prior to
                                 `end_time`, ordered from latest to
                                 earliest.
    @property _status_diagnosis  The DUT's status as of `end_time`.
    @property _status_task       The DUT's last status task as of
                                 `end_time`.

    """

    @classmethod
    def get_host_history(cls, afe, hostname, start_time, end_time):
        """Create a `HostJobHistory` instance for a single host.

        Simple factory method to construct host history from a
        hostname.  Simply looks up the host in the AFE database, and
        passes it to the class constructor.

        @param afe         Autotest frontend
        @param hostname    Name of the host.
        @param start_time  Start time for the history's time
                           interval.
        @param end_time    End time for the history's time interval.

        @return A new `HostJobHistory` instance.

        """
        afehost = afe.get_hosts(hostname=hostname)[0]
        return cls(afe, afehost, start_time, end_time)


    @classmethod
    def get_multiple_histories(cls, afe, start_time, end_time,
                               board=None, pool=None):
        """Create `HostJobHistory` instances for a set of hosts.

        The set of hosts can be specified as "all hosts of a given
        board type", "all hosts in a given pool", or "all hosts
        of a given board and pool".

        @param afe         Autotest frontend
        @param start_time  Start time for the history's time
                           interval.
        @param end_time    End time for the history's time interval.
        @param board       All hosts must have this board type; if
                           `None`, all boards are allowed.
        @param pool        All hosts must be in this pool; if
                           `None`, all pools are allowed.

        @return A list of new `HostJobHistory` instances.

        """
        # If `board` or `pool` are both `None`, we could search the
        # entire database, which is more expensive than we want.
        # Our caller currently won't (can't) do this, but assert to
        # be safe.
        assert board is not None or pool is not None
        labels = []
        if board is not None:
            labels.append(constants.Labels.BOARD_PREFIX + board)
        if pool is not None:
            labels.append(constants.Labels.POOL_PREFIX + pool)
        kwargs = {'multiple_labels': labels}
        hosts = afe.get_hosts(**kwargs)
        return [cls(afe, h, start_time, end_time) for h in hosts]


    def __init__(self, afe, afehost, start_time, end_time):
        self._afe = afe
        self.hostname = afehost.hostname
        self.end_time = end_time
        self.start_time = start_time
        self._host = afehost
        # Don't spend time on queries until they're needed.
        self._history = None
        self._status_interval = None
        self._status_diagnosis = None
        self._status_task = None


    def _get_history(self, start_time, end_time):
        """Get the list of events for the given interval."""
        newtasks = _SpecialTaskEvent.get_tasks(
                self._afe, self._host.id, start_time, end_time)
        newhqes = _TestJobEvent.get_hqes(
                self._afe, self._host.id, start_time, end_time)
        newhistory = newtasks + newhqes
        newhistory.sort(reverse=True)
        return newhistory


    def __iter__(self):
        if self._history is None:
            self._history = self._get_history(self.start_time,
                                              self.end_time)
        return self._history.__iter__()


    def _extract_prefixed_label(self, prefix):
        labels = [l for l in self._host.labels
                    if l.startswith(prefix)]
        return labels[0][len(prefix) : ] if labels else None


    @property
    def host(self):
        """Return the AFE host object for this history."""
        return self._host


    @property
    def host_board(self):
        """Return the board name for this history's DUT."""
        prefix = constants.Labels.BOARD_PREFIX
        return self._extract_prefixed_label(prefix)


    @property
    def host_pool(self):
        """Return the pool name for this history's DUT."""
        prefix = constants.Labels.POOL_PREFIX
        return self._extract_prefixed_label(prefix)


    def _init_status_task(self):
        """Fill in `self._status_diagnosis` and `_status_task`."""
        if self._status_diagnosis is not None:
            return
        self._status_task = _SpecialTaskEvent.get_status_task(
                self._afe, self._host.id, self.end_time)
        if self._status_task is not None:
            self._status_diagnosis = self._status_task.diagnosis
        else:
            self._status_diagnosis = UNKNOWN


    def _init_status_interval(self):
        """Fill in `self._status_interval`."""
        if self._status_interval is not None:
            return
        self._init_status_task()
        self._status_interval = []
        if self._status_task is None:
            return
        query_end = time_utils.epoch_time_to_date_string(self.end_time)
        interval = self._afe.get_host_diagnosis_interval(
                self._host.id, query_end,
                self._status_diagnosis != WORKING)
        if not interval:
            return
        self._status_interval = self._get_history(
                parse_time(interval[0]),
                parse_time(interval[1]))


    def diagnosis_interval(self):
        """Find this history's most recent diagnosis interval.

        Returns a list of `_JobEvent` instances corresponding to the
        most recent diagnosis interval occurring before this
        history's end time.

        The list is returned as with `self._history`, ordered from
        most to least recent.

        @return The list of the `_JobEvent`s in the diagnosis
                interval.

        """
        self._init_status_interval()
        return self._status_interval


    def last_diagnosis(self):
        """Return the diagnosis of whether the DUT is working.

        This searches the DUT's job history, looking for the most
        recent status task for the DUT.  Return a tuple of
        `(diagnosis, task)`.

        The `diagnosis` entry in the tuple is one of these values:
          * UNUSED - The host's last status task is older than
              `self.start_time`.
          * WORKING - The DUT is working.
          * BROKEN - The DUT likely requires manual intervention.
          * UNKNOWN - No task could be found indicating status for
              the DUT.

        If the DUT was working at last check, but hasn't been used
        inside this history's time interval, the status `UNUSED` is
        returned with the last status task, instead of `WORKING`.

        The `task` entry in the tuple is the status task that led to
        the diagnosis.  The task will be `None` if the diagnosis is
        `UNKNOWN`.

        @return A tuple with the DUT's diagnosis and the task that
                determined it.

        """
        self._init_status_task()
        diagnosis = self._status_diagnosis
        if (self.start_time is not None and
                self._status_task is not None and
                self._status_task.end_time < self.start_time and
                diagnosis == WORKING):
            diagnosis = UNUSED
        return diagnosis, self._status_task


def get_diagnosis_interval(host_id, end_time, success):
    """Return the last diagnosis interval for a given host and time.

    This routine queries the database for the special tasks on a
    given host before a given time.  From those tasks it selects the
    last status task before a change in status, and the first status
    task after the change.  When `success` is true, the change must
    be from "working" to "broken".  When false, the search is for a
    change in the opposite direction.

    A "successful status task" is any successful special task.  A
    "failed status task" is a failed Repair task.  These criteria
    are based on the definition of "status task" in the module-level
    docstring, above.

    This is the RPC endpoint for `AFE.get_host_diagnosis_interval()`.

    @param host_id     Database host id of the desired host.
    @param end_time    Find the last eligible interval before this time.
    @param success     Whether the eligible interval should start with a
                       success or a failure.

    @return A list containing the start time of the earliest job
            selected, and the end time of the latest job.

    """
    base_query = afe_models.SpecialTask.objects.filter(
            host_id=host_id, is_complete=True)
    success_query = base_query.filter(success=True)
    failure_query = base_query.filter(success=False, task='Repair')
    if success:
        query0 = success_query
        query1 = failure_query
    else:
        query0 = failure_query
        query1 = success_query
    query0 = query0.filter(time_finished__lte=end_time)
    query0 = query0.order_by('time_started').reverse()
    if not query0:
        return []
    task0 = query0[0]
    query1 = query1.filter(time_finished__gt=task0.time_finished)
    task1 = query1.order_by('time_started')[0]
    return [task0.time_started.strftime(time_utils.TIME_FMT),
            task1.time_finished.strftime(time_utils.TIME_FMT)]


def get_status_task(host_id, end_time):
    """Get the last status task for a host before a given time.

    This routine returns a Django query for the AFE database to find
    the last task that finished on the given host before the given
    time that was either a successful task, or a Repair task.  The
    query criteria are based on the definition of "status task" in
    the module-level docstring, above.

    This is the RPC endpoint for `_SpecialTaskEvent.get_status_task()`.

    @param host_id     Database host id of the desired host.
    @param end_time    End time of the range of interest.

    @return A Django query-set selecting the single special task of
            interest.

    """
    # Selects status tasks:  any Repair task, or any successful task.
    status_tasks = (django_models.Q(task='Repair') |
                    django_models.Q(success=True))
    # Our caller needs a Django query set in order to serialize the
    # result, so we don't resolve the query here; we just return a
    # slice with at most one element.
    return afe_models.SpecialTask.objects.filter(
            status_tasks,
            host_id=host_id,
            time_finished__lte=end_time,
            is_complete=True).order_by('time_started').reverse()[0:1]
