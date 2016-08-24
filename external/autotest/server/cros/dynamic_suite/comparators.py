# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Comparators for use in dynamic_suite module unit tests."""

import mox

class StatusContains(mox.Comparator):
    @staticmethod
    def CreateFromStrings(status=None, test_name=None, reason=None):
        status_comp = mox.StrContains(status) if status else mox.IgnoreArg()
        name_comp = mox.StrContains(test_name) if test_name else mox.IgnoreArg()
        reason_comp = mox.StrContains(reason) if reason else mox.IgnoreArg()
        return StatusContains(status_comp, name_comp, reason_comp)


    def __init__(self, status=mox.IgnoreArg(), test_name=mox.IgnoreArg(),
                 reason=mox.IgnoreArg()):
        """Initialize.

        Takes mox.Comparator objects to apply to job_status.Status
        member variables.

        @param status: status code, e.g. 'INFO', 'START', etc.
        @param test_name: expected test name.
        @param reason: expected reason
        """
        self._status = status
        self._test_name = test_name
        self._reason = reason


    def equals(self, rhs):
        """Check to see if fields match base_job.status_log_entry obj in rhs.

        @param rhs: base_job.status_log_entry object to match.
        @return boolean
        """
        return (self._status.equals(rhs.status_code) and
                self._test_name.equals(rhs.operation) and
                self._reason.equals(rhs.message))


    def __repr__(self):
        return '<Status containing \'%s\t%s\t%s\'>' % (self._status,
                                                       self._test_name,
                                                       self._reason)


class InHostList(mox.Comparator):
    """PyMox comparator that matches given Host.hostname to a stored list."""
    def __init__(self, host_list):
        self._hostname_list = [h.hostname for h in host_list]


    def equals(self, rhs):
        """Check to see if rhs.hostname is in stored hostname list.

        @param rhs: Host object to match.
        @return boolean
        """
        return rhs.hostname in self._hostname_list


class AllInHostList(mox.Comparator):
    """PyMox comparator that matches a list of Host base on hostname.

    Matches if, for each Host to be matched, its hostname is in the list of
    hostnames provided at initialization.
    """
    def __init__(self, host_list):
        self._in_host_list = InHostList(host_list)


    def equals(self, rhs):
        """Check to see if all hostnames in rhs are in stored hostname list.

        @param rhs: iterable of Host objects to match.
        @return False if rhs is []; otherwise checks all objects in rhs.
        """
        tests = map(self._in_host_list.equals, rhs)
        if tests:
            return reduce(lambda a,b: a and b, tests, True)
        return False
