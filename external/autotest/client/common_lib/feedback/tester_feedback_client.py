# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Feedback client implementation for interacting with a human tester."""

import xmlrpclib

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.feedback import client


# Query return codes.
#
QUERY_RET_SUCCESS = 0
QUERY_RET_FAIL = 1
QUERY_RET_ERROR = 2


class Client(client.Client):
    """Human tester feedback implementation."""

    def __init__(self, test_name, dut_name, remote_addr):
        """Constructs the client object.

        @param test_name: The name of the test.
        @param dut_name: The name of the DUT.
        @param remote_addr: The 'name:port' of the remote feedback service host.
        """
        super(Client, self).__init__()
        self._client_id = '%s:%s' % (test_name, dut_name)
        self._remote_addr = remote_addr
        self._query_num = 0
        self._rpc_proxy = None


    def _make_query_call(self, query_num, query_method, **kwargs):
        """Make an RPC query call (used by query objects).

        @param query_num: The unique query identifying number.
        @param query_method: The query method being called.

        @raise xmlrpclib.Error: An error during RPC call processing.
        """
        # XML-RPC does not support kwargs, so we just pass it as a dictionary.
        return self._rpc_proxy.query_call(self._client_id, query_num,
                                          query_method, kwargs)


    # Interface overrides.
    #
    def _initialize_impl(self, _test, _host):
        """Initializes the feedback object.

        Initializes an XML-RPC proxy and registers the client at the remote end.

        @param _test: An object representing the test case (unused).
        @param _host: An object representing the DUT (unused).
        """
        self._rpc_proxy = xmlrpclib.ServerProxy('http://%s' % self._remote_addr)
        try:
            self._rpc_proxy.new_client(self._client_id)
        except xmlrpclib.Error as e:
            raise error.TestError('Feedback client registration error: %s' % e)


    def _new_query_impl(self, query_id):
        """Instantiates a new query.

        @param query_id: A query identifier.

        @return A query object.
        """
        if query_id in client.INPUT_QUERIES:
            query_cls = InputQuery
        elif query_id in client.OUTPUT_QUERIES:
            query_cls = OutputQuery
        else:
            raise error.TestError('Unknown query (%s)' % query_id)

        # Create, register and return a new query.
        self._query_num += 1
        try:
            self._rpc_proxy.new_query(self._client_id, query_id, self._query_num)
        except xmlrpclib.Error as e:
            raise error.TestError('Feedback query registration error: %s' % e)
        return query_cls(self, self._query_num)


    def _finalize_impl(self):
        """Finalizes the feedback object."""
        try:
            self._rpc_proxy.delete_client(self._client_id)
        except xmlrpclib.Error as e:
            raise error.TestError(
                    'Feedback client deregistration error: %s' % e)


class _Query(object):
    """Human tester feedback query base class."""

    def __init__(self, client, query_num):
        super(_Query, self).__init__()
        self.client = client
        self.query_num = query_num


    def _make_query_call(self, query_method, **kwargs):
        try:
            ret, desc = self.client._make_query_call(self.query_num,
                                                     query_method, **kwargs)
        except xmlrpclib.Error as e:
            ret, desc = QUERY_RET_ERROR, str(e)

        if ret == QUERY_RET_SUCCESS:
            return
        if ret == QUERY_RET_FAIL:
            raise error.TestFail('Tester feedback request failed: %s' % desc)
        if ret == QUERY_RET_ERROR:
            raise error.TestError('Tester feedback request error: %s' % desc)
        raise error.TestError('Unknown feedback call return code (%s)' % ret)


    # Interface overrides.
    #
    def _prepare_impl(self, **kwargs):
        self._make_query_call('prepare', **kwargs)


    def _validate_impl(self, **kwargs):
        self._make_query_call('validate', **kwargs)


class OutputQuery(_Query, client.OutputQuery):
    """Human tester feedback output query."""

    def __init__(self, client, query_num):
        super(OutputQuery, self).__init__(client, query_num)


class InputQuery(_Query, client.InputQuery):
    """Human tester feedback input query."""

    def __init__(self, client, query_num):
        super(InputQuery, self).__init__(client, query_num)


    # Interface override.
    #
    def _emit_impl(self):
        self._make_query_call('emit')
