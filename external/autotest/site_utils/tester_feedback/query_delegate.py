# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Feedback query delegate interfaces and implementation registry."""

import multiprocessing

import common
from autotest_lib.client.common_lib.feedback import client


# Mapping of query identifiers to delegate classes.
_query_delegate_registry = {}


class _QueryDelegate(object):
    """A base class for query delegates."""

    _query_count = multiprocessing.Value('d', 0)

    def __init__(self, test, dut, multiplexer, atomic=True):
        """Constructs the delegate.

        @param test: The name of the test.
        @param dut: The name of the DUT.
        @param multiplexer: Feedback request multiplexer object.
        @param atomic: Whether this is an atomic query.
        """
        super(_QueryDelegate, self).__init__()
        self.test = test
        self.dut = dut
        self._multiplexer = multiplexer
        self._atomic = atomic

        # Assign a unique query number.
        with self._query_count.get_lock():
            self._query_num = self._query_count.value
            self._query_count.value += 1


    def _process_request(self, req):
        """Submits a given request to the multiplexer for processing."""
        return self._multiplexer.process_request(req, self._query_num,
                                                 self._atomic)


    def prepare(self, **kwargs):
        """Delegate for a query's prepare() method."""
        return self._prepare_impl(**kwargs)


    def _prepare_impl(self, **kwargs):
        """Concrete implementation of the query's prepare() call."""
        raise NotImplementedError


    def validate(self, **kwargs):
        """Delegate for a query's validate() method.

        This clears the atomic sequence with the multiplexer to make sure it
        isn't blocked waiting for more requests from this query.
        """
        try:
            return self._validate_impl(**kwargs)
        finally:
            if self._atomic:
                self._multiplexer.end_atomic_seq(self._query_num)


    def _validate_impl(self, **kwargs):
        """Concrete implementation of the query's validate() call."""
        raise NotImplementedError


class OutputQueryDelegate(_QueryDelegate):
    """A base class for output query delegates."""


class InputQueryDelegate(_QueryDelegate):
    """A base class for input query delegates."""

    def emit(self):
        """Delegate for an input query's emit() method."""
        return self._emit_impl()


    def _emit_impl(self):
        """Concrete implementation of the query's emit() call."""
        raise NotImplementedError


def register_delegate_cls(query_id, delegate_cls):
    """Registers a delegate class with a given query identifier.

    @param query_id: Query identifier constant.
    @param delegate_cls: The class implementing a delegate for this query.
    """
    _query_delegate_registry[query_id] = delegate_cls


def get_delegate_cls(query_id):
    """Returns a query delegate class for a given query type.

    @param query_id: A query type identifier.

    @return A query delegate class.

    @raise ValueError: Unknown query type.
    @raise NotImplementedError: Query type not supported.
    """
    if query_id not in client.ALL_QUERIES:
        raise ValueError
    if query_id not in _query_delegate_registry:
        raise NotImplementedError
    return _query_delegate_registry[query_id]
