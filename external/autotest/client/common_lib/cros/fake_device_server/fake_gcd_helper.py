# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import uuid

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import process_watcher
from autotest_lib.client.common_lib.cros.fake_device_server.client_lib import \
        meta


class FakeGCDHelper(object):
    """Helper object that knows how to bring up and kill fake GCD instances."""

    def __init__(self, host=None):
        """Construct an instance.

        @param host: host object if the server should be started on a remote
                host.

        """
        self._generation = str(uuid.uuid1())
        self._process = process_watcher.ProcessWatcher(
                '/usr/local/autotest/common_lib/cros/'
                        'fake_device_server/server.py',
                args=(self._generation,),
                host=host)
        self._meta = meta.MetaClient()


    def start(self, timeout_seconds=30):
        """Start this instance and confirm that it is up.

        @param timeout_seconds: number of seconds to wait for server start.

        """
        self._process.start()
        start_time = time.time()
        while time.time() - start_time < timeout_seconds:
            received_generation = self._meta.get_generation()
            if self._generation == received_generation:
                return
            time.sleep(1)

        raise error.TestError('Failed to start fake GCD server.')


    def close(self):
        """Close this instance."""
        self._process.close()
