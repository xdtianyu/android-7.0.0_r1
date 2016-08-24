# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import utils
from autotest_lib.client.cros import storage as storage_mod
from autotest_lib.client.common_lib import autotemp, error


class hardware_UsbBasicFileOperations(storage_mod.StorageTester):
    version = 1
    preserve_srcdir = True
    _src, _dst = None, None


    def run_once(self, volume_filter={'bus':'usb'}):
        storage = self.wait_for_device(volume_filter, cycles=1,
                                       mount_volume=True)[0]
        mount_point = storage['mountpoint']

        # -> Megabytes
        size = 1*1024*1024

        self._src = autotemp.tempfile(unique_id='tmpfile',
                                      dir=mount_point)
        self._dst = autotemp.tempfile(unique_id='autotest',
                                      dir=self.tmpdir)
        # Step 1: check if file creation works
        try:
            storage_mod.create_file(self._src.name, size)
        except error.CmdError, e:
            msg = ('fatal error occurred during file creation: '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)

        # not part of current check, remember the value for later use
        src_md5 = storage_mod.checksum_file(self._src.name)

        # Step 2: check if open works
        try:
            f = open(self._src.name, 'rb')
        except Exception, e:
            msg = ('fatal error occurred during open(): '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)

        try:
            f.read()
        except Exception, e:
            msg = ('fatal error occurred during read(): '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)

        try:
            f.close()
        except Exception, e:
            msg = ('fatal error occurred during close(): '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)


        # Step 3: check if file copy works
        try:
            utils.force_copy(self._src.name, self._dst.name)
        except Exception, e:
            msg = ('fatal error occurred during a file copy: '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)

        if src_md5 != storage_mod.checksum_file(self._dst.name):
            msg = ('fatal error occurred during a file copy, '
                   'md5 from origin and from destination are different: '
                   'basic file operation failed')
            raise error.TestFail(msg)


        # Step 4: check if file removal works
        try:
            os.remove(self._src.name)
        except OSError, e:
            msg = ('fatal error occurred during file removal: '
                   'basic file operation failed: %s' % e)
            raise error.TestFail(msg)

        if os.path.isfile(self._src.name):
            msg = ('fatal error occurred during file removal: '
                   'file still present after command, '
                   'basic file operation failed')
            raise error.TestFail(msg)

        utils.drop_caches()

        if os.path.isfile(self._src.name):
            msg = ('fatal error occurred during file removal: '
                   'file still present after command issued and '
                   'disk cached flushed), '
                   'basic file operation failed')
            raise error.TestFail(msg)

        # Step 5: check if modification to a file are persistent
        # copy file, modify src and modify dst the same way, checksum
        storage_mod.create_file(self._src.name, size)
        utils.force_copy(self._src.name, self._dst.name)

        # apply the same change to both files (which are identical in origin)
        src_md5 = modify_file(self._src.name)
        dst_md5 = modify_file(self._dst.name)

        # both copy of they file have to be the same
        if src_md5 != dst_md5:
            msg = ('fatal error occurred after modifying src and dst: '
                   'md5 checksums differ - %s / %s ,'
                   'basic file operation failed' % (src_md5, dst_md5))
            raise error.TestFail(msg)


    def cleanup(self):
        if self._src:
            self._src.clean()
        if self._dst:
            self._dst.clean()

        self.scanner.unmount_all()

        super(hardware_UsbBasicFileOperations, self).cleanup()


def modify_file(path):
    '''Modify a file returning its new MD5

    Open |path|, change a byte within the file and return the new md5.

    The change applied to the file is based on the file content and size.
    This means that identical files will result in identical changes and thus
    will return the same MD5.

    @param path: a path to the file to be modified
    @return the MD5 of |path| after the modification
    '''
    position = os.path.getsize(path) / 2

    # modify the file means: read a char, increase its value and write it back
    # given the same file (identical in size and bytes) it will apply the same
    # change
    f = open(path, 'r+b')
    f.seek(position)
    c = f.read(1)
    f.seek(position)
    f.write(chr(ord(c)+1))
    f.close()
    return storage_mod.checksum_file(path)
