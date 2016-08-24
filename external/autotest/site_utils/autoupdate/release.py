# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Modules for obtaining Chrome OS release info."""


import ConfigParser
import bisect
import os


_RELEASE_CONFIG_FILE = os.path.join(os.path.dirname(__file__),
                                    'release_config.ini')

# Prefix for brachpoint definitions in the config file.
_CONF_BRANCH_SECTION = 'BRANCH'
_CONF_BRANCH_POINTS_OPT = 'branch_points'
_CONF_BRANCH_POINT_OPT_PREFIX = 'bp_'
_CONF_NEXT_BRANCH_OPT = 'next_branch'


class ReleaseError(BaseException):
    """Errors related to release and branch inference."""
    pass


class ReleaseInfo(object):
    """Provides reference information about Chrome OS releases.

    Currently, this class serves for mapping between releases and branches /
    release milestones. The information lives in a .ini file at the current
    directory, which has a single section [BRANCH] containing

      branch_points: comma-separated list of release branches (e.g. R10, R11,
      ...)

      bp_XYZ: for each branch listed above, a variable that maps to the Chrome
      OS release at that branchpoint (e.g. bp_r10: 0.10.156.0). Note that .ini
      file variables are case-insensitive.

      next_branch: the name of the current (unforked) branch (e.g. R24)

    It is also worth noting that a branch point X.Y.Z (alternatively, W.X.Y.Z)
    of some branch R denotes the build number X (repsectively, W) that
    constitutes the said branch. Therefore, it is only from build X+1 (W+1) and
    onward that releases will be tagged with R+1.

    """
    def __init__(self):
        self._release_config = None
        self._branchpoint_dict = None
        self._next_branch = None
        self._sorted_branchpoint_list = None
        self._sorted_shifted_branchpoint_rel_key_list = None

    def initialize(self):
        """Read release config and initialize lookup data structures."""
        self._release_config = ConfigParser.ConfigParser()
        try:
            self._release_config.readfp(open(_RELEASE_CONFIG_FILE))

            # Build branchpoint dictionary.
            branchpoint_list_str = self._release_config.get(
                    _CONF_BRANCH_SECTION, _CONF_BRANCH_POINTS_OPT)
            if branchpoint_list_str:
                branchpoint_list = map(str.strip,
                                       branchpoint_list_str.split(','))
            else:
                branchpoint_list = []

            self._branchpoint_dict = {}
            for branchpoint in branchpoint_list:
                self._branchpoint_dict[branchpoint] = (
                          self._release_config.get(
                                  _CONF_BRANCH_SECTION,
                                  _CONF_BRANCH_POINT_OPT_PREFIX + branchpoint))

            # Get next branch name.
            self._next_branch = self._release_config.get(_CONF_BRANCH_SECTION,
                                                         _CONF_NEXT_BRANCH_OPT)
            if not self._next_branch:
                raise ReleaseError("missing `%s' option" %
                                   _CONF_NEXT_BRANCH_OPT)
        except IOError, e:
            raise ReleaseError('failed to open release config file (%s): %s' %
                               (_RELEASE_CONFIG_FILE, e))
        except ConfigParser.Error, e:
            raise ReleaseError('failed to load release config: %s' % e)

        # Infer chronologically sorted list of branchpoints.
        self._sorted_branchpoint_list = self._branchpoint_dict.items()
        self._sorted_branchpoint_list.append((self._next_branch, '99999.0.0'))
        self._sorted_branchpoint_list.sort(
                key=lambda (branch, release): self._release_key(release))

        # Also store a sorted list of branchpoint release keys, for easy lookup.
        self._sorted_shifted_branchpoint_rel_key_list = [
                self._release_key(self._next_build_number_release(release))
                for (branch, release) in self._sorted_branchpoint_list]


    def _next_build_number_release(self, release):
        """Returns the release of the next build following a given release.

        Given a release number 'X.Y.Z' (new scheme) or '0.X.Y.Z' (old scheme)
        it will return 'X+1.0.0' or '0.X+1.0.0', respectively.

        @param release: the release number in dotted notation (string)

        @return The release number of the next build.

        @raise ReleaseError if the release is malformed.

        """
        release_components = release.split('.')
        if len(release_components) == 4 and release_components[0] == '0':
            prepend = '0.'
            x = int(release_components[1])
        elif len(release_components) != 3:
            raise ReleaseError('invalid release number: %s' % release)
        else:
            prepend = ''
            x = int(release_components[0])

        return '%s%s.0.0' % (prepend, x + 1)


    def _release_key(self, release):
        """Convert a Chrome OS release string into an integer key.

        This translates a release string 'X.Y.Z' (new scheme) or 'W.X.Y.Z' (old
        scheme where W = 0) into an integer whose value equals X * 10^7 + Y *
        10^3 + Z, assuming that Y < 10^4 and Z < 10^3, and will scale safely to
        any foreseeable major release number (X).

        @param release: the release number in dotted notation (string)

        @return A unique integer key representing the release.

        @raise ReleaseError if the release is malformed.

        """
        release_components = release.split('.')
        if len(release_components) == 4 and release_components[0] == '0':
            release_components = release_components[1:]
        elif len(release_components) != 3:
            raise ReleaseError('invalid release number: %s' % release)
        x, y, z = [int(s) for s in release_components]
        return x * 10000000 + y * 1000 + z


    def get_branch_list(self):
        """Retruns chronologically sorted list of branch names."""
        return [branch for (branch, release) in self._sorted_branchpoint_list]


    def get_branch(self, release):
        """Returns the branch name of a given release version. """
        i = bisect.bisect_left(self._sorted_shifted_branchpoint_rel_key_list,
                               self._release_key(release))
        return self._sorted_branchpoint_list[i][0] if i else None


    def get_branchpoint_release(self, branch):
        """Returns the branchpoint release of a given branch.

        Returns None if given name is the next branch.

        @raise KeyError if branch name not known

        """
        if branch == self._next_branch:
            return None
        return self._branchpoint_dict[branch]
