#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import autotest_lib.server.frontend as frontend
from autotest_lib.frontend.afe import site_rpc_interface
from autotest_lib.frontend.afe import rpc_interface

class directAFE(frontend.AFE):
    """
    A wrapper for frontend.AFE which exposes all of the AFE
    functionality, but makes direct calls to site_rpc_interface and
    rpc_interface rather than making RPC calls to an RPC server.
    """
    def run(self, call, **dargs):
        func = None

        try:
            func = rpc_interface.__getattribute__(call)
        except AttributeError:
            pass

        try:
            func = site_rpc_interface.__getattribute__(call)
        except AttributeError:
            pass

        if not func:
            raise AttributeError('No function named %s in either '
                                 'rpc_interface or site_rpc_interface' % call)

        return func(**dargs)