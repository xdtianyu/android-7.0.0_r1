# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Cache module for rdb requests/host objects.

This module supplies the following api:
    1. A cache backend.
    2. A cache manager for the backend.
    3. A memoize decorator to encapsulate caching logic.

This cache manager functions as a lookaside buffer for host requests.
Its correctness is contingent on the following conditions:
1. The concurrency of the rdb remains 0.
2. Clients of the cache don't trust the leased bit on the cached object.
3. The cache is created at the start of a single batched request,
    populated during the request, and completely discarded at the end.

Rather than caching individual hosts, the cache manager maintains
'cache lines'. A cache line is defined as a key: value pair, where
the key is as returned by get_key, and the value is a list of RDBHosts
that match the key. The following limitations are placed on cache lines:
1. A new line can only contain unleased hosts.
2. A key can only be set once, with a single line, before removal.
3. Every 'get' deletes the entire line.

Consider the following examples:
Normal case: 3 grouped requests, all with the same deps/acls, but different
priorities/parent_job_ids. The requests (X+Y+Z) > matching hosts (K):
 (request1, count=X)- hits the database, takes X hosts, caches (K-X)
 (request2, count=Y) - hits the cache and is fully satisfied, caches (K-(X+Y))
 (request3, count=Z) - hits the cache, needs to acquire (X+Y+Z)-K next tick]:

 Host Count |  RDB                         | Cache
------------------------------------------------------------------
X:          | request1                     | {}
K:          | find_hosts(deps, acls)       |
X:          | leased_hosts                 |
K-X:        | ---------------------------> | {key: [K-X hosts]}
Y<K-X:      | request2 <---[K-X hosts]---- | {}
Y:          | leased_hosts                 |
K-(X+Y):    | ---------------------------> | {key: [K-(X+Y) hosts]}
Z>K-(X+Y):  | request3 <-[K-(X+Y) hosts]-- | {}
Z-(K-(X+Y)):| leased_hosts                 |

Since hosts are only released by the scheduler there is no way the
third request could have been satisfied completely even if we had checked
the database real-time.

Stale cache entries: 3 grouped requests that don't have the same deps/acls.
P(1,2,3) are priorities, with P3 being the highest:
 (request1(deps=[a,b], P3), Count=X) - Caches hosts
 (request2(deps=[a], P2), Count=Y) - hits the database
 (request3(deps=[a,b], P1)], Count=Z) - Tries to use cached hosts but fails

 Host Count |  RDB                         | Cache
------------------------------------------------------------------
X:          | request1(deps=[a,b])         | {}
K:          | find_hosts(deps=[a,b])       |
X:          | leased_hosts                 |
K-X:        | ---------------------------> | {deps=[a,b]: [(K-X) hosts]}
Y<K-X:      | request2(deps=[a])           | {}
K-X:        | find_hosts(deps=[a])         |
Y:          | leased_hosts                 |
K-(X+Y):    | ---------------------------> | {deps=[a]: [(K-(X+Y)) hosts],
            |                              |        | overlap |
            |                              |  deps=[a, b], [(K-X) hosts]}
Z:          | request3(deps=[a,b])<-[K-X]--| {deps=[a]: [K-(X+Y) hosts]}
Z-(K-(X+Y)):| leased_hosts                 | {deps=[a]: [N-Y hosts]}

Note that in the last case, even though the cache returns hosts that
have already been assigned to request2, request3 cannot use them. This is
acceptable because the number of hosts we lease per tick is << the number
of requests, so it's faster to check leased bits real time than query for hosts.
"""


import abc
import collections
import logging

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.common_lib.global_config import global_config
from autotest_lib.scheduler import rdb_utils

MEMOIZE_KEY = 'memoized_hosts'

def memoize_hosts(func):
    """Decorator used to memoize through the cache manager.

    @param func: The function/method to decorate.
        Before calling this function we check the cache for values matching
        its request argument, and anything returned by the function is cached
        cached under the same request.
    """
    def cache(self, request, count, **kwargs):
        """Caching function for the memoize decorator.

        @param request: The rdb request, as defined in rdb_requests.
        @param count: The count of elements needed to satisfy the request.
        @param kwargs:
            Named args for the memoized function. This map should not contain
            the key MEMOIZED_KEY, as this is reserved for the passing of
            the cached/memoized hosts to the function itself.
        """
        cache_key = self.cache.get_key(request.deps, request.acls)
        try:
            kwargs[MEMOIZE_KEY] = self.cache.get_line(cache_key)
        except rdb_utils.CacheMiss:
            pass
        hosts = func(self, request, count, **kwargs)
        self.cache.set_line(cache_key, hosts)
        return hosts
    return cache


class CacheBackend(object):
    """Base class for a cache backend."""
    __metaclass__ = abc.ABCMeta

    def set(self, key, value):
        """Set a key.

        @param key: The key to set.
        @param value: The value to cache.
        """
        pass


    def get(self, key):
        """Get the value stored under a key.

        @param key: The key to retrieve the value for.
        @return: The value stored under the key.
        @raises KeyError: If the key isn't present in the cache.
        """
        pass


    def delete(self, key):
        """Delete the key, value pair from the cache.

        @param key: The key used to find the key, value pair to delete.
        @raises KeyError: If the key isn't already in the cache.
        """
        pass


    def has_key(self, key):
        """Check if the key exists in the cache.

        @param key: The key to check.
        @return: True if the key is in the cache.
        """
        return False


class DummyCacheBackend(CacheBackend):
    """A dummy cache backend.

    This cache will claim to have no keys. Every get is a cache miss.
    """

    def get(self, key):
        raise KeyError


class InMemoryCacheBackend(CacheBackend):
    """In memory cache backend.

    Uses a simple dictionary to store key, value pairs.
    """
    def __init__(self):
        self._cache = {}

    def get(self, key):
        return self._cache[key]

    def set(self, key, value):
        self._cache[key] = value

    def delete(self, key):
        self._cache.pop(key)

    def has_key(self, key):
        return key in self._cache

# TODO: Implement a MemecacheBackend, invalidate when unleasing a host, refactor
# the AcquireHostRequest to contain a core of (deps, acls) that we can use as
# the key for population and invalidation. The caching manager is still valid,
# regardless of the backend.

class RDBHostCacheManager(object):
    """RDB Cache manager."""

    key = collections.namedtuple('key', ['deps', 'acls'])
    use_cache = global_config.get_config_value(
            'RDB', 'use_cache', type=bool, default=True)

    def __init__(self):
        self._cache_backend = (InMemoryCacheBackend()
                               if self.use_cache else DummyCacheBackend())
        self.hits = 0
        self.misses = 0
        self.stale_entries = []


    def mean_staleness(self):
        """Compute the average stale entries per line.

        @return: A floating point representing the mean staleness.
        """
        return (reduce(lambda x, y: float(x+y), self.stale_entries)/
                len(self.stale_entries)) if self.stale_entries else 0


    def hit_ratio(self):
        """Compute the hit ratio of this cache.

        @return: A floating point percentage of the hit ratio.
        """
        if not self.hits and not self.misses:
            return 0
        requests = float(self.hits + self.misses)
        return (self.hits/requests) * 100


    def record_stats(self):
        """Record stats about the cache managed by this instance."""
        hit_ratio = self.hit_ratio()
        staleness = self.mean_staleness()
        logging.debug('Cache stats: hit ratio: %.2f%%, '
                      'avg staleness per line: %.2f%%.', hit_ratio, staleness)
        autotest_stats.Gauge(rdb_utils.RDB_STATS_KEY).send(
                'cache.hit_ratio', hit_ratio)
        autotest_stats.Gauge(rdb_utils.RDB_STATS_KEY).send(
                'cache.stale_entries', staleness)


    @classmethod
    def get_key(cls, deps, acls):
        """Return a key for the given deps, acls.

        @param deps: A list of deps, as taken by the AcquireHostRequest.
        @param acls: A list of acls, as taken by the AcquireHostRequest.
        @return: A cache key for the given deps/acls.
        """
        # All requests with the same deps, acls should hit the same cache line.
        # TODO: Do something smarter with acls, only one needs to match.
        return cls.key(deps=frozenset(deps), acls=frozenset(acls))


    def get_line(self, key):
        """Clear and return the cache line matching the key.

        @param key: The key the desired cache_line is stored under.
        @return: A list of rdb hosts matching the key, or None.

        @raises rdb_utils.CacheMiss: If the key isn't in the cache.
        """
        try:
            cache_line = self._cache_backend.get(key)
        except KeyError:
            self.misses += 1
            raise rdb_utils.CacheMiss('Key %s not in cache' % (key,))
        self.hits += 1
        self._cache_backend.delete(key)
        return list(cache_line)


    def _check_line(self, line, key):
        """Sanity check a cache line.

        This method assumes that a cache line is made up of RDBHost objects,
        and checks to see if they all match each other/the key passed in.
        Checking is done in terms of host labels and acls, note that the hosts
        in the line can have different deps/acls, as long as they all have the
        deps required by the key, and at least one matching acl of the key.

        @param line: The cache line value.
        @param key: The key the line will be stored under.
        @raises rdb_utils.RDBException:
            If one of the hosts in the cache line is already leased.
            The cache already has a different line under the given key.
            The given key doesn't match the hosts in the line.
        """
        # Note that this doesn't mean that all hosts in the cache are unleased.
        if any(host.leased for host in line):
            raise rdb_utils.RDBException('Cannot cache leased hosts %s' % line)

        # Confirm that the given line can be used to service the key by checking
        # that all hosts have the deps mentioned in the key, and at least one
        # matching acl.
        h_keys = set([self.get_key(host.labels, host.acls) for host in line])
        for h_key in h_keys:
            if (not h_key.deps.issuperset(key.deps) or
                    not key.acls.intersection(h_key.acls)):
                raise rdb_utils.RDBException('Given key: %s does not match key '
                        'computed from hosts in line: %s' % (key, h_keys))
        if self._cache_backend.has_key(key):
            raise rdb_utils.RDBException('Cannot override a cache line. It '
                    'must be cleared before setting. Key: %s, hosts %s' %
                    (key, line))


    def set_line(self, key, hosts):
        """Cache a list of similar hosts.

        set_line will no-op if:
            The hosts aren't all unleased.
            The hosts don't have deps/acls matching the key.
            A cache line under the same key already exists.
        The first 2 cases will lead to a cache miss in the corresponding get.

        @param hosts: A list of unleased hosts with the same deps/acls.
        @raises RDBException: If hosts is None, since None is reserved for
            key expiration.
        """
        if hosts is None:
            raise rdb_utils.RDBException('Cannot set None in the cache.')

        # An empty list means no hosts matching the request are available.
        # This can happen if a previous request leased all matching hosts.
        if not hosts or not self.use_cache:
            self._cache_backend.set(key, [])
            return
        try:
            self._check_line(hosts, key)
        except rdb_utils.RDBException as e:
            logging.error(e)
        else:
            self._cache_backend.set(key, set(hosts))
