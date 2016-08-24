# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import autotemp

class desktopui_FontCache(test.test):
    version = 1
    _mounted = False
    _new_cache = None
    _FONTCACHE = "/usr/share/cache/fontconfig"


    def _mount_cache(self):
        utils.system("mount -n --bind %s %s" % (self._new_cache.name,
                                                self._FONTCACHE))
        self._mounted = True

    def _unmount_cache(self):
        if self._mounted:
            utils.system("umount -n %s" % self._FONTCACHE)
            self._mounted = False

    def cleanup(self):
        self._unmount_cache()
        if self._new_cache:
            self._new_cache.clean()


    def run_once(self):
        self._new_cache = autotemp.tempdir(unique_id="new-font-cache")
        # Generate a new cache and compare it to the existing cache. Ideally, we
        # would simply point fc-cache to a new cache location, however, that
        # doesn't seem possible. So, just bind mount the existing cache location
        # out of rootfs temporarily.
        self._mount_cache()
        utils.system("fc-cache -fv")
        self._unmount_cache()
        utils.system("diff -qr %s %s" % (self._FONTCACHE, self._new_cache.name))
