# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utils for recording job overhead in metadata db."""

import logging

from autotest_lib.client.common_lib import enum
from autotest_lib.client.common_lib import host_queue_entry_states
from autotest_lib.client.common_lib import host_states
from autotest_lib.client.common_lib.cros.graphite import autotest_es


# Metadata db type string for job time stats
DEFAULT_KEY = 'job_time_breakdown'

# Metadata db type string for suite time stats
SUITE_RUNTIME_KEY = 'suite_runtime'

# Job breakdown statuses
_hs = host_states.Status
_qs = host_queue_entry_states.Status
_status_list = [
        _qs.QUEUED, _qs.RESETTING, _qs.VERIFYING,
        _qs.PROVISIONING, _hs.REPAIRING, _qs.CLEANING,
        _qs.RUNNING, _qs.GATHERING, _qs.PARSING]
STATUS = enum.Enum(*_status_list, string_values=True)


def record_state_duration(
        job_or_task_id, hostname, status, duration_secs,
        type_str=DEFAULT_KEY, is_special_task=False):
    """Record state duration for a job or a task.

    @param job_or_task_id: Integer, representing a job id or a special task id.
    @param hostname: String, representing a hostname.
    @param status: One of the enum values of job_overhead.STATUS.
    @param duration_secs: Duration of the job/task in secs.
    @param is_special_task: True/Fals, whether this is a special task.
    @param type_str: The elastic search type string to be used when sending data
                     to metadata db.
    """
    if not job_or_task_id or not hostname or not status:
        logging.error(
                'record_state_duration failed: job_or_task_id=%s, '
                'hostname=%s, status=%s', job_or_task_id, hostname, status)
        return
    id_str = 'task_id' if is_special_task else 'job_id'
    metadata = {
            id_str: int(job_or_task_id),
            'hostname': hostname,
            'status': status,
            'duration': duration_secs}
    autotest_es.post(type_str=type_str, metadata=metadata)


def record_suite_runtime(suite_job_id, suite_name, board, build, num_child_jobs,
                         runtime_in_secs):
    """Record suite runtime.

    @param suite_job_id: The job id of the suite for which we are going to
                         collect stats.
    @param suite_name: The suite name, e.g. 'bvt', 'dummy'.
    @param board: The target board for which the suite is run,
                  e.g., 'lumpy', 'link'.
    @param build: The build for which the suite is run,
                  e.g. 'lumpy-release/R35-5712.0.0'.
    @param num_child_jobs: Total number of child jobs of the suite.
    @param runtime_in_secs: Duration of the suite from the start to the end.
    """
    metadata = {
            'suite_job_id': suite_job_id,
            'suite_name': suite_name,
            'board': board,
            'build': build,
            'num_child_jobs': num_child_jobs,
            'duration': runtime_in_secs}
    autotest_es.post(type_str=SUITE_RUNTIME_KEY, metadata=metadata)
