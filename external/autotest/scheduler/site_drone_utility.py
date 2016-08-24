# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging, signal

from autotest_lib.client.common_lib import error, utils

# Override default parser with our site parser.
# This is coordinated with site_monitor_db.py.
def check_parse(process_info):
    return process_info['comm'] == 'site_parse'
