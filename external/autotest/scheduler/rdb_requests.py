# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""RDB request managers and requests.

RDB request managers: Call an rdb api_method with a list of RDBRequests, and
match the requests to the responses returned.

RDB Request classes: Used in conjunction with the request managers. Each class
defines the set of fields the rdb needs to fulfill the request, and a hashable
request object the request managers use to identify a response with a request.
"""

import collections

import common
from autotest_lib.scheduler import rdb_utils


class RDBRequestManager(object):
    """Base request manager for RDB requests.

    Each instance of a request manager is associated with one request, and
    one api call. All subclasses maintain a queue of unexecuted requests, and
    and expose an api to add requests/retrieve the response for these requests.
    """


    def __init__(self, request, api_call):
        """
        @param request: A subclass of rdb_utls.RDBRequest. The manager can only
            manage requests of one type.
        @param api_call: The rdb api call this manager is expected to make.
            A manager can only send requests of type request, to this api call.
        """
        self.request = request
        self.api_call = api_call
        self.request_queue = []


    def add_request(self, **kwargs):
        """Add an RDBRequest to the queue."""
        self.request_queue.append(self.request(**kwargs).get_request())


    def response(self):
        """Execute the api call and return a response for each request.

        The order of responses is the same as the order of requests added
        to the queue.

        @yield: A response for each request added to the queue after the
            last invocation of response.
        """
        if not self.request_queue:
            raise rdb_utils.RDBException('No requests. Call add_requests '
                    'with the appropriate kwargs, before calling response.')

        result = self.api_call(self.request_queue)
        requests = self.request_queue
        self.request_queue = []
        for request in requests:
            yield result.get(request) if result else None


class BaseHostRequestManager(RDBRequestManager):
    """Manager for batched get requests on hosts."""


    def response(self):
        """Yields a popped host from the returned host list."""

        # As a side-effect of returning a host, this method also removes it
        # from the list of hosts matched up against a request. Eg:
        #    hqes: [hqe1, hqe2, hqe3]
        #    client requests: [c_r1, c_r2, c_r3]
        #    generate requests in rdb: [r1 (c_r1 and c_r2), r2]
        #    and response {r1: [h1, h2], r2:[h3]}
        # c_r1 and c_r2 need to get different hosts though they're the same
        # request, because they're from different queue_entries.
        for hosts in super(BaseHostRequestManager, self).response():
            yield hosts.pop() if hosts else None


class RDBRequestMeta(type):
    """Metaclass for constructing rdb requests.

    This meta class creates a read-only request template by combining the
    request_arguments of all classes in the inheritence hierarchy into a
    namedtuple.
    """
    def __new__(cls, name, bases, dctn):
        for base in bases:
            try:
                dctn['_request_args'].update(base._request_args)
            except AttributeError:
                pass
        dctn['template'] = collections.namedtuple('template',
                                                  dctn['_request_args'])
        return type.__new__(cls, name, bases, dctn)


class RDBRequest(object):
    """Base class for an rdb request.

    All classes inheriting from RDBRequest will need to specify a list of
    request_args necessary to create the request, and will in turn get a
    request that the rdb understands.
    """
    __metaclass__ = RDBRequestMeta
    __slots__ = set(['_request_args', '_request'])
    _request_args = set([])


    def __init__(self, **kwargs):
        for key,value in kwargs.iteritems():
            try:
                hash(value)
            except TypeError as e:
                raise rdb_utils.RDBException('All fields of a %s must be. '
                        'hashable %s: %s, %s failed this test.' %
                        (self.__class__, key, type(value), value))
        try:
            self._request = self.template(**kwargs)
        except TypeError:
            raise rdb_utils.RDBException('Creating %s requires args %s got %s' %
                    (self.__class__, self.template._fields, kwargs.keys()))


    def get_request(self):
        """Returns a request that the rdb understands.

        @return: A named tuple with all the fields necessary to make a request.
        """
        return self._request


class HashableDict(dict):
    """A hashable dictionary.

    This class assumes all values of the input dict are hashable.
    """

    def __hash__(self):
        return hash(tuple(sorted(self.items())))


class HostRequest(RDBRequest):
    """Basic request for information about a single host.

    Eg: HostRequest(host_id=x): Will return all information about host x.
    """
    _request_args =  set(['host_id'])


class UpdateHostRequest(HostRequest):
    """Defines requests to update hosts.

    Eg:
        UpdateHostRequest(host_id=x, payload={'afe_hosts_col_name': value}):
            Will update column afe_hosts_col_name with the given value, for
            the given host_id.

    @raises RDBException: If the input arguments don't contain the expected
        fields to make the request, or are of the wrong type.
    """
    _request_args = set(['payload'])


    def __init__(self, **kwargs):
        try:
            kwargs['payload'] = HashableDict(kwargs['payload'])
        except (KeyError, TypeError) as e:
            raise rdb_utils.RDBException('Creating %s requires args %s got %s' %
                    (self.__class__, self.template._fields, kwargs.keys()))
        super(UpdateHostRequest, self).__init__(**kwargs)


class AcquireHostRequest(HostRequest):
    """Defines requests to acquire hosts.

    Eg:
        AcquireHostRequest(host_id=None, deps=[d1, d2], acls=[a1, a2],
                priority=None, parent_job_id=None): Will acquire and return a
                host that matches the specified deps/acls.
        AcquireHostRequest(host_id=x, deps=[d1, d2], acls=[a1, a2]) : Will
            acquire and return host x, after checking deps/acls match.

    @raises RDBException: If the the input arguments don't contain the expected
        fields to make a request, or are of the wrong type.
    """
    # TODO(beeps): Priority and parent_job_id shouldn't be a part of the
    # core request.
    _request_args = set(['priority', 'deps', 'preferred_deps', 'acls',
                         'parent_job_id', 'suite_min_duts'])


    def __init__(self, **kwargs):
        try:
            kwargs['deps'] = frozenset(kwargs['deps'])
            kwargs['preferred_deps'] = frozenset(kwargs['preferred_deps'])
            kwargs['acls'] = frozenset(kwargs['acls'])

            # parent_job_id defaults to NULL but always serializing it as an int
            # fits the rdb's type assumptions. Note that job ids are 1 based.
            if kwargs['parent_job_id'] is None:
                kwargs['parent_job_id'] = 0
        except (KeyError, TypeError) as e:
            raise rdb_utils.RDBException('Creating %s requires args %s got %s' %
                    (self.__class__, self.template._fields, kwargs.keys()))
        super(AcquireHostRequest, self).__init__(**kwargs)


