#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

"""Interface for a USB-connected Monsoon power meter
(http://msoon.com/LabEquipment/PowerMonitor/).
"""

_Python3_author_ = 'angli@google.com (Ang Li)'
_author_ = 'kens@google.com (Ken Shirriff)'

import argparse
import sys
import time
import collections

from acts.controllers.monsoon import Monsoon

def main(FLAGS):
    """Simple command-line interface for Monsoon."""
    if FLAGS.avg and FLAGS.avg < 0:
        print("--avg must be greater than 0")
        return

    mon = Monsoon(serial=int(FLAGS.serialno[0]))

    if FLAGS.voltage is not None:
        mon.set_voltage(FLAGS.voltage)

    if FLAGS.current is not None:
        mon.set_max_current(FLAGS.current)

    if FLAGS.status:
        items = sorted(mon.status.items())
        print("\n".join(["%s: %s" % item for item in items]))

    if FLAGS.usbpassthrough:
        mon.usb(FLAGS.usbpassthrough)

    if FLAGS.startcurrent is not None:
         mon.set_max_init_current(FLAGS.startcurrent)

    if FLAGS.samples:
        # Have to sleep a bit here for monsoon to be ready to lower the rate of
        # socket read timeout.
        time.sleep(1)
        result = mon.take_samples(FLAGS.hz, FLAGS.samples,
            sample_offset=FLAGS.offset, live=True)
        print(repr(result))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=("This is a python utility "
                 "tool to control monsoon power measurement boxes."))
    parser.add_argument("--status", action="store_true",
        help="Print power meter status.")
    parser.add_argument("-avg", "--avg", type=int, default=0,
        help="Also report average over last n data points.")
    parser.add_argument("-v", "--voltage", type=float,
        help="Set output voltage (0 for off)")
    parser.add_argument("-c", "--current", type=float,
        help="Set max output current.")
    parser.add_argument("-sc", "--startcurrent", type=float,
        help="Set max power-up/inital current.")
    parser.add_argument("-usb", "--usbpassthrough", choices=("on", "off",
        "auto"), help="USB control (on, off, auto).")
    parser.add_argument("-sp", "--samples", type=int,
        help="Collect and print this many samples")
    parser.add_argument("-hz", "--hz", type=int,
        help="Sample this many times per second.")
    parser.add_argument("-d", "--device", help="Use this /dev/ttyACM... file.")
    parser.add_argument("-sn", "--serialno", type=int, nargs=1, required=True,
        help="The serial number of the Monsoon to use.")
    parser.add_argument("--offset", type=int, nargs='?', default=0,
        help="The number of samples to discard when calculating average.")
    parser.add_argument("-r", "--ramp", action="store_true", help=("Gradually "
        "increase voltage to prevent tripping Monsoon overvoltage"))
    args = parser.parse_args()
    main(args)
