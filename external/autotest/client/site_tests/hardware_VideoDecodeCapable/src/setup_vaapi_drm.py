# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to set up vaapi extension module.
"""
from distutils.core import setup, Extension

module = Extension('vaapi',
                   define_macros = [('USE_DRM', '1')],
                   libraries = ['drm', 'va', 'va-drm'],
                   sources = ['vaapimodule.cc'])

setup(name = 'vaapi', version = '1.0', ext_modules = [module])
