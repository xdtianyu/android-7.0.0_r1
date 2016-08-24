# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

import ConfigParser

CONFIG_FILE = os.path.join(os.path.dirname(__file__), 'rpm_config.ini')
rpm_config = ConfigParser.SafeConfigParser()
rpm_config.read(CONFIG_FILE)