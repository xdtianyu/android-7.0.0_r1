# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A simple script to play an mtplot device data file.

Usage:   python tools/mtplot_play.py file

Example:
    1. Replay a touchpad raw data file:
       $ python tools/mtplot_play.py \
         /tmp/two_finger_tap.vertical-link-fw_1.0.AA-robot-20130806_224733.dat
    2. Replay a touchscreen raw data file:
       $ python tools/mtplot_play.py -d touchscreen \
         /tmp/two_finger_tap.horizontal-link-fw_1.0.AA-robot-20130806_220011.dat
"""

import argparse
import os
import subprocess
import sys

import common
import mtb

from common_util import print_and_exit, simple_system
from firmware_utils import ScreenShot, SimpleX
from touch_device import TouchDevice


def generate_mtplot_image_from_log(device_node, mtplot_file):
    """Convert the mtplot file to evemu format, and play it with evemu-play.

    @param device_node: the touch device node on which to play the mtplot_file
    @param mtplot_file: a device file in mtplot format
    """
    # Convert the mtplot file to evemu file.
    evemu_file = mtb.convert_mtplot_file_to_evemu_file(mtplot_file,
                                                       evemu_dir='/tmp',
                                                       force=True)

    if not evemu_file:
        msg = 'Error to convert data from mtplot format to evemu format: %s'
        print msg % mtplot_file
        return

    # Launch mtplot in order to capture the image.
    mtplot_cmd = 'mtplot -d :0 %s' % device_node
    devnull = open(os.devnull, 'w')
    proc = subprocess.Popen(mtplot_cmd.split(), stdout=devnull)

    play_cmd = 'evemu-play --insert-slot0 %s < %s' % (device_node, evemu_file)
    print 'Executing: %s\n' % play_cmd
    simple_system(play_cmd)

    # evemu_file looks like drumroll.fast-link-fw_1.0-robot-20130829.evemu.dat
    # image_file looks like drumroll.fast-link-fw_1.0-robot-20130829
    image_file = evemu_file.rsplit('.', 2)[0]

    # Dump the screen shot to the image file.
    width, height = SimpleX('aura').get_screen_size()
    geometry_str = '%dx%d+%d+%d' % (width, height, 0, 0)
    ScreenShot(geometry_str).dump_root(image_file)

    # Terminate mtplot.
    proc.poll()
    if proc.returncode is None:
        proc.terminate()
        proc.wait()
    devnull.close()

    print 'Files saved:'
    print 'The evemu file: %s'  % evemu_file
    print 'The mtplot image file: %s\n'  % image_file


def _parse():
    """Parse the command line options."""
    parser = argparse.ArgumentParser(
            description='Play a raw data file and capture its image.')
    parser.add_argument('filename', help='a raw data file in mtplot format')
    parser.add_argument('-d', '--device',
                        help='the device type (default: touchpad)',
                        choices=['touchpad', 'touchscreen'],
                        default='touchpad')
    args = parser.parse_args()

    # Get the touchpad/touchscreen device node from the device option
    is_ts = (args.device == 'touchscreen')
    args.device_node = TouchDevice.get_device_node(is_touchscreen=is_ts)
    if args.device_node is None:
        print_and_exit('Error: fail to get device node for %s.' % args.device)

    # Check the existence of the raw data file.
    if not os.path.isfile(args.filename):
        print_and_exit('Error: The file "%s" does not exist.' % args.filename)

    print '\nthe device node of the %s: %s\n' % (args.device, args.device_node)
    print 'the raw data file: %s\n' % args.filename

    return args


if __name__ == '__main__':
    args = _parse()
    generate_mtplot_image_from_log(args.device_node, args.filename)
