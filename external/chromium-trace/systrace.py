#!/usr/bin/env python

# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

version = sys.version_info[:2]
if version != (2, 7):
  sys.stderr.write('Systrace does not support Python %d.%d. '
                   'Please use Python 2.7.\n' % version)
  sys.exit(1)

systrace_dir = os.path.abspath(
    os.path.join(os.path.dirname(__file__), 'catapult', 'systrace'))
sys.path.insert(0, systrace_dir)

from systrace import systrace

if __name__ == '__main__':
  sys.exit(systrace.main())
