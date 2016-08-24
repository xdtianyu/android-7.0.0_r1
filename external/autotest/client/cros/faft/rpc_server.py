#!/usr/bin/python -u
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Exposes the FAFTClient interface over XMLRPC.

It launches a XMLRPC server and exposes the functions in RPCFunctions().
"""

from datetime import datetime
from optparse import OptionParser
from SimpleXMLRPCServer import SimpleXMLRPCServer

import common
from autotest_lib.client.cros.faft.rpc_functions import RPCFunctions


def main():
    """The Main program, to run the XMLRPC server."""
    parser = OptionParser(usage='Usage: %prog [options]')
    parser.add_option('--port', type='int', dest='port', default=9990,
                      help='port number of XMLRPC server')
    (options, _) = parser.parse_args()

    print '[%s] XMLRPC Server: Spinning up FAFT server' % str(datetime.now())
    # Launch the XMLRPC server to provide FAFTClient commands.
    server = SimpleXMLRPCServer(('localhost', options.port), allow_none=True,
                                logRequests=True)
    server.register_introspection_functions()
    server.register_instance(RPCFunctions())
    print '[%s] XMLRPC Server: Serving FAFT functions on port %s' % (
        str(datetime.now()), options.port)

    server.serve_forever()


if __name__ == '__main__':
    main()
