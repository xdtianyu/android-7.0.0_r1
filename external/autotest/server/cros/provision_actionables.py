# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This file defines tasks to be executed in provision actions."""

import abc
import logging

import common


class BaseActionable(object):
    """Base class of an actionable item."""

    @abc.abstractmethod
    def execute(self, job, host, *args, **kwargs):
        """Execute the action item.

        @param job: A job object from a control file.
        @param host: A host to run this action against.
        @param args: arguments to passed to the test.
        @param kwargs: keyword arguments to passed to the test.

        @returns True if succeeds, False otherwise,
                 subclass should override this method.
        """
        raise NotImplementedError('Subclass should override execute.')


class TestActionable(BaseActionable):
    """A test to be executed as an action"""

    def __init__(self, test, extra_kwargs={}):
        """Init method.

        @param test: String, the test to run, e.g. dummy_PassServer
        @param extra_kargs: A dictionary, extra keyval-based args
                            that will be passed when execute the test.
        """
        self.test = test
        self.extra_kwargs = extra_kwargs


    def execute(self, job, host, *args, **kwargs):
        """Execute the action item.

        @param job: A job object from a control file.
        @param host: A host to run this action against.
        @param args: arguments to passed to the test.
        @param kwargs: keyword arguments to passed to the test.

        @returns True if succeeds, False otherwise.
        """
        kwargs.update(self.extra_kwargs)
        return job.run_test(self.test, host=host, *args, **kwargs)


class RebootActionable(BaseActionable):
    """Reboot action."""

    def execute(self, job, host, *args, **kwargs):
        """Execute the action item.

        @param job: A job object from a control file.
        @param host: A host to run this action against.
        @param args: arguments to passed to the test.
        @param kwargs: keyword arguments to passed to the test.

        @returns True if succeeds.
        """
        logging.error('Executing RebootActionable ... ')
        host.reboot()
        logging.error('RebootActionable execution succeeds. ')
        return True
