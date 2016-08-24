# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import os
import shutil
import tarfile
import tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_DebugDaemonDumpDebugLogs(test.test):
    version = 1

    def runDump(self, compressed):
        filename = 'compressed_dump.tgz' if compressed else 'uncompressed_dump.tar'
        tmp_file = os.path.join(self.tmp_dir, filename)
        try:
            fh = os.open(tmp_file, os.O_TRUNC | os.O_CREAT | os.O_WRONLY)
            self.iface.DumpDebugLogs(compressed, fh)
        except:
            raise
        finally:
            os.close(fh)

        mode = 'r:gz' if compressed else 'r:'
        with tarfile.open(tmp_file, mode) as tar_file:
            if len(tar_file.getmembers()) == 0:
                raise error.TestFail("%s log file list is empty." %
                       "compressed" if compressed else "uncompressed")


    def run_once(self, *args, **kwargs):
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.debugd', '/org/chromium/debugd')
        self.iface = dbus.Interface(proxy,
                                    dbus_interface='org.chromium.debugd')
        self.tmp_dir = tempfile.mkdtemp()
        self.runDump(True)
        self.runDump(False)
        if os.path.exists(self.tmp_dir):
            shutil.rmtree(self.tmp_dir)

