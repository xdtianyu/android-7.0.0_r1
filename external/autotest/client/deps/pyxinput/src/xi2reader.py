#!/usr/bin/env python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ctypes
import select

import xi2
import xlib


class XI2Reader(object):
    """A reader to create connection to X server and read x input events."""
    def __init__(self, display_name=':0'):
        """Constructor

        Args:
            display_name: The X window display name.
        """
        self._display = xlib.XOpenDisplay(display_name)
        self._window = xlib.XDefaultRootWindow(self._display)
        self._data = []

        self._register()

        # Consumes the very first traffic within the connection with X server.
        xlib.XFlush(self._display)

    def _register(self):
        """Registers device and events to listen on"""
        mask = xi2.XIEventMask()
        mask.deviceid = xi2.XIAllDevices
        mask.mask_len = xi2.XIMaskLen(xi2.XI_RawMotion)
        mask.mask = ctypes.cast((ctypes.c_ubyte * mask.mask_len)(),
                                ctypes.POINTER(ctypes.c_ubyte))

        self._set_mask(mask.mask, xi2.XI_RawKeyPress)
        self._set_mask(mask.mask, xi2.XI_RawKeyRelease)
        self._set_mask(mask.mask, xi2.XI_RawButtonPress)
        self._set_mask(mask.mask, xi2.XI_RawButtonRelease)
        self._set_mask(mask.mask, xi2.XI_RawMotion)

        xi2.XISelectEvents(self._display, self._window, ctypes.pointer(mask), 1)
        xlib.XSelectInput(self._display, self._window, ctypes.c_long(0))

    def _set_mask(self, ptr, event):
        """Sets event mask"""
        val = xi2.XISetMask(ptr, event)
        ptr[event >> 3] = val

    def get_valuator_names(self, device_id):
        """Gets the valuator names for device.

        Return:
            An dictionary maps valuator index to descriptive names.
            Sample output:
            {
                0: 'Rel X',
                1: 'Rel Y',
                2: 'Abs Start Timestamp',
                3: 'Abs End Timestamp',
                4: 'Rel Vert Wheel',
                5: 'Rel Horiz Wheel'
            }
        """
        num_devices = ctypes.c_int()
        device = xi2.XIQueryDevice(self._display, device_id,
                ctypes.pointer(num_devices)).contents

        valuator_names = []
        for i in range(device.num_classes):
            if device.classes[i].contents.type == xi2.XIValuatorClass:
                valuator_class_info = ctypes.cast(device.classes[i],
                        ctypes.POINTER(xi2.XIValuatorClassInfo)).contents
                valuator_names.append(xlib.XGetAtomName(reader._display,
                        valuator_class_info.label))
        valuator_names_dict = {}
        for i in range(len(valuator_names)):
            valuator_names_dict[i] = valuator_names[i]
        return valuator_names_dict

    def get_connection_number(self):
        """Gets the file descriptor number for the connection with X server"""
        return xlib.XConnectionNumber(reader._display)

    def read_pending_events(self):
        """Read all the new event datas.

        Return:
            An array contains all event data with event type and valuator
            values. Sample format:
            {
                'deviceid': 11,
                'evtype': 17,
                'time': 406752437L,
                'valuators': {
                    0: (396.0, -38.0),
                    1: (578.0, -21.0),
                    2: (22802890.0, 22802890.0),
                    3: (26145746.0, 26145746.0)
                }
            }
        """
        data = []
        while xlib.XPending(self._display):
            xevent = xlib.XEvent()
            xlib.XNextEvent(self._display, ctypes.pointer(xevent))
            cookie = xevent.xcookie

            # Get event valuator_data
            result = xlib.XGetEventData(self._display, ctypes.pointer(cookie))
            if (not result or cookie.type != xlib.GenericEvent):
                continue

            raw_event_ptr = ctypes.cast(cookie.data,
                    ctypes.POINTER(xi2.XIRawEvent))
            raw_event = raw_event_ptr.contents
            valuator_state = raw_event.valuators

            # Two value arrays
            val_ptr = valuator_state.values
            val_idx = 0
            raw_val_ptr = raw_event.raw_values
            raw_val_idx = 0

            valuator_data = {}
            for i in range(valuator_state.mask_len):
                if xi2.XIMaskIsSet(valuator_state.mask, i):
                    valuator_data[i] = (val_ptr[val_idx],
                            raw_val_ptr[raw_val_idx])
                    val_idx += 1
                    raw_val_idx += 1
            data.append({'deviceid': raw_event.deviceid,
                         'evtype': cookie.evtype,
                         'time': raw_event.time,
                         'valuators': valuator_data})
        return data


if __name__ == '__main__':
    reader = XI2Reader()
    fd = reader.get_connection_number()

    while True:
        rl, _, _ = select.select([fd], [], [])
        if fd not in rl:
            break
        print reader.read_pending_events()
