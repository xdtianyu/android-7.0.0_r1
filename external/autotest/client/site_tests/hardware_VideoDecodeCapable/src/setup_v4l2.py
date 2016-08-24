# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to set up v4l2 extension module.
"""
from distutils.core import setup, Extension

setup (name = 'v4l2',
       version = '1.0',
       ext_modules = [Extension('v4l2', ['v4l2module.cc'])])
