#!/usr/bin/env python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Description:
#
# Class for injecting input events to linux 'evdev' input devices.
#
# Provides evemu-play-like functionality if run from the command line:
# $ input_event_player.py -d /dev/input/event6

""" Playback input events on a linux input device. """

import glob
import os.path
import re
import time

from input_device import InputDevice, InputEvent
from optparse import OptionParser


class InputEventPlayer:
    """ Linux evdev input event player.

    An "evdev" input event player injects a stream of "input events" to kernel
    evdev driver. With this player, we could easily playback an input event file
    which was recorded with the tool evemu-record previously.

    """

    def __init__(self):
        self.tv_sec = None
        self.tv_usec = None

    def playback(self, device, gesture_file):
        """ Play the events in gesture_file on device.

        Keyword arguments:
        device -- the InputDevice device object
        gesture_file -- the name of the event file recorded previously
        """
        if not device:
            raise
        event_str = 'E: (\d+)\.(\d+) ([0-9a-f]{4}) ([0-9a-f]{4}) ([-]?\d+)'
        event_pattern = re.compile(event_str)
        for line in open(gesture_file, 'rt'):
            m = event_pattern.match(line)
            if not m:
                raise
            event = InputEvent(int(m.group(1)),
                               int(m.group(2)),
                               int(m.group(3), 16),
                               int(m.group(4), 16),
                               int(m.group(5)))
            if not self.tv_sec:
                self.tv_sec = event.tv_sec
                self.tv_usec = event.tv_usec
            delta = event.tv_sec - self.tv_sec
            delta += ((event.tv_usec - self.tv_usec) / 1000000.0)
            # Sleep only if the event is 0.05 ms later than the previous one
            if delta > 0.0000500:
                time.sleep(delta)
            self.tv_sec = event.tv_sec
            self.tv_usec = event.tv_usec
            event.write(device.f)


if __name__ == '__main__':
    parser = OptionParser()

    parser.add_option('-d', '--devpath', dest='devpath', default='',
                      help='device path (/dev/input/event0)')
    parser.add_option('-t', '--touchpad', action='store_true', dest='touchpad',
                      default=False, help='Find and use first touchpad device')
    parser.add_option('-f', '--file', action='store', dest='gesture_file',
                      help='Event file to playback')
    (options, args) = parser.parse_args()

    if options.touchpad:
        for evdev in glob.glob('/dev/input/event*'):
            device = InputDevice(evdev)
            if device.is_touchpad():
                break
        else:
            print 'Can not find a touchpad device'
            exit()
    elif not os.path.exists(options.devpath):
        print 'Can not find the input device "%s".' % options.devpath
        exit()
    else:
        device = InputDevice(options.devpath)
    if not options.gesture_file:
        print 'Gesture file is not specified.'
        exit()
    if not os.path.exists(options.gesture_file):
        print 'Can not find the gesture file %s.' % options.gesture_file
        exit()

    InputEventPlayer().playback(device, options.gesture_file)
    print 'Gesture file %s has been played.' % options.gesture_file
