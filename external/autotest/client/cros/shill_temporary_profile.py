# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class ShillTemporaryProfile(object):
    """Context enclosing the use of a temporary shill profile.  It takes
    a shill manager dbus object and profile name, and makes sure that
    this profile is pushed atop the topmost default profile for the duration
    of this object lifetime."""
    def __init__(self, manager, profile_name='test'):
        self._manager = manager
        self._profile_name = profile_name


    def __enter__(self):
        self._manager.PopAllUserProfiles()
        try:
            self._manager.RemoveProfile(self._profile_name)
        except:
            pass
        self._manager.CreateProfile(self._profile_name)
        self._manager.PushProfile(self._profile_name)
        return self


    def __exit__(self, exception, value, traceback):
        try:
            self._manager.PopProfile(self._profile_name)
            self._manager.RemoveProfile(self._profile_name)
        except:
            pass
