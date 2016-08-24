# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.scheduler import rdb
from autotest_lib.scheduler import rdb_cache_manager
from autotest_lib.scheduler import rdb_lib
from autotest_lib.scheduler import rdb_testing_utils as test_utils
from autotest_lib.scheduler import rdb_utils


def get_line_with_labels(required_labels, cache_lines):
    """Get the cache line with the hosts that match given labels.

    Confirm that all hosts have matching labels within a line,
    then return the lines with the requested labels. There can
    be more than one, since we use acls in the cache key.

    @param labels: A list of label names.
    @cache_lines: A list of cache lines to look through.

    @return: A list of the cache lines with the requested labels.
    """
    label_lines = []
    for line in cache_lines:
        if not line:
            continue
        labels = list(line)[0].labels.get_label_names()
        if any(host.labels.get_label_names() != labels for host in line):
            raise AssertionError('Mismatch in deps within a cache line')
        if required_labels == labels:
            label_lines.append(line)
    return label_lines


def get_hosts_for_request(
        response_map, deps=test_utils.DEFAULT_DEPS,
        acls=test_utils.DEFAULT_ACLS, priority=0, parent_job_id=0, **kwargs):
    """Get the hosts for a request matching kwargs from the response map.

    @param response_map: A response map from an rdb request_handler.
    """
    return response_map[
            test_utils.AbstractBaseRDBTester.get_request(
                    deps, acls, priority, parent_job_id)]


class RDBCacheTest(test_utils.AbstractBaseRDBTester, unittest.TestCase):
    """Unittests for RDBHost objects."""


    def testCachingBasic(self):
        """Test that different requests will hit the database."""

        # r1 should cache h2 and use h1; r2 should cach [] and use h2
        # at the end the cache should contain one stale line, with
        # h2 in it, and one empty line since r2 acquired h2.
        default_params = test_utils.get_default_job_params()
        self.create_job(**default_params)
        default_params['deps'] = default_params['deps'][0]
        self.create_job(**default_params)
        for i in range(0, 2):
            self.db_helper.create_host(
                    'h%s'%i, **test_utils.get_default_host_params())
        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        def local_get_response(self):
            """ Local rdb.get_response handler."""
            requests = self.response_map.keys()
            if not (self.cache.hits == 0 and self.cache.misses == 2):
                raise AssertionError('Neither request should have hit the '
                        'cache, but both should have inserted into it.')

            lines = get_line_with_labels(
                    test_utils.DEFAULT_DEPS,
                    self.cache._cache_backend._cache.values())
            if len(lines) > 1:
                raise AssertionError('Caching was too agressive, '
                        'the second request should not have cached anything '
                        'because it used the one free host.')

            cached_host = lines[0].pop()
            default_params = test_utils.get_default_job_params()
            job1_host = get_hosts_for_request(
                    self.response_map, **default_params)[0]
            default_params['deps'] = default_params['deps'][0]
            job2_host = get_hosts_for_request(
                    self.response_map, **default_params)[0]
            if (job2_host.hostname == job1_host.hostname or
                cached_host.hostname not in
                [job2_host.hostname, job1_host.hostname]):
                raise AssertionError('Wrong host cached %s. The first job '
                        'should have cached the host used by the second.' %
                        cached_host.hostname)

            # Shouldn't be able to lease this host since r2 used it.
            try:
                cached_host.lease()
            except rdb_utils.RDBException:
                pass
            else:
                raise AssertionError('Was able to lease a stale host. The '
                        'second request should have leased it.')
            return test_utils.wire_format_response_map(self.response_map)

        self.god.stub_with(rdb.AvailableHostRequestHandler,
                           'get_response', local_get_response)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testCachingPriority(self):
        """Test requests with the same deps but different priorities."""
        # All 3 jobs should find hosts, and there should be one host left
        # behind in the cache. The first job will take one host and cache 3,
        # the second will take one and cache 2, while the last will take one.
        # The remaining host in the cache should not be stale.
        default_job_params = test_utils.get_default_job_params()
        for i in range(0, 3):
            default_job_params['priority'] = i
            job = self.create_job(**default_job_params)

        default_host_params = test_utils.get_default_host_params()
        for i in range(0, 4):
            self.db_helper.create_host('h%s'%i, **default_host_params)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        def local_get_response(self):
            """ Local rdb.get_response handler."""
            if not (self.cache.hits == 2 and self.cache.misses ==1):
                raise AssertionError('The first request should have populated '
                        'the cache for the others.')

            default_job_params = test_utils.get_default_job_params()
            lines = get_line_with_labels(
                    default_job_params['deps'],
                    self.cache._cache_backend._cache.values())
            if len(lines) > 1:
                raise AssertionError('Should only be one cache line left.')

            # Make sure that all the jobs got different hosts, and that
            # the host cached isn't being used by a job.
            cached_host = lines[0].pop()
            cached_host.lease()

            job_hosts = []
            default_job_params = test_utils.get_default_job_params()
            for i in range(0, 3):
                default_job_params['priority'] = i
                hosts = get_hosts_for_request(self.response_map,
                                              **default_job_params)
                assert(len(hosts) == 1)
                host = hosts[0]
                assert(host.id not in job_hosts and cached_host.id != host.id)
            return test_utils.wire_format_response_map(self.response_map)

        self.god.stub_with(rdb.AvailableHostRequestHandler,
                           'get_response', local_get_response)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testCachingEmptyList(self):
        """Test that the 'no available hosts' condition isn't a cache miss."""
        default_params = test_utils.get_default_job_params()
        for i in range(0 ,3):
            default_params['parent_job_id'] = i
            self.create_job(**default_params)

        default_host_params = test_utils.get_default_host_params()
        self.db_helper.create_host('h1', **default_host_params)

        def local_get_response(self):
            """ Local rdb.get_response handler."""
            if not (self.cache.misses == 1 and self.cache.hits == 2):
                raise AssertionError('The first request should have taken h1 '
                        'while the other 2 should have hit the cache.')

            request = test_utils.AbstractBaseRDBTester.get_request(
                    test_utils.DEFAULT_DEPS, test_utils.DEFAULT_ACLS)
            key = self.cache.get_key(deps=request.deps, acls=request.acls)
            if self.cache._cache_backend.get(key) != []:
                raise AssertionError('A request with no hosts does not get '
                        'cached corrrectly.')
            return test_utils.wire_format_response_map(self.response_map)

        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        self.god.stub_with(rdb.AvailableHostRequestHandler,
                           'get_response', local_get_response)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testStaleCacheLine(self):
        """Test that a stale cache line doesn't satisfy a request."""

        # Create 3 jobs, all of which can use the same hosts. The first
        # will cache the only remaining host after taking one, the second
        # will also take a host, but not cache anything, while the third
        # will try to use the host cached by the first job but fail because
        # it is already leased.
        default_params = test_utils.get_default_job_params()
        default_params['priority'] = 2
        self.create_job(**default_params)
        default_params['priority'] = 1
        default_params['deps'] = default_params['deps'][0]
        self.create_job(**default_params)
        default_params['priority'] = 0
        default_params['deps'] = test_utils.DEFAULT_DEPS
        self.create_job(**default_params)

        host_1 = self.db_helper.create_host(
                'h1', **test_utils.get_default_host_params())
        host_2 = self.db_helper.create_host(
                'h2', **test_utils.get_default_host_params())
        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        def local_get_response(self):
            """ Local rdb.get_response handler."""
            default_job_params = test_utils.get_default_job_params()

            # Confirm that even though the third job hit the cache, it wasn't
            # able to use the cached host because it was already leased, and
            # that it doesn't add it back to the cache.
            assert(self.cache.misses == 2 and self.cache.hits == 1)
            lines = get_line_with_labels(
                        default_job_params['deps'],
                        self.cache._cache_backend._cache.values())
            assert(len(lines) == 0)
            assert(int(self.cache.mean_staleness()) == 100)
            return test_utils.wire_format_response_map(self.response_map)

        self.god.stub_with(rdb.AvailableHostRequestHandler,
                           'get_response', local_get_response)
        acquired_hosts = list(rdb_lib.acquire_hosts(queue_entries))
        self.assertTrue(acquired_hosts[0].id == host_1.id and
                        acquired_hosts[1].id == host_2.id and
                        acquired_hosts[2] is None)


    def testCacheAPI(self):
        """Test the cache managers api."""
        cache = rdb_cache_manager.RDBHostCacheManager()
        key = cache.get_key(
                deps=test_utils.DEFAULT_DEPS, acls=test_utils.DEFAULT_ACLS)

        # Cannot set None, it's reserved for cache expiration.
        self.assertRaises(rdb_utils.RDBException, cache.set_line, *(key, None))

        # Setting an empty list indicates a query with no results.
        cache.set_line(key, [])
        self.assertTrue(cache.get_line(key) == [])

        # Getting a value will delete the key, leading to a miss on subsequent
        # gets before a set.
        self.assertRaises(rdb_utils.CacheMiss, cache.get_line, *(key,))

        # Caching a leased host is just a waste of cache space.
        host = test_utils.FakeHost(
                'h1', 1, labels=test_utils.DEFAULT_DEPS,
                acls=test_utils.DEFAULT_ACLS, leased=1)
        cache.set_line(key, [host])
        self.assertRaises(
                rdb_utils.CacheMiss, cache.get_line, *(key,))

        # Retrieving an unleased cached host shouldn't mutate it, even if the
        # key is reconstructed.
        host.leased=0
        cache.set_line(cache.get_key(host.labels, host.acls), [host])
        self.assertTrue(
                cache.get_line(cache.get_key(host.labels, host.acls)) == [host])

        # Caching different hosts under the same key isn't allowed.
        different_host = test_utils.FakeHost(
                'h2', 2, labels=[test_utils.DEFAULT_DEPS[0]],
                acls=test_utils.DEFAULT_ACLS, leased=0)
        cache.set_line(key, [host, different_host])
        self.assertRaises(
                rdb_utils.CacheMiss, cache.get_line, *(key,))

        # Caching hosts with the same deps but different acls under the
        # same key is allowed, as long as the acls match the key.
        different_host = test_utils.FakeHost(
                'h2', 2, labels=test_utils.DEFAULT_DEPS,
                acls=[test_utils.DEFAULT_ACLS[1]], leased=0)
        cache.set_line(key, [host, different_host])
        self.assertTrue(set(cache.get_line(key)) == set([host, different_host]))

        # Make sure we don't divide by zero while calculating hit ratio
        cache.misses = 0
        cache.hits = 0
        self.assertTrue(cache.hit_ratio() == 0)
        cache.hits = 1
        hit_ratio = cache.hit_ratio()
        self.assertTrue(type(hit_ratio) == float and hit_ratio == 100)


    def testDummyCache(self):
        """Test that the dummy cache doesn't save hosts."""

        # Create 2 jobs and 3 hosts. Both the jobs should not hit the cache,
        # nor should they cache anything, but both jobs should acquire hosts.
        default_params = test_utils.get_default_job_params()
        default_host_params = test_utils.get_default_host_params()
        for i in range(0, 2):
            default_params['parent_job_id'] = i
            self.create_job(**default_params)
            self.db_helper.create_host('h%s'%i, **default_host_params)
        self.db_helper.create_host('h2', **default_host_params)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        self.god.stub_with(
                rdb_cache_manager.RDBHostCacheManager, 'use_cache', False)

        def local_get_response(self):
            """ Local rdb.get_response handler."""
            requests = self.response_map.keys()
            if not (self.cache.hits == 0 and self.cache.misses == 2):
                raise AssertionError('Neither request should have hit the '
                        'cache, but both should have inserted into it.')

            # Make sure both requests actually found a host
            default_params = test_utils.get_default_job_params()
            job1_host = get_hosts_for_request(
                    self.response_map, **default_params)[0]
            default_params['parent_job_id'] = 1
            job2_host = get_hosts_for_request(
                    self.response_map, **default_params)[0]
            if (not job1_host or not job2_host or
                job2_host.hostname == job1_host.hostname):
                raise AssertionError('Excected acquisitions did not occur.')

            assert(hasattr(self.cache._cache_backend, '_cache') == False)
            return test_utils.wire_format_response_map(self.response_map)

        self.god.stub_with(rdb.AvailableHostRequestHandler,
                           'get_response', local_get_response)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))

