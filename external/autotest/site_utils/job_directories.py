import abc
import datetime
import glob
import json
import os
import re
import shutil
import time

import common
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


_AFE = frontend_wrappers.RetryingAFE()

SPECIAL_TASK_PATTERN = '.*/hosts/[^/]+/(\d+)-[^/]+'
JOB_PATTERN = '.*/(\d+)-[^/]+'

def _is_job_expired(age_limit, timestamp):
  """Check whether a job timestamp is older than an age limit.

  @param age_limit: Minimum age, measured in days.  If the value is
                    not positive, the job is always expired.
  @param timestamp: Timestamp of the job whose age we are checking.
                    The format must match time_utils.TIME_FMT.

  @returns True iff the job is old enough to be expired.
  """
  if age_limit <= 0:
    return True
  job_time = time_utils.time_string_to_datetime(timestamp)
  expiration = job_time + datetime.timedelta(days=age_limit)
  return datetime.datetime.now() >= expiration


def get_job_id_or_task_id(result_dir):
    """Extract job id or special task id from result_dir

    @param result_dir: path to the result dir.
            For test job:
            /usr/local/autotest/results/2032-chromeos-test/chromeos1-rack5-host6
            The hostname at the end is optional.
            For special task:
            /usr/local/autotest/results/hosts/chromeos1-rack5-host6/1343-cleanup

    @returns: integer representing the job id or task id. Returns None if fail
              to parse job or task id from the result_dir.
    """
    if not result_dir:
        return
    result_dir = os.path.abspath(result_dir)
    # Result folder for job running inside container has only job id.
    ssp_job_pattern = '.*/(\d+)$'
    # Try to get the job ID from the last pattern of number-text. This avoids
    # issue with path like 123-results/456-debug_user, in which 456 is the real
    # job ID.
    m_job = re.findall(JOB_PATTERN, result_dir)
    if m_job:
        return int(m_job[-1])
    m_special_task = re.match(SPECIAL_TASK_PATTERN, result_dir)
    if m_special_task:
        return int(m_special_task.group(1))
    m_ssp_job_pattern = re.match(ssp_job_pattern, result_dir)
    if m_ssp_job_pattern and utils.is_in_container():
        return int(m_ssp_job_pattern.group(1))


class _JobDirectory(object):
  """State associated with a job to be offloaded.

  The full life-cycle of a job (including failure events that
  normally don't occur) looks like this:
   1. The job's results directory is discovered by
      `get_job_directories()`, and a job instance is created for it.
   2. Calls to `offload()` have no effect so long as the job
      isn't complete in the database and the job isn't expired
      according to the `age_limit` parameter.
   3. Eventually, the job is both finished and expired.  The next
      call to `offload()` makes the first attempt to offload the
      directory to GS.  Offload is attempted, but fails to complete
      (e.g. because of a GS problem).
   4. After the first failed offload `is_offloaded()` is false,
      but `is_reportable()` is also false, so the failure is not
      reported.
   5. Another call to `offload()` again tries to offload the
      directory, and again fails.
   6. After a second failure, `is_offloaded()` is false and
      `is_reportable()` is true, so the failure generates an e-mail
      notification.
   7. Finally, a call to `offload()` succeeds, and the directory no
      longer exists.  Now `is_offloaded()` is true, so the job
      instance is deleted, and future failures will not mention this
      directory any more.

  Only steps 1. and 7. are guaranteed to occur.  The others depend
  on the timing of calls to `offload()`, and on the reliability of
  the actual offload process.

  """

  __metaclass__ = abc.ABCMeta

  GLOB_PATTERN = None   # must be redefined in subclass

  def __init__(self, resultsdir):
    self._dirname = resultsdir
    self._id = get_job_id_or_task_id(resultsdir)
    self._offload_count = 0
    self._first_offload_start = 0

  @classmethod
  def get_job_directories(cls):
    """Return a list of directories of jobs that need offloading."""
    return [d for d in glob.glob(cls.GLOB_PATTERN) if os.path.isdir(d)]

  @abc.abstractmethod
  def get_timestamp_if_finished(self):
    """Return this job's timestamp from the database.

    If the database has not marked the job as finished, return
    `None`.  Otherwise, return a timestamp for the job.  The
    timestamp is to be used to determine expiration in
    `_is_job_expired()`.

    @return Return `None` if the job is still running; otherwise
            return a string with a timestamp in the appropriate
            format.
    """
    raise NotImplementedError("_JobDirectory.get_timestamp_if_finished")

  def enqueue_offload(self, queue, age_limit):
    """Enqueue the job for offload, if it's eligible.

    The job is eligible for offloading if the database has marked
    it finished, and the job is older than the `age_limit`
    parameter.

    If the job is eligible, offload processing is requested by
    passing the `queue` parameter's `put()` method a sequence with
    the job's `_dirname` attribute and its directory name.

    @param queue     If the job should be offloaded, put the offload
                     parameters into this queue for processing.
    @param age_limit Minimum age for a job to be offloaded.  A value
                     of 0 means that the job will be offloaded as
                     soon as it is finished.

    """
    if not self._offload_count:
      timestamp = self.get_timestamp_if_finished()
      if not timestamp:
        return
      if not _is_job_expired(age_limit, timestamp):
        return
      self._first_offload_start = time.time()
    self._offload_count += 1
    if self.process_gs_instructions():
      queue.put([self._dirname, os.path.dirname(self._dirname)])

  def is_offloaded(self):
    """Return whether this job has been successfully offloaded."""
    return not os.path.exists(self._dirname)

  def is_reportable(self):
    """Return whether this job has a reportable failure."""
    return self._offload_count > 1

  def get_failure_time(self):
    """Return the time of the first offload failure."""
    return self._first_offload_start

  def get_failure_count(self):
    """Return the number of times this job has failed to offload."""
    return self._offload_count

  def get_job_directory(self):
    """Return the name of this job's results directory."""
    return self._dirname

  def process_gs_instructions(self):
    """Process any gs_offloader instructions for this special task.

    @returns True/False if there is anything left to offload.
    """
    # Default support is to still offload the directory.
    return True


class RegularJobDirectory(_JobDirectory):
  """Subclass of _JobDirectory for regular test jobs."""

  GLOB_PATTERN = '[0-9]*-*'

  def process_gs_instructions(self):
    """Process any gs_offloader instructions for this job.

    @returns True/False if there is anything left to offload.
    """
    # Go through the gs_offloader instructions file for each test in this job.
    for path in glob.glob(os.path.join(self._dirname, '*',
                                       constants.GS_OFFLOADER_INSTRUCTIONS)):
      with open(path, 'r') as f:
        gs_off_instructions = json.load(f)
      if gs_off_instructions.get(constants.GS_OFFLOADER_NO_OFFLOAD):
        shutil.rmtree(os.path.dirname(path))

    # Finally check if there's anything left to offload.
    if not os.listdir(self._dirname):
      shutil.rmtree(self._dirname)
      return False
    return True


  def get_timestamp_if_finished(self):
    """Get the timestamp to use for finished jobs.

    @returns the latest hqe finished_on time. If the finished_on times are null
             returns the job's created_on time.
    """
    entry = _AFE.get_jobs(id=self._id, finished=True)
    if not entry:
      return None
    hqes = _AFE.get_host_queue_entries(finished_on__isnull=False,
                                       job_id=self._id)
    if not hqes:
      return entry[0].created_on
    # While most Jobs have 1 HQE, some can have multiple, so check them all.
    return max([hqe.finished_on for hqe in hqes])


class SpecialJobDirectory(_JobDirectory):
  """Subclass of _JobDirectory for special (per-host) jobs."""

  GLOB_PATTERN = 'hosts/*/[0-9]*-*'

  def __init__(self, resultsdir):
    super(SpecialJobDirectory, self).__init__(resultsdir)

  def get_timestamp_if_finished(self):
    entry = _AFE.get_special_tasks(id=self._id, is_complete=True)
    return entry[0].time_finished if entry else None
