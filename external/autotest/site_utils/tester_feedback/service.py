# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tester feedback backend service implementation."""

import SimpleXMLRPCServer
import multiprocessing

import feedback_delegate
import request_multiplexer

# TODO(garnold) Load query delegate implementations as they become available
# (b/26769927).
# pylint: disable=unused-import
import audio_query_delegate_impl


class FeedbackService(object):
    """The feedback service main object."""

    def __init__(self):
        self._multiplexer = None
        self._server_port = 0
        self._server_process = None
        self._running = False


    @property
    def server_port(self):
        """Returns the service listening port."""
        return self._server_port


    def start(self):
        """Starts the feedback service."""
        if self._running:
            return

        # Start the feedback request multiplexer.
        self._multiplexer = request_multiplexer.FeedbackRequestMultiplexer()
        self._multiplexer.start()

        # Start the feedback delegate RPC server.
        rpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer(('localhost', 0))
        rpc_server.register_instance(
                feedback_delegate.FeedbackDelegate(self._multiplexer))
        self._server_port = rpc_server.server_address[1]
        self._server_process = multiprocessing.Process(
                target=rpc_server.serve_forever)
        self._server_process.start()

        self._running = True


    def stop(self):
        """Stops the feedback service."""
        if not self._running:
            return

        # Stop the RPC server.
        self._server_process.terminate()
        self._server_process.join()
        self._server_port = 0

        # Stop the multiplexer.
        self._multiplexer.stop()

        self._running = False
