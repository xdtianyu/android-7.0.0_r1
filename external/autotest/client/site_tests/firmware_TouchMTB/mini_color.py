#!/usr/bin/python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

''' A mini class for terminal color '''

import re


CSI = '\033['                       # ANSI Control Sequence Introducer
NORMAL = CSI + '0m'
CODE = {'black': CSI + '1;30m',     # color code for foreground color
        'red': CSI + '1;31m',
        'green': CSI + '1;32m',
        'yellow': CSI + '1;33m',
        'blue': CSI + '1;34m',
        'magenta': CSI + '1;35m',
        'cyan': CSI + '1;36m',
        'white': CSI + '1;37m',
        'default': CSI + '1;39m',
       }


def color_string(string, bgn_sym, end_sym, color):
    ''' Insert color code for a bracketed substring in a given string '''
    cstring = re.sub(bgn_sym, CODE[color] + bgn_sym, string)
    cstring = re.sub(end_sym, end_sym + NORMAL, cstring)
    return cstring
