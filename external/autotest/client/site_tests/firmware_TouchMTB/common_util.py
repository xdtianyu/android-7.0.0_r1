# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common utility functions that are not touch specific."""

import logging
import os
import subprocess
import sys
import termios
import tty


def simple_system(cmd):
    """Replace autotest utils.system() locally.

    This method is grabbed from hardware_Trackpad and was written by truty.
    """
    ret = subprocess.call(cmd, shell=True)
    if ret:
        logging.warning('Command (%s) failed (ret=%s).', cmd, ret)
    return ret


def simple_system_output(cmd):
    """Replace autotest utils.system_output() locally.

    This method is grabbed from hardware_Trackpad and was written by truty.
    """
    try:
        proc = subprocess.Popen(cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        stdout, _ = proc.communicate()
    except Exception, e:
        logging.warning('Command (%s) failed (%s).', cmd, e)
    else:
        if proc.returncode:
            return None
        return stdout.strip()


def getch():
    """Get a single character without typing ENTER."""
    fin = sys.stdin
    old_attrs = termios.tcgetattr(fin)
    tty.setraw(fin.fileno())
    try:
        char = fin.read(1)
    except ValueError:
        char = ''
    finally:
        termios.tcsetattr(fin, termios.TCSADRAIN, old_attrs)
    return char


def program_exists(program):
    """Check if an executable program exists."""
    return os.system('which %s > /dev/null 2>&1' % program) == 0


def print_and_exit(msg, exit_code=1):
    """Print a message and exit."""
    print msg
    sys.exit(exit_code)


class Debug:
    """A simple class to print the debug message."""
    def __init__(self, debug_flag=False):
        self._debug_flag = debug_flag

    def print_msg(self, msg):
        """Print the message if _debug_flag is True."""
        if self._debug_flag:
            print msg
