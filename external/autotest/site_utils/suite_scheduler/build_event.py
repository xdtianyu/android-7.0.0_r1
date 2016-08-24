# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import priorities

import base_event, forgiving_config_parser, manifest_versions, task

class BuildEvent(base_event.BaseEvent):
    """Base class for events that come from the build system.

    For example, a new build completing or a new version of Chromium.

    @var _revision: The last git revision we checked for new build artifacts.
    """


    def __init__(self, keyword, manifest_versions, always_handle):
        """Constructor.

        @param keyword: the keyword/name of this event, e.g. nightly.
        @param manifest_versions: ManifestVersions instance to use for querying.
        @param always_handle: If True, make ShouldHandle() always return True.
        """
        super(BuildEvent, self).__init__(keyword, manifest_versions,
                                         always_handle)
        self._revision = None


    def Merge(self, to_merge):
        """Merge this event with to_merge, changing all mutable properties.

        self._revision REMAINS UNCHANGED.  That's part of this instance's
        identity.

        @param to_merge: A BuildEvent instance to merge into this isntance.
        """
        super(BuildEvent, self).Merge(to_merge)


    def Prepare(self):
        """Perform any one-time setup that must occur before [Should]Handle().

        Creates initial revision checkpoint.
        """
        self._revision = self._mv.GetCheckpoint()


    def ShouldHandle(self):
        """True if there's been a new successful build since |self._revision|

        @return True if there's been a new build, false otherwise.
        """
        if super(BuildEvent, self).ShouldHandle():
            return True
        else:
            return self._mv.AnyManifestsSinceRev(self._revision)


    def _AllPerBranchBuildsSince(self, board, revision):
        """Get all per-branch, per-board builds since git |revision|.

        @param board: the board whose builds we want.
        @param revision: the revision to look back until.
        @return {branch: [build-name1, build-name2]}
        """
        all_branch_manifests = self._mv.ManifestsSinceRev(revision, board)
        all_branch_builds = {}
        for (type, milestone), manifests in all_branch_manifests.iteritems():
            branch_name = task.PickBranchName(type, milestone)
            for manifest in manifests:
                build = base_event.BuildName(board, type, milestone, manifest)
                all_branch_builds.setdefault(branch_name, []).append(build)
        return all_branch_builds


    def GetBranchBuildsForBoard(self, board):
        return self._AllPerBranchBuildsSince(board, self._revision)


class NewBuild(BuildEvent):
    KEYWORD = 'new_build'
    PRIORITY = priorities.Priority.POSTBUILD
    TIMEOUT = 12  # 12 hours, and builds come out every 6


    def __init__(self, mv, always_handle):
        """Constructor.

        @param mv: ManifestVersions instance to use for querying.
        @param always_handle: If True, make ShouldHandle() always return True.
        """
        super(NewBuild, self).__init__(self.KEYWORD, mv, always_handle)


    def UpdateCriteria(self):
        self._revision = self._mv.GetCheckpoint()
