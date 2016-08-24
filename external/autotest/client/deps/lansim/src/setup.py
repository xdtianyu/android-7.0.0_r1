# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from distutils.core import setup, Extension

# C extension modules.
DEPS=['Makefile', 'setup.py', 'pyiftun.version']
PYIFTUN_SRC = [
    'pyiftun.c',
    'wrapper_linux_if.c',
    'wrapper_linux_if_tun.c',
    'wrapper_sys_ioctl.c',
]
PYIFTUN_DEPS = DEPS + PYIFTUN_SRC

CFLAGS=['-O2', '-Wall', '-Werror']

ext_mods = []
ext_mods.append(Extension('pyiftun',
    sources = PYIFTUN_SRC,
    extra_compile_args=CFLAGS,
    extra_link_args = ['-Wl,--version-script=pyiftun.version'],
    depends = DEPS + PYIFTUN_SRC,
))

# Python modules.
py_mods = [
    'lansim.host',
    'lansim.simulator',
    'lansim.tools',
    'lansim.tuntap',
]

setup(name = 'lansim',
    version = '1',
    description = 'A LAN simulator in Python',
    maintainer = 'Alex Deymo',
    maintainer_email = 'deymo@chromium.org',
    # Pure python modules from lansim_py:
    package_dir = { 'lansim': 'py' },
    py_modules = py_mods,
    # Compiled modules on the package:
    ext_package = 'lansim',
    ext_modules = ext_mods,
)
