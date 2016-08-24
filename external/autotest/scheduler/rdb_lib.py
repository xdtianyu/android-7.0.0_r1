# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module manages translation between monitor_db and the rdb. """

import common
from autotest_lib.scheduler import rdb
from autotest_lib.scheduler import rdb_hosts
from autotest_lib.scheduler import rdb_requests
from autotest_lib.server.cros import provision


# Adapters for scheduler specific objects: Convert job information to a
# format more ameanable to the rdb/rdb request managers.
class JobQueryManager(object):
    """A caching query manager for all job related information."""
    def __init__(self, queue_entries, suite_min_duts=None):
        """Initialize.

        @param queue_entries: A list of HostQueueEntry objects.
        @param suite_min_duts: A dictionary where the key is suite job id,
                and the value is the value of 'suite_min_dut' in the suite's
                job keyvals. It should cover all the suite jobs which
                the jobs (associated with the queue_entries) belong to.
        """
        # TODO(beeps): Break this dependency on the host_query_manager,
        # crbug.com/336934.
        from autotest_lib.scheduler import query_managers
        self.query_manager = query_managers.AFEHostQueryManager()
        jobs = [queue_entry.job_id for queue_entry in queue_entries]
        self._job_acls = self.query_manager._get_job_acl_groups(jobs)
        self._job_deps = self.query_manager._get_job_dependencies(jobs)
        self._labels = self.query_manager._get_labels(self._job_deps)
        self._suite_min_duts = suite_min_duts or {}


    def get_job_info(self, queue_entry):
        """Extract job information from a queue_entry/host-scheduler.

        @param queue_entry: The queue_entry for which we need job information.

        @return: A dictionary representing job related information.
        """
        job_id = queue_entry.job_id
        job_deps, job_preferred_deps = [], []
        for dep in self._job_deps.get(job_id, []):
           if not provision.is_for_special_action(self._labels[dep].name):
               job_deps.append(dep)
           elif provision.Provision.acts_on(self._labels[dep].name):
               job_preferred_deps.append(dep)

        job_acls = self._job_acls.get(job_id, [])
        parent_id = queue_entry.job.parent_job_id
        min_duts = self._suite_min_duts.get(parent_id, 0) if parent_id else 0

        return {'deps': job_deps, 'acls': job_acls,
                'preferred_deps': job_preferred_deps,
                'host_id': queue_entry.host_id,
                'parent_job_id': queue_entry.job.parent_job_id,
                'priority': queue_entry.job.priority,
                'suite_min_duts': min_duts}


def acquire_hosts(queue_entries, suite_min_duts=None):
    """Acquire hosts for the list of queue_entries.

    The act of acquisition involves leasing a host from the rdb.

    @param queue_entries: A list of queue_entries that need hosts.
    @param suite_min_duts: A dictionary that maps suite job id to the minimum
                           number of duts required.

    @yield: An rdb_hosts.RDBClientHostWrapper for each host acquired on behalf
        of a queue_entry, or None if a host wasn't found.

    @raises RDBException: If something goes wrong making the request.
    """
    job_query_manager = JobQueryManager(queue_entries, suite_min_duts)
    request_manager = rdb_requests.BaseHostRequestManager(
            rdb_requests.AcquireHostRequest, rdb.rdb_host_request_dispatcher)
    for entry in queue_entries:
        request_manager.add_request(**job_query_manager.get_job_info(entry))

    for host in request_manager.response():
        yield (rdb_hosts.RDBClientHostWrapper(**host)
               if host else None)


def get_hosts(host_ids):
    """Get information about the hosts with ids in host_ids.

    get_hosts is different from acquire_hosts in that it is completely
    oblivious to the leased state of a host.

    @param host_ids: A list of host_ids.

    @return: A list of rdb_hosts.RDBClientHostWrapper objects.

    @raises RDBException: If something goes wrong in making the request.
    """
    request_manager = rdb_requests.BaseHostRequestManager(
            rdb_requests.HostRequest, rdb.get_hosts)
    for host_id in host_ids:
        request_manager.add_request(host_id=host_id)

    hosts = []
    for host in request_manager.response():
        hosts.append(rdb_hosts.RDBClientHostWrapper(**host)
                     if host else None)
    return hosts
