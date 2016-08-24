# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides utility methods for interacting with upstart"""

import os

from autotest_lib.client.common_lib import utils

def ensure_running(service_name):
    """Fails if |service_name| is not running.

    @param service_name: name of the service.
    """
    cmd = 'initctl status %s | grep start/running' % service_name
    utils.system(cmd)


def has_service(service_name):
    """Returns true if |service_name| is installed on the system.

    @param service_name: name of the service.
    """
    return os.path.exists('/etc/init/' + service_name + '.conf')
