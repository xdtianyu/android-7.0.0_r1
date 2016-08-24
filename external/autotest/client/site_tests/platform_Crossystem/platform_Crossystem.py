# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class _Matcher(object):
    """Extends regular expression with a match/do not match bit and
    a saner definition of "match".
    """


    def __init__(self, pattern):

        self._pattern = pattern
        # If the pattern starts with !, it means "do not match".
        if pattern[0] == '!':
            self._positive_match = False
            pattern = pattern[1:]
        else:
            self._positive_match = True

        # re.match() forces the RE to match from the beginning, but doesn't
        # require that the RE matches the entire string, so wrap with ^$ even
        # though the ^ is not strictly needed.
        self._regexp = re.compile("^" + pattern + "$")


    def match(self, string):
        return bool(self._regexp.match(string)) == self._positive_match


_ALPHANUM = _Matcher("[\d\w]+")
_NUM = _Matcher("[\d]+")
_HEXNUM = _Matcher("0x[\da-fA-F]+")
_BIT = _Matcher("[01]")
_ANYTHING = _Matcher("!(\(error\))|")  # anything but "(error)" or ""

def check(var, matcher):
    """
    Runs "crossystem @var" and raises an error
    if the output does not match @matcher

    @param var: the name of a crossystem variable
    @param matcher: a matcher that must match the output of crossystem @var

    """
    output = utils.system_output("crossystem %s" % var).strip()
    if not matcher.match(output):
        raise error.TestFail("crossystem %s = \"%s\", does not match \"%s\"" %
                (var, output, matcher._pattern))


class platform_Crossystem(test.test):
    """See control file for doc"""
    version = 2

    def run_once(self):
        """Checks that crossystem works and returns plausible values for
        a set of variables that are implemented on all platforms.
        """

        for var, matcher in (
                ("arch", _ALPHANUM),
                ("cros_debug", _BIT),
                ("debug_build", _BIT),
                ("devsw_boot", _BIT),
                ("devsw_cur", _BIT),
                ("fwid", _ANYTHING),
                ("hwid", _ANYTHING),
                ("loc_idx", _NUM),
                ("mainfw_act", _ALPHANUM),
                ("mainfw_type", _ALPHANUM),
                ("ro_fwid", _ANYTHING),
                ("tpm_fwver", _HEXNUM),
                ("tpm_kernver", _HEXNUM),
                ("wpsw_boot", _BIT),
                ("wpsw_cur", _BIT),
        ):
            check(var, matcher)
