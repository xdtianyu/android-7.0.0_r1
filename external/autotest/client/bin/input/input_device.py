#!/usr/bin/env python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Description:
#
# Class for handling linux 'evdev' input devices.
#
# Provides evtest-like functionality if run from the command line:
# $ input_device.py -d /dev/input/event6

""" Read properties and events of a linux input device. """

import array
import copy
import fcntl
import os.path
import select
import struct
import time

from collections import OrderedDict

from linux_input import *


class Valuator:
    """ A Valuator just stores a value """
    def __init__(self):
        self.value = 0

class SwValuator(Valuator):
    """ A Valuator used for EV_SW (switch) events """
    def __init__(self, value):
        self.value = value

class AbsValuator(Valuator):
    """
    An AbsValuator, used for EV_ABS events stores a value as well as other
    properties of the corresponding absolute axis.
    """
    def __init__(self, value, min_value, max_value, fuzz, flat, resolution):
        self.value = value
        self.min = min_value
        self.max = max_value
        self.fuzz = fuzz
        self.flat = flat
        self.resolution = resolution


class InputEvent:
    """
    Linux evdev input event

    An input event has the following fields which can be accessed as public
    properties of this class:
        tv_sec
        tv_usec
        type
        code
        value
    """
    def __init__(self, tv_sec=0, tv_usec=0, type=0, code=0, value=0):
        self.format = input_event_t
        self.format_size = struct.calcsize(self.format)
        (self.tv_sec, self.tv_usec, self.type, self.code,
         self.value) = (tv_sec, tv_usec, type, code, value)

    def read(self, stream):
        """ Read an input event from the provided stream and unpack it. """
        packed = stream.read(self.format_size)
        (self.tv_sec, self.tv_usec, self.type, self.code,
         self.value) = struct.unpack(self.format, packed)

    def write(self, stream):
        """ Pack an input event and write it to the provided stream. """
        packed = struct.pack(self.format, self.tv_sec, self.tv_usec, self.type,
                             self.code, self.value)
        stream.write(packed)
        stream.flush()

    def __str__(self):
        t = EV_TYPES.get(self.type, self.type)
        if self.type in EV_STRINGS:
            c = EV_STRINGS[self.type].get(self.code, self.code)
        else:
            c = self.code
        return ('%d.%06d: %s[%s] = %d' %
                (self.tv_sec, self.tv_usec, t, c, self.value))


class InputDevice:
    """
    Linux evdev input device

    A linux kernel "evdev" device sends a stream of "input events".
    These events are grouped together into "input reports", which is a set of
    input events ending in a single EV_SYN/SYN_REPORT event.

    Each input event is tagged with a type and a code.
    A given input device supports a subset of the possible types, and for
    each type it supports a subset of the possible codes for that type.

    The device maintains a "valuator" for each supported type/code pairs.
    There are two types of "valuators":
       Normal valuators represent just a value.
       Absolute valuators are only for EV_ABS events. They have more fields:
           value, minimum, maximum, resolution, fuzz, flatness
    Note: Relative and Absolute "Valuators" are also often called relative
          and absolute axis, respectively.

    The evdev protocol is stateful.  Input events are only sent when the values
    of a valuator actually changes.  This dramatically reduces the stream of
    events emenating from the kernel.

    Multitouch devices are a special case.  There are two types of multitouch
    devices defined in the kernel:
        Multitouch type "A" (MT-A) devices:
            In each input report, the device sends an unordered list of all
            active contacts.  The data for each active contact is separated
            in the input report by an EV_SYN/SYN_MT_REPORT event.
            Thus, the MT-A contact event stream is not stateful.
            Note: MT-A is not currently supported by this class.

        Multitouch type "B" (MT-B) devices:
            The device maintains a list of slots, where each slot contains a
            single contact.  In each input report, the device only sends
            information about the slots that have changed.
            Thus, the MT-B contact event stream is stateful.
            When reporting multiple slots, the EV_ABS/MT_SLOT valuator is used
            to indicate the 'current' slot for which subsequent EV_ABS/ABS_MT_*
            valuator events apply.
            An inactive slot has EV_ABS/ABS_MT_TRACKING_ID == -1
            Active slots have EV_ABS/ABS_MT_TRACKING_ID >= 0

    Besides maintaining the set of supported ABS_MT valuators in the supported
    valuator list, a array of slots is also maintained.  Each slot has its own
    unique copy of just the supported ABS_MT valuators.  This represents the
    current state of that slot.
    """

    def __init__(self, path, ev_syn_cb=None):
        """
        Constructor opens the device file and probes its properties.

        Note: The device file is left open when the constructor exits.
        """
        self.path = path
        self.ev_syn_cb = ev_syn_cb
        self.events = {}     # dict { ev_type : dict { ev_code : Valuator } }
        self.mt_slots = []   # [ dict { mt_code : AbsValuator } ] * |MT-B slots|

        # Open the device node, and use ioctls to probe its properties
        self.f = None
        self.f = open(path, 'rb+', buffering=0)
        self._ioctl_version()
        self._ioctl_id()
        self._ioctl_name()
        for t in self._ioctl_types():
            self._ioctl_codes(t)
        self._setup_mt_slots()

    def __del__(self):
        """
        Deconstructor closes the device file, if it is open.
        """
        if self.f and not self.f.closed:
            self.f.close()

    def process_event(self, ev):
        """
        Processes an incoming input event.

        Returns True for EV_SYN/SYN_REPORT events to indicate that a complete
        input report has been received.

        Returns False for other events.

        Events not supported by this device are silently ignored.

        For MT events, updates the slot valuator value for the current slot.
        If current slot is the 'primary' slot, also updates the events entry.

        For all other events, updates the corresponding valuator value.
        """
        if ev.type == EV_SYN and ev.code == SYN_REPORT:
            return True
        elif ev.type not in self.events or ev.code not in self.events[ev.type]:
            return False
        elif self.is_mt_b() and ev.type == EV_ABS and ev.code in ABS_MT_RANGE:
            # TODO: Handle MT-A
            slot = self._get_current_slot()
            slot[ev.code].value = ev.value
            # if the current slot is the "primary" slot,
            # update the events[] entry, too.
            if slot == self._get_mt_primary_slot():
                self.events[ev.type][ev.code].value = ev.value
        else:
            self.events[ev.type][ev.code].value = ev.value
        return False

    def _ioctl_version(self):
        """ Queries device file for version information. """
        # Version is a 32-bit integer, which encodes 8-bit major version,
        # 8-bit minor version and 16-bit revision.
        version = array.array('I', [0])
        fcntl.ioctl(self.f, EVIOCGVERSION, version, 1)
        self.version = (version[0] >> 16, (version[0] >> 8) & 0xff,
                        version[0] & 0xff)

    def _ioctl_id(self):
        """ Queries device file for input device identification. """
        # struct input_id is 4 __u16
        gid = array.array('H', [0] * 4)
        fcntl.ioctl(self.f, EVIOCGID, gid, 1)
        self.id_bus = gid[ID_BUS]
        self.id_vendor = gid[ID_VENDOR]
        self.id_product = gid[ID_PRODUCT]
        self.id_version = gid[ID_VERSION]

    def _ioctl_name(self):
        """ Queries device file for the device name. """
        # Device name is a C-string up to 255 bytes in length.
        name_len = 255
        name = array.array('B', [0] * name_len)
        name_len = fcntl.ioctl(self.f, EVIOCGNAME(name_len), name, 1)
        self.name = name[0:name_len-1].tostring()

    def _ioctl_get_switch(self, sw):
        """
        Queries device file for current value of all switches and returns
        a boolean indicating whether the switch sw is set.
        """
        size = SW_CNT / 8    # Buffer size of one __u16
        buf = array.array('H', [0])
        fcntl.ioctl(self.f, EVIOCGSW(size), buf)
        return SwValuator(((buf[0] >> sw) & 0x01) == 1)

    def _ioctl_absinfo(self, axis):
        """
        Queries device file for absinfo structure for given absolute axis.
        """
        # struct input_absinfo is 6 __s32
        a = array.array('i', [0] * 6)
        fcntl.ioctl(self.f, EVIOCGABS(axis), a, 1)
        return AbsValuator(a[0], a[1], a[2], a[3], a[4], a[5])

    def _ioctl_codes(self, ev_type):
        """
        Queries device file for supported event codes for given event type.
        """
        self.events[ev_type] = {}
        if ev_type not in EV_SIZES:
            return

        size = EV_SIZES[ev_type] / 8    # Convert bits to bytes
        ev_code = array.array('B', [0] * size)
        try:
            count = fcntl.ioctl(self.f, EVIOCGBIT(ev_type, size), ev_code, 1)
            for c in range(count * 8):
                if test_bit(c, ev_code):
                    if ev_type == EV_ABS:
                        self.events[ev_type][c] = self._ioctl_absinfo(c)
                    elif ev_type == EV_SW:
                        self.events[ev_type][c] = self._ioctl_get_switch(c)
                    else:
                        self.events[ev_type][c] = Valuator()
        except IOError as (errno, strerror):
            # Errno 22 signifies that this event type has no event codes.
            if errno != 22:
                raise

    def _ioctl_types(self):
        """ Queries device file for supported event types. """
        ev_types = array.array('B', [0] * (EV_CNT / 8))
        fcntl.ioctl(self.f, EVIOCGBIT(EV_SYN, EV_CNT / 8), ev_types, 1)
        types  = []
        for t in range(EV_CNT):
            if test_bit(t, ev_types):
                types.append(t)
        return types

    def _convert_slot_index_to_slot_id(self, index):
        """ Convert a slot index in self.mt_slots to its slot id. """
        return self.abs_mt_slot.min + index

    def _ioctl_mt_slots(self):
        """Query mt slots values using ioctl.

        The ioctl buffer argument should be binary equivalent to
        struct input_mt_request_layout {
            __u32 code;
            __s32 values[num_slots];

        Note that the slots information returned by EVIOCGMTSLOTS
        corresponds to the slot ids ranging from abs_mt_slot.min to
        abs_mt_slot.max which is not necessarily the same as the
        slot indexes ranging from 0 to num_slots - 1 in self.mt_slots.
        We need to map between the slot index and the slot id correctly.
        };
        """
        # Iterate through the absolute mt events that are supported.
        for c in range(ABS_MT_FIRST, ABS_MT_LAST):
            if c not in self.events[EV_ABS]:
                continue
            # Sync with evdev kernel driver for the specified code c.
            mt_slot_info = array.array('i', [c] + [0] * self.num_slots)
            mt_slot_info_len = (self.num_slots + 1) * mt_slot_info.itemsize
            fcntl.ioctl(self.f, EVIOCGMTSLOTS(mt_slot_info_len), mt_slot_info)
            values = mt_slot_info[1:]
            for slot_index in range(self.num_slots):
                slot_id = self._convert_slot_index_to_slot_id(slot_index)
                self.mt_slots[slot_index][c].value = values[slot_id]

    def _setup_mt_slots(self):
        """
        Sets up the device's mt_slots array.

        Each element of the mt_slots array is initialized as a deepcopy of a
        dict containing all of the MT valuators from the events dict.
        """
        # TODO(djkurtz): MT-A
        if not self.is_mt_b():
            return
        ev_abs = self.events[EV_ABS]
        # Create dict containing just the MT valuators
        mt_abs_info = dict((axis, ev_abs[axis])
                           for axis in ev_abs
                           if axis in ABS_MT_RANGE)

        # Initialize TRACKING_ID to -1
        mt_abs_info[ABS_MT_TRACKING_ID].value = -1

        # Make a copy of mt_abs_info for each MT slot
        self.abs_mt_slot = ev_abs[ABS_MT_SLOT]
        self.num_slots = self.abs_mt_slot.max - self.abs_mt_slot.min + 1
        for s in range(self.num_slots):
            self.mt_slots.append(copy.deepcopy(mt_abs_info))

        self._ioctl_mt_slots()

    def get_current_slot_id(self):
        """
        Return the current slot id.
        """
        if not self.is_mt_b():
            return None
        return self.events[EV_ABS][ABS_MT_SLOT].value

    def _get_current_slot(self):
        """
        Returns the current slot, as indicated by the last ABS_MT_SLOT event.
        """
        current_slot_id = self.get_current_slot_id()
        if current_slot_id is None:
            return None
        return self.mt_slots[current_slot_id]

    def _get_tid(self, slot):
        """ Returns the tracking_id for the given MT slot. """
        return slot[ABS_MT_TRACKING_ID].value

    def _get_mt_valid_slots(self):
        """
        Returns a list of valid slots.

        A valid slot is a slot whose tracking_id != -1.
        """
        return [s for s in self.mt_slots if self._get_tid(s) != -1]

    def _get_mt_primary_slot(self):
        """
        Returns the "primary" MT-B slot.

        The "primary" MT-B slot is arbitrarily chosen as the slot with lowest
        tracking_id (> -1).  It is used to make an MT-B device look like
        single-touch (ST) device.
        """
        slot = None
        for s in self.mt_slots:
            tid = self._get_tid(s)
            if tid < 0:
                continue
            if not slot or tid < self._get_tid(slot):
                slot = s
        return slot

    def _code_if_mt(self, type, code):
        """
        Returns MT-equivalent event code for certain specific event codes
        """
        if type != EV_ABS:
            return code
        elif code == ABS_X:
            return  ABS_MT_POSITION_X
        elif code == ABS_Y:
            return ABS_MT_POSITION_Y
        elif code == ABS_PRESSURE:
            return ABS_MT_PRESSURE
        elif code == ABS_TOOL_WIDTH:
            return ABS_TOUCH_MAJOR
        else:
            return code

    def _get_valuator(self, type, code):
        """ Returns Valuator for given event type and code """
        if (not type in self.events) or (not code in self.events[type]):
            return None
        if type == EV_ABS:
            code = self._code_if_mt(type, code)
        return self.events[type][code]

    def _get_value(self, type, code):
        """
        Returns the value of the valuator with the give event (type, code).
        """
        axis = self._get_valuator(type, code)
        if not axis:
            return None
        return axis.value

    def _get_min(self, type, code):
        """
        Returns the min value of the valuator with the give event (type, code).

        Note: Only AbsValuators (EV_ABS) have max values.
        """
        axis = self._get_valuator(type, code)
        if not axis:
            return None
        return axis.min

    def _get_max(self, type, code):
        """
        Returns the min value of the valuator with the give event (type, code).

        Note: Only AbsValuators (EV_ABS) have max values.
        """
        axis = self._get_valuator(type, code)
        if not axis:
            return None
        return axis.max

    """ Public accessors """

    def get_num_fingers(self):
        if self.is_mt_b():
            return len(self._get_mt_valid_slots())
        elif self.is_mt_a():
            return 0  # TODO(djkurtz): MT-A
        else:  # Single-Touch case
            if not self._get_value(EV_KEY, BTN_TOUCH) == 1:
                return 0
            elif self._get_value(EV_KEY, BTN_TOOL_TRIPLETAP) == 1:
                return 3
            elif self._get_value(EV_KEY, BTN_TOOL_DOUBLETAP) == 1:
                return 2
            elif self._get_value(EV_KEY, BTN_TOOL_FINGER) == 1:
                return 1
            else:
                return 0

    def get_x(self):
        return self._get_value(EV_ABS, ABS_X)

    def get_x_min(self):
        return self._get_min(EV_ABS, ABS_X)

    def get_x_max(self):
        return self._get_max(EV_ABS, ABS_X)

    def get_y(self):
        return self._get_value(EV_ABS, ABS_Y)

    def get_y_min(self):
        return self._get_min(EV_ABS, ABS_Y)

    def get_y_max(self):
        return self._get_max(EV_ABS, ABS_Y)

    def get_pressure(self):
        return self._get_value(EV_ABS, ABS_PRESSURE)

    def get_pressure_min(self):
        return self._get_min(EV_ABS, ABS_PRESSURE)

    def get_pressure_max(self):
        return self._get_max(EV_ABS, ABS_PRESSURE)

    def get_left(self):
        return int(self._get_value(EV_KEY, BTN_LEFT) == 1)

    def get_right(self):
        return int(self._get_value(EV_KEY, BTN_RIGHT) == 1)

    def get_middle(self):
        return int(self._get_value(EV_KEY, BTN_MIDDLE) == 1)

    def get_microphone_insert(self):
        return self._get_value(EV_SW, SW_MICROPHONE_INSERT)

    def get_headphone_insert(self):
        return self._get_value(EV_SW, SW_HEADPHONE_INSERT)

    def get_lineout_insert(self):
        return self._get_value(EV_SW, SW_LINEOUT_INSERT)

    def is_touchpad(self):
        return ((EV_KEY in self.events) and
                (BTN_TOOL_FINGER in self.events[EV_KEY]) and
                (EV_ABS in self.events))

    def is_keyboard(self):
        return ((EV_KEY in self.events) and
                (KEY_F2 in self.events[EV_KEY]))

    def is_touchscreen(self):
        return ((EV_KEY in self.events) and
                (BTN_TOUCH in self.events[EV_KEY]) and
                (not BTN_TOOL_FINGER in self.events[EV_KEY]) and
                (EV_ABS in self.events))

    def is_mt_b(self):
        return self.is_mt() and ABS_MT_SLOT in self.events[EV_ABS]

    def is_mt_a(self):
        return self.is_mt() and ABS_MT_SLOT not in self.events[EV_ABS]

    def is_mt(self):
        return (EV_ABS in self.events and
                (set(self.events[EV_ABS]) & set(ABS_MT_RANGE)))

    def is_hp_jack(self):
        return (EV_SW in self.events and
                SW_HEADPHONE_INSERT in self.events[EV_SW])

    def is_mic_jack(self):
        return (EV_SW in self.events and
                SW_MICROPHONE_INSERT in self.events[EV_SW])

    def is_audio_jack(self):
        return (EV_SW in self.events and
                ((SW_HEADPHONE_INSERT in self.events[EV_SW]) or
                 (SW_MICROPHONE_INSERT in self.events[EV_SW] or
                 (SW_LINEOUT_INSERT in self.events[EV_SW]))))

    """ Debug helper print functions """

    def print_abs_info(self, axis):
        if EV_ABS in self.events and axis in self.events[EV_ABS]:
            a = self.events[EV_ABS][axis]
            print '      Value       %6d' % a.value
            print '      Min         %6d' % a.min
            print '      Max         %6d' % a.max
            if a.fuzz != 0:
                print '      Fuzz        %6d' % a.fuzz
            if a.flat != 0:
                print '      Flat        %6d' % a.flat
            if a.resolution != 0:
                print '      Resolution  %6d' % a.resolution

    def print_props(self):
        print ('Input driver Version: %d.%d.%d' %
               (self.version[0], self.version[1], self.version[2]))
        print ('Input device ID: bus %x vendor %x product %x version %x' %
               (self.id_bus, self.id_vendor, self.id_product, self.id_version))
        print 'Input device name: "%s"' % (self.name)
        for t in self.events:
            print '  Event type %d (%s)' % (t, EV_TYPES.get(t, '?'))
            for c in self.events[t]:
                if (t in EV_STRINGS):
                    code = EV_STRINGS[t].get(c, '?')
                    print '    Event code %s (%d)' % (code, c)
                else:
                    print '    Event code (%d)' % (c)
                self.print_abs_info(c)

    def get_slots(self):
        """ Get those slots with positive tracking IDs. """
        slot_dict = OrderedDict()
        for slot_index in range(self.num_slots):
            slot = self.mt_slots[slot_index]
            if self._get_tid(slot) == -1:
                continue
            slot_id = self._convert_slot_index_to_slot_id(slot_index)
            slot_dict[slot_id] = slot
        return slot_dict

    def print_slots(self):
        slot_dict = self.get_slots()
        for slot_id, slot in slot_dict.items():
            print 'slot #%d' % slot_id
            for a in slot:
                abs = EV_STRINGS[EV_ABS].get(a, '?')
                print '  %s = %6d' % (abs, slot[a].value)


def print_report(device):
    print '----- EV_SYN -----'
    if device.is_touchpad():
        f = device.get_num_fingers()
        if f == 0:
            return
        x = device.get_x()
        y = device.get_y()
        z = device.get_pressure()
        l = device.get_left()
        print 'Left=%d Fingers=%d X=%d Y=%d Pressure=%d' % (l, f, x, y, z)
        if device.is_mt():
            device.print_slots()


if __name__ == "__main__":
    from optparse import OptionParser
    import glob
    parser = OptionParser()

    parser.add_option("-a", "--audio_jack", action="store_true",
                      dest="audio_jack", default=False,
                      help="Find and use all audio jacks")
    parser.add_option("-d", "--devpath", dest="devpath",
                      default="/dev/input/event0",
                      help="device path (/dev/input/event0)")
    parser.add_option("-q", "--quiet", action="store_false", dest="verbose",
                      default=True, help="print less messages to stdout")
    parser.add_option("-t", "--touchpad", action="store_true", dest="touchpad",
                      default=False, help="Find and use first touchpad device")
    (options, args) = parser.parse_args()

    # TODO: Use gudev to detect touchpad
    devices = []
    if options.touchpad:
        for path in glob.glob('/dev/input/event*'):
            device = InputDevice(path)
            if device.is_touchpad():
                print 'Using touchpad %s.' % path
                options.devpath = path
                devices.append(device)
                break
        else:
            print 'No touchpad found!'
            exit()
    elif options.audio_jack:
        for path in glob.glob('/dev/input/event*'):
            device = InputDevice(path)
            if device.is_audio_jack():
                devices.append(device)
        device = None
    elif os.path.exists(options.devpath):
        print 'Using %s.' % options.devpath
        devices.append(InputDevice(options.devpath))
    else:
        print '%s does not exist.' % options.devpath
        exit()

    for device in devices:
        device.print_props()
        if device.is_touchpad():
            print ('x: (%d,%d), y: (%d,%d), z: (%d, %d)' %
                   (device.get_x_min(), device.get_x_max(),
                    device.get_y_min(), device.get_y_max(),
                    device.get_pressure_min(), device.get_pressure_max()))
            device.print_slots()
            print 'Number of fingers: %d' % device.get_num_fingers()
            print 'Current slot id: %d' % device.get_current_slot_id()
    print '------------------'
    print

    ev = InputEvent()
    while True:
        _rl, _, _ = select.select([d.f for d in devices], [], [])
        for fd in _rl:
            # Lookup for the device which owns fd.
            device = [d for d in devices if d.f == fd][0]
            try:
                ev.read(fd)
            except KeyboardInterrupt:
                exit()
            is_syn = device.process_event(ev)
            print ev
            if is_syn:
                print_report(device)
