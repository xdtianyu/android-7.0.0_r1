# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manifest of settings for the test lab created via clusterctl.

This module contains datastructures clusterctl uses to create a test cluster:
1. Ports:
    - master_afe_port: The port on which the master afe is listening
            for heartbeats. Autocorrection of ports on collision is
            currently an experimental feature, so freeing up the
            specified port is more reliable.
    - shards_base_port: The base port from which to assign ports to
            shards. The afe on shards does not __need__ to be exposed.
            It is useful for debugging, and a link to their afes will
            show up on the host page of hosts sent to shards on the
            master frontend.
    - vm_host_name: Prepended to the specified ports to discover
            cluster services. For example, with a hostname like
            'abc' the shards will do their heartbeat against
            'abc:master_afe_port'.
2. Shards: A list of boards for which to create shards. Note
    that currently to add a new shard you will have to perform 3 steps:
    - Add a new shard to this list
    - Copy the existing shard section in the ClusterTemplate
    - Pass num_shards=num_shards+1 to clusterctl
    Automating this process is a wip.
"""

# The port on which the master afe appears on.
master_afe_port = 8001
# Shards will have their afes listening on base_port + shard number.
shards_base_port = 8003
# Hostname of the vm host (generally your desktop).
vm_host_name = 'localhost'
# Boards for which to create shards.
shards = ['board:stumpy']
