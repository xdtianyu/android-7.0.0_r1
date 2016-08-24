# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import tempfile

# Parent passes base path as first argument.
child_path = os.path.join(sys.argv[1], "mountns-enter-child.py")

# Mount tmpfs.
tmpdir = tempfile.mkdtemp(prefix="newns-", dir="/tmp")
ret = subprocess.check_call(["mount", "tmpfs", tmpdir, "-t", "tmpfs"])
test_file = os.path.join(tmpdir, "test")
with open(test_file, "w") as t:
    print >> t, "test"

# Exec child and enter existing mount namespace.
ret = subprocess.call(["/sbin/minijail0", "-V", "/proc/1/ns/mnt", "--",
                       sys.executable, child_path, test_file])

# Clean up.
subprocess.check_call("umount %s" % tmpdir, shell=True)
os.rmdir(tmpdir)

# Return child's exit status.
sys.exit(ret)
