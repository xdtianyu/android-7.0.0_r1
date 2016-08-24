# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tester feedback request multiplexer."""

from multiprocessing import reduction
import Queue
import collections
import multiprocessing
import os
import sys

import common
from autotest_lib.client.common_lib.feedback import tester_feedback_client

import input_handlers
import request
import sequenced_request


ReqTuple = collections.namedtuple(
        'ReqTuple', ('obj', 'reduced_reply_pipe', 'query_num', 'atomic'))


class FeedbackRequestMultiplexer(object):
    """A feedback request multiplexer."""

    class RequestProcessingTerminated(Exception):
        """User internally to signal processor termination."""


    def __init__(self):
        self._request_queue = multiprocessing.Queue()
        self._pending = []
        self._request_handling_process = None
        self._running = False
        self._atomic_seq = None


    def _dequeue_request(self, block=False):
        try:
            req_tuple = self._request_queue.get(block=block)
        except Queue.Empty:
            return False

        if req_tuple is None:
            raise self.RequestProcessingTerminated
        self._pending.append(req_tuple)
        return True


    def _atomic_seq_cont(self):
        """Returns index of next pending request in atomic sequence, if any."""
        for req_idx, req_tuple in enumerate(self._pending):
            if req_tuple.query_num == self._atomic_seq:
                return req_idx


    def _handle_requests(self, stdin):
        """Processes feedback requests until termination is signaled.

        This method is run in a separate process and needs to override stdin in
        order for raw_input() to work.
        """
        sys.stdin = stdin
        try:
            while True:
                req_idx = None

                # Wait for a (suitable) request to become available.
                while True:
                    if self._atomic_seq is None:
                        if self._pending:
                            break
                    else:
                        req_idx = self._atomic_seq_cont()
                        if req_idx is not None:
                            break
                    self._dequeue_request(block=True)

                # If no request was pre-selected, prompt the user to choose one.
                if req_idx is None:
                    raw_input('Pending feedback requests, hit Enter to '
                              'process... ')

                    # Pull all remaining queued requests.
                    while self._dequeue_request():
                        pass

                    # Select the request to process.
                    if len(self._pending) == 1:
                        print('Processing: %s' %
                              self._pending[0].obj.get_title())
                        req_idx = 0
                    else:
                        choose_req = sequenced_request.SequencedFeedbackRequest(
                                None, None, None)
                        choose_req.append_question(
                                'List of pending feedback requests:',
                                input_handlers.MultipleChoiceInputHandler(
                                        [req_tuple.obj.get_title()
                                         for req_tuple in self._pending],
                                        default=1),
                                prompt='Choose a request to process')
                        req_idx, _ = choose_req.execute()

                # Pop and handle selected request, then close pipe.
                req_tuple = self._pending.pop(req_idx)
                if req_tuple.obj is not None:
                    try:
                        ret = req_tuple.obj.execute()
                    except request.FeedbackRequestError as e:
                        ret = (tester_feedback_client.QUERY_RET_ERROR, str(e))
                    reply_pipe = req_tuple.reduced_reply_pipe[0](
                            *req_tuple.reduced_reply_pipe[1])
                    reply_pipe.send(ret)
                    reply_pipe.close()

                # Set the atomic sequence if so instructed.
                self._atomic_seq = (req_tuple.query_num if req_tuple.atomic
                                    else None)

        except self.RequestProcessingTerminated:
            pass


    def start(self):
        """Starts the request multiplexer."""
        if self._running:
            return

        dup_stdin = os.fdopen(os.dup(sys.stdin.fileno()))
        self._request_handling_process = multiprocessing.Process(
                target=self._handle_requests, args=(dup_stdin,))
        self._request_handling_process.start()

        self._running = True


    def stop(self):
        """Stops the request multiplexer."""
        if not self._running:
            return

        # Tell the request handler to quit.
        self._request_queue.put(None)
        self._request_handling_process.join()

        self._running = False


    def process_request(self, request, query_num, atomic):
        """Processes a feedback requests and returns its result.

        This call is used by queries for submitting individual requests. It is
        a blocking call that should be called from a separate execution thread.

        @param request: The feedback request to process.
        @param query_num: The unique query number.
        @param atomic: Whether subsequent request(s) are expected and should be
                       processed without interruption.
        """
        reply_pipe_send, reply_pipe_recv = multiprocessing.Pipe()
        reduced_reply_pipe_send = reduction.reduce_connection(reply_pipe_send)
        self._request_queue.put(ReqTuple(request, reduced_reply_pipe_send,
                                         query_num, atomic))
        return reply_pipe_recv.recv()


    def end_atomic_seq(self, query_num):
        """Ends the current atomic sequence.

        This enqueues a null request with the given query_num and atomicity set
        to False, causing the multiplexer to terminate the atomic sequence.

        @param query_num: The unique query number.
        """
        self._request_queue.put(ReqTuple(None, None, query_num, False))
