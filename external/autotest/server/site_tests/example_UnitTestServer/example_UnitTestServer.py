# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server.bin import unit_test_server

class example_UnitTestServer(unit_test_server.unit_test_server):
    version = 1
    client_test = 'example_UnitTest'
    test_files = ['main.cc']
