#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script adds desired tests to ChromeOS Autotest server.

pushd $(dirname "$0") > /dev/null
# Add all site_tests
./test_importer.py -t /usr/local/autotest/client/site_tests
./test_importer.py -t /usr/local/autotest/server/site_tests

# Add all profilers
./test_importer.py -p /usr/local/autotest/client/profilers

# Add selected tests
./test_importer.py -w site_whitelist_tests
popd > /dev/null
