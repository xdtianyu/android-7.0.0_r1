# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Config(object):
    """Client side services config. Accessible by server side code as well."""

    # RPC server that runs on the DUT.
    rpc_port = 9990
    rpc_command = '/usr/local/autotest/cros/faft/rpc_server.py'
    rpc_command_short = 'rpc_server'
    rpc_ready_call = 'system.is_available'
    rpc_timeout = 20
    rpc_logfile = '/var/log/faft_xmlrpc_server.log'
