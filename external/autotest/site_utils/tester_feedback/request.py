# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Feedback request interface."""


class FeedbackRequestError(Exception):
    """An error during feedback request processing."""


class FeedbackRequest(object):
    """A abstract class for managing a single feedback request."""

    _TITLE_TEMPLATE = '%(desc)s request from %(dut)s (%(test)s)'

    def __init__(self, test, dut, desc):
        """Initializes the request object.

        @param test: The test name.
        @param dut: The DUT name.
        @param desc: A one-liner describing the essence of the request.
        """
        self.test = test
        self.dut = dut
        self.desc = desc


    def get_title(self):
        """Returns the request descriptive title.

        This method is used by the resuest multiplexer to obtain a list of
        pending request titles.

        @return: A short string describing the request and who made it.
        """
        return (self._TITLE_TEMPLATE %
                {'test': self.test, 'dut': self.dut, 'desc': self.desc})


    def execute(self):
        """Executes the feedback request.

        @return: The result of processing the request. This value varies
                 depending on the request type and is up to the invoking
                 delegate to evaluate and act upon.

        @raise FeedbackRequestError: Failed to execute the request.
        """
        raise NotImplementedError
