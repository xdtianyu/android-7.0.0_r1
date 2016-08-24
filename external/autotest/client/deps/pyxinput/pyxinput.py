#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is the setup script for pyxinput autotest dependency, which
# will be called at emerge stage.

import logging
import os

import ctypesgencore

from autotest_lib.client.bin import utils


version = 1

def setup(topdir):
    class Opt(object):
        """Object to hold ctypesgen parseing options"""
        def __init__(self, attrs):
            for attr in attrs:
                setattr(self, attr, attrs[attr])

        def gen(self):
            """Generate outputs"""
            desc = ctypesgencore.parser.parse(self.headers, self)
            ctypesgencore.processor.process(desc, self)
            ctypesgencore.printer.WrapperPrinter(self.output, self, desc)

    os.chdir(os.path.join(topdir, 'src'))

    # Generate xlib.py
    opt = Opt(ctypesgencore.options.default_values)
    opt.libraries = ['X11']
    opt.headers = ['/usr/include/X11/Xlib.h',
                   '/usr/include/X11/X.h',
                   '/usr/include/X11/Xutil.h']
    opt.output = 'xlib.py'
    opt.other_known_names = ['None']
    opt.gen()

    # Generate xi2.py
    opt = Opt(ctypesgencore.options.default_values)
    opt.libraries = ['Xi']
    opt.headers = ['/usr/include/X11/extensions/XI2.h',
                   '/usr/include/X11/extensions/XInput2.h']
    opt.output = 'xi2.py'
    opt.other_known_names = ['None']
    opt.gen()

    os.chdir(topdir)

pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup, pwd)
