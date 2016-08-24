# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""RDB utilities.

Do not import rdb or autotest modules here to avoid cyclic dependencies.
"""

import collections

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.common_lib import priorities

RDB_STATS_KEY = 'rdb'

class RDBException(Exception):
    """Generic RDB exception."""

    def wire_format(self, **kwargs):
        """Convert the exception to a format better suited to an rpc response.
        """
        return str(self)


class CacheMiss(RDBException):
    """Generic exception raised for a cache miss in the rdb."""
    pass


class LabelIterator(object):
    """An Iterator for labels.

    Within the rdb any label/dependency comparisons are performed based on label
    ids. However, the host object returned needs to contain label names instead.
    This class returns label ids for iteration, but a list of all label names
    when accessed through get_label_names.
    """

    def __init__(self, labels):
        self.labels = labels


    def __iter__(self):
        return iter(label.id for label in self.labels)


    def get_label_names(self):
        """Get all label names of the labels associated with this class.

        @return: A list of label names.
        """
        return [label.name for label in self.labels]


class RequestAccountant(object):
    """A helper class that count requests and manages min_duts requirement.

    On initialization, this object takes a list of host requests.
    It will batch the requests by grouping similar requests together
    and generate a mapping from unique request-> count of the request.
    It will also generates a mapping from suite_job_id -> min_duts.

    RDB does a two-round of host aquisition. The first round attempts
    to get min_duts for each suite. The second round attemps to satisfy
    the rest requests.  RDB calls get_min_duts and get_rest to
    figure out how many duts it should attempt to get for a unique
    request in the first and second round respectively.

    Assume we have two distinct requests
          R1 (parent_job_id: 10, need hosts: 2)
          R2 (parent_job_id: 10, need hosts: 4)
    And parent job P (job_id:10) has min dut requirement of 3. So we got
          requests_to_counts = {R1: 2, R2: 4}
          min_duts_map = {P: 3}

    First round acquiring:
    Call get_min_duts(R1)
          return 2, because P hasn't reach its min dut limit (3) yet
          requests_to_counts -> {R1: 2-2=0, R2: 4}
          min_duts_map -> {P: 3-2=1}

    Call get_min_duts(R2)
         return 1, because although R2 needs 4 duts, P's min dut limit is now 1
          requests_to_counts -> {R1: 0, R2: 4-1=3}
          min_duts_map -> {P: 1-1=0}

    Second round acquiring:
    Call get_rest(R1):
         return 0, requests_to_counts[R1]
    Call get_rest(R2):
         return 3, requests_to_counts[R2]

    Note it is possible that in the first round acquiring, although
    R1 requested 2 duts, it may only get 1 or None. However get_rest
    doesn't need to care whether the first round succeeded or not, as
    in the case when the first round failed, regardless how many duts
    get_rest requests, it will not be fullfilled anyway.
    """

    _gauge = autotest_stats.Gauge(RDB_STATS_KEY)


    def __init__(self, host_requests):
        """Initialize.

        @param host_requests: A list of request to acquire hosts.
        """
        self.requests_to_counts = {}
        # The order matters, it determines which request got fullfilled first.
        self.requests = []
        for request, count in self._batch_requests(host_requests):
            self.requests.append(request)
            self.requests_to_counts[request] = count
        self.min_duts_map = dict(
                (r.parent_job_id, r.suite_min_duts)
                for r in self.requests_to_counts.iterkeys() if r.parent_job_id)


    @classmethod
    def _batch_requests(cls, requests):
        """ Group similar requests, sort by priority and parent_job_id.

        @param requests: A list or unsorted, unordered requests.

        @return: A list of tuples of the form (request, number of occurances)
            formed by counting the number of requests with the same acls/deps/
            priority in the input list of requests, and sorting by priority.
            The order of this list ensures against priority inversion.
        """
        sort_function = lambda request: (request[0].priority,
                                         -request[0].parent_job_id)
        return sorted(collections.Counter(requests).items(), key=sort_function,
                      reverse=True)


    def get_min_duts(self, host_request):
        """Given a distinct host request figure out min duts to request for.

        @param host_request: A request.
        @returns: The minimum duts that should be requested.
        """
        parent_id = host_request.parent_job_id
        count = self.requests_to_counts[host_request]
        if parent_id:
            min_duts = self.min_duts_map.get(parent_id, 0)
            to_acquire = min(count, min_duts)
            self.min_duts_map[parent_id] = max(0, min_duts - to_acquire)
        else:
            to_acquire = 0
        self.requests_to_counts[host_request] -= to_acquire
        return to_acquire


    def get_duts(self, host_request):
        """Return the number of duts host_request still need.

        @param host_request: A request.
        @returns: The number of duts need to be requested.
        """
        return self.requests_to_counts[host_request]


    def record_acquire_min_duts(cls, host_request, hosts_required,
                                acquired_host_count):
        """Send stats to graphite about host acquisition.

        @param host_request: A request.
        @param hosts_required: Number of hosts required to satisfy request.
        @param acquired_host_count: Number of host acquired.
        """
        try:
            priority =  priorities.Priority.get_string(host_request.priority)
        except ValueError:
            return
        key = ('min_duts_acquisition_rate.%s') % priority
        cls._gauge.send(key, acquired_host_count/float(hosts_required))
