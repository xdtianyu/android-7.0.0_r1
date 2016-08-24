# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re, socket, subprocess
from autotest_lib.client.common_lib import error

class HostRoute(object):
    """
    Host Route: A utility for retrieving information about our route to a host

    """

    def __init__(self, host):
        self.host = host    # Remote host
        self.calculate()

    def calculate(self):
        output = self.run_command(["ip", "route", "get", self.host])
        # This converts "172.22.18.53 via 10.0.0.1 dev eth0 src 10.0.0.200 \n.."
        # into ("via", "10.0.0.1", "dev", "eth0", "src", "10.0.0.200")
        route_info = re.split("\s*", output.split("\n")[0].rstrip(' '))[1:]

        # Further, convert the list into a dict {"via": "10.0.0.1", ...}
        self.route_info = dict(tuple(route_info[i:i+2])
                               for i in range(0, len(route_info), 2))

        if 'src' not in self.route_info:
            raise error.TestFail('Cannot find route to host %s' % self.host)

class LocalHostRoute(HostRoute):
    """
    Self Host Route: Retrieve host route for the test-host machine

    """
    def __init__(self, host):
        # TODO(pstew): If we could depend on the host having the "ip" command
        # we would just be able to do this:
        #
        #     HostRoute.__init__(self, host)
        #
        # but alas, we can't depend on this, so we fake it by creating a
        # socket and figuring out what local address we bound to if we
        # connected to the client
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.connect((host, 22)) # NB: Port doesn't matter
        self.route_info = { 'src': sock.getsockname()[0] }

    def run_command(self, args):
        return subprocess.Popen(args, stdout=subprocess.PIPE).communicate()[0]

class RemoteHostRoute(HostRoute):
    """
    Remote Host Route: Retrieve host route for a remote (DUT, server) machine

    """
    def __init__(self, remote, host):
        self.remote = remote
        HostRoute.__init__(self, host)

    def run_command(self, args):
        return self.remote.run(' '.join(args)).stdout
