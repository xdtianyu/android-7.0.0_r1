# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tester feedback delegate."""

import logging
import xmlrpclib

import common
from autotest_lib.client.common_lib.feedback import tester_feedback_client

import query_delegate


class FeedbackDelegate(object):
    """An object for managing feedback RPC calls."""

    def __init__(self, multiplexer):
        self._multiplexer = multiplexer
        self._clients = {}


    def _get_client(self, client_id):
        """Returns the query dictionary for a client.

        @param client_id: The client identifier.

        @return: A dictionary mapping registered query numbers to query delegate
                 objects for the given client.

        @raise xmlrpclib.Fault: The client was not registered.
        """
        if client_id not in self._clients:
            raise xmlrpclib.Fault('Unknown client (%s)' % client_id)
        return self._clients[client_id]


    def _get_delegate_cls(self, query_id):
        """Returns a query delegate class for a given query type.

        @param query_id: The query type for which a delegate is needed.

        @return: A query delegate class.

        @raise xmlrpclib.Fault: Query type is invalid or unsupported.
        """
        try:
            return query_delegate.get_delegate_cls(query_id)
        except ValueError:
            raise xmlrpclib.Fault('Unknown query type (%s)' % query_id)
        except NotImplementedError:
            raise xmlrpclib.Fault('Unsupported query type (%s)' % query_id)


    def new_client(self, client_id):
        """Register a new client.

        A client identifier is unique for a given test and DUT: at any given
        time, there's only one test that is using this identifier. That said,
        a client identifier may be reused across different tests at different
        times within the lifetime of the feedback delegate. In general, clients
        are expected to unregister when they finish running. However, for the
        delegate to be resilient to test crashes, we forgo this requirement and
        only emit a warning.

        @param client_id: The client identifier.

        @return: True (avoiding None with XML-RPC).
        """
        if client_id in self._clients:
            logging.warning('Overwriting existing client entry %s; prior '
                            'instance did not shutdown properly?', client_id)
        self._clients[client_id] = {}
        return True


    def delete_client(self, client_id):
        """Unregister a client.

        @param client_id: The client identifier.

        @return: True (avoiding None with XML-RPC).
        """
        del self._clients[client_id]
        return True


    def new_query(self, client_id, query_id, query_num):
        """Register a new query from a client.

        @param client_id: The client identifier.
        @param query_id: The query type.
        @param query_num: The query's unique number.

        @return: True (avoiding None with XML-RPC).

        @raise xmlrpclib.Fault: The client or query arguments are invalid.
        """
        client = self._get_client(client_id)
        if query_num in client:
            raise xmlrpclib.Fault('New query (%s) is already registered' %
                                  query_num)
        test_name, dut_name = client_id.split(':')
        client[query_num] = self._get_delegate_cls(query_id)(
                test_name, dut_name, self._multiplexer)
        return True


    def query_call(self, client_id, query_num, query_method, kwargs_dict):
        """Perform a query call.

        @param client_id: The client identifier.
        @param query_num: The query unique number.
        @param query_method: The method being called.
        @param kwargs_dict: Extra arguments being passed to the method call.

        @return: A pair containing a method return code (constant defined in
                 tester_feedback_client) and a description of the result
                 (string).

        @raise: xmlrpclib.Fault: Method execution failed.
        """
        try:
            query = self._get_client(client_id)[query_num]
        except KeyError:
            raise xmlrpclib.Fault('Query %d unknown to client %s' %
                                  (query_num, client_id))

        # Route the query call to the appropriate method.
        local_method = getattr(query, query_method, None)
        if local_method is None:
            ret = (tester_feedback_client.QUERY_RET_ERROR,
                   'Unknown query method (%s)' % query_method)
        else:
            ret = local_method(**kwargs_dict)

        # If there's an explicit result, return it; otherwise, return success.
        if ret is None:
            return tester_feedback_client.QUERY_RET_SUCCESS, ''
        return ret
