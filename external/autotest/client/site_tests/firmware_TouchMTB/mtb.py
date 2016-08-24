# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides MTB parser and related packet methods."""

import copy
import logging
import math
import os
import re
import sys

from collections import defaultdict, namedtuple, OrderedDict

from firmware_constants import AXIS, GV, MTB, UNIT, VAL
from geometry.elements import Point
from geometry.two_farthest_clusters import (
        get_radii_of_two_minimal_enclosing_circles as get_two_min_radii,
        get_two_farthest_points
)
sys.path.append('../../bin/input')
from linux_input import *


# Define TidPacket to keep the point, pressure, and SYN_REPOT time of a packet.
TidPacket = namedtuple('TidPacket', ['syn_time', 'point', 'pressure'])


# Define FingerPath class to keep track of the slot, and a list of tid packets
# of a finger (i.e., a tracking ID).
class FingerPath(namedtuple('FingerPath', ['slot', 'tid_packets'])):
    """This keeps the slot number and the list of tid packets of a finger."""
    __slots__ = ()

    def get(self, attr):
        """Get the list of the specified attribute, attr (i.e., point,
        pressure, or syn_time), of TidPacket for the finger.
        """
        return [getattr(tid_packet, attr) for tid_packet in self.tid_packets]


def get_mtb_packets_from_file(event_file):
    """ A helper function to get mtb packets by parsing the event file.

    @param event_file: an mtb_event file
    """
    return Mtb(packets=MtbParser().parse_file(event_file))


def make_pretty_packet(packet):
    """Convert the event list in a packet to a pretty format."""
    pretty_packet = []
    for event in packet:
        pretty_event = []
        pretty_event.append('Event:')
        pretty_event.append('time %.6f,' % event[MTB.EV_TIME])
        if event.get(MTB.SYN_REPORT):
            pretty_event.append('-------------- SYN_REPORT ------------\n')
        else:
            ev_type = event[MTB.EV_TYPE]
            pretty_event.append('type %d (%s),' % (ev_type, EV_TYPES[ev_type]))
            ev_code = event[MTB.EV_CODE]
            pretty_event.append('code %d (%s),' %
                                 (ev_code, EV_STRINGS[ev_type][ev_code]))
            pretty_event.append('value %d' % event[MTB.EV_VALUE])
        pretty_packet.append(' '.join(pretty_event))
    return '\n'.join(pretty_packet)


def convert_to_evemu_format(packets):
    """Convert the text event format to the evemu format."""
    evemu_output = []
    evemu_format = 'E: %.6f %04x %04x %d'
    evemu_format_syn_report = 'E: %.6f 0000 0000 0'
    for packet in packets:
        for event in packet:
            if event.get(MTB.SYN_REPORT):
                evemu_event = evemu_format_syn_report % event[MTB.EV_TIME]
            else:
                evemu_event = evemu_format % (event[MTB.EV_TIME],
                                              event[MTB.EV_TYPE],
                                              event[MTB.EV_CODE],
                                              event[MTB.EV_VALUE])
            evemu_output.append(evemu_event)
    return evemu_output


def convert_mtplot_file_to_evemu_file(mtplot_file, evemu_dir=None,
                                      evemu_ext='.evemu', force=False):
    """Convert a mtplot event file to an evemu event file.

    Example:
       'one_finger_swipe.dat' is converted to 'one_finger_swipe.evemu.dat'
    """
    if not os.path.isfile(mtplot_file):
        print 'Error: there is no such file: "%s".' % mtplot_file
        return None

    # Convert mtplot event format to evemu event format.
    mtplot_packets = MtbParser().parse_file(mtplot_file)
    evemu_packets = convert_to_evemu_format(mtplot_packets)

    # Create the evemu file from the mtplot file.
    mtplot_dir, mtplot_filename = os.path.split(mtplot_file)
    mtplot_basename, mtplot_ext = os.path.splitext(mtplot_filename)

    evemu_dir = evemu_dir if evemu_dir else mtplot_dir
    evemu_file = (os.path.join(evemu_dir, mtplot_basename) + evemu_ext +
                  mtplot_ext)

    # Make sure that the file to be created does not exist yet unless force flag
    # is set to be True.
    if os.path.isfile(evemu_file) and not force:
        print 'Warning: the "%s" already exists. Quit.' % evemu_file
        return None

    # Write the converted evemu events to the evemu file.
    try:
        with open(evemu_file, 'w') as evemu_f:
            evemu_f.write('\n'.join(evemu_packets))
    except Exception as e:
        print 'Error: cannot write data to %s' % evemu_file
        return None

    return evemu_file


def create_final_state_packet(packets):
    """Given a sequence of packets, generate a packet representing
    the final state of events
    """
    def try_to_add(packet, event):
        """Try to add an event, if its value is not None, into the packet."""
        _, _, _, value = event
        if value is not None:
            packet.append(MtbParser.make_ev_dict(event))

    # Put the packets through a state machine to get the
    # final state of events
    sm = MtbStateMachine()
    for packet in packets:
        for event in packet:
            sm.add_event(event)
        sm.get_current_tid_data_for_all_tids()

    # Create the dummy packets representing the final state. We use
    # request_data_ready=False so that we still receive tid_packets
    # even if not all the events are populated (e.g. if a pressure
    # or position event is missing.)
    final_state_packet = []

    # It is possible that all fingers have left at this time instant.
    if sm.number_fingers == 0:
        return final_state_packet

    # Extract slot data from the snapshot of the state machine.
    syn_time = None
    for slot_data in sm.get_snapshot():
        syn_time, slot, tid, point, pressure = slot_data
        try_to_add(final_state_packet, (syn_time, EV_ABS, ABS_MT_SLOT, slot))
        try_to_add(final_state_packet,
                   (syn_time, EV_ABS, ABS_MT_TRACKING_ID, tid))
        try_to_add(final_state_packet,
                   (syn_time, EV_ABS, ABS_MT_POSITION_X, point.x))
        try_to_add(final_state_packet,
                   (syn_time, EV_ABS, ABS_MT_POSITION_Y, point.y))
        try_to_add(final_state_packet,
                   (syn_time, EV_ABS, ABS_MT_PRESSURE, pressure))

    # Add syn_report event to indicate the end of the packet
    if syn_time:
        final_state_packet.append(MtbParser.make_syn_report_ev_dict(syn_time))
    return final_state_packet


class MtbEvent:
    """Determine what an MTB event is.

    This class is just a bundle of a variety of classmethods about
    MTB event classification.
    """
    @classmethod
    def is_ABS_MT_TRACKING_ID(cls, event):
        """Is this event ABS_MT_TRACKING_ID?"""
        return (not event.get(MTB.SYN_REPORT) and
                event[MTB.EV_TYPE] == EV_ABS and
                event[MTB.EV_CODE] == ABS_MT_TRACKING_ID)

    @classmethod
    def is_new_contact(cls, event):
        """Is this packet generating new contact (Tracking ID)?"""
        return cls.is_ABS_MT_TRACKING_ID(event) and event[MTB.EV_VALUE] != -1

    @classmethod
    def is_finger_leaving(cls, event):
        """Is the finger is leaving in this packet?"""
        return cls.is_ABS_MT_TRACKING_ID(event) and event[MTB.EV_VALUE] == -1

    @classmethod
    def is_ABS_MT_SLOT(cls, event):
        """Is this packet ABS_MT_SLOT?"""
        return (not event.get(MTB.SYN_REPORT) and
                event[MTB.EV_TYPE] == EV_ABS and
                event[MTB.EV_CODE] == ABS_MT_SLOT)

    @classmethod
    def is_ABS_MT_POSITION_X(cls, event):
        """Is this packet ABS_MT_POSITION_X?"""
        return (not event.get(MTB.SYN_REPORT) and
                event[MTB.EV_TYPE] == EV_ABS and
                event[MTB.EV_CODE] == ABS_MT_POSITION_X)

    @classmethod
    def is_ABS_MT_POSITION_Y(cls, event):
        """Is this packet ABS_MT_POSITION_Y?"""
        return (not event.get(MTB.SYN_REPORT) and
                event[MTB.EV_TYPE] == EV_ABS and
                event[MTB.EV_CODE] == ABS_MT_POSITION_Y)

    @classmethod
    def is_ABS_MT_PRESSURE(self, event):
        """Is this packet ABS_MT_PRESSURE?"""
        return (not event.get(MTB.SYN_REPORT) and
                event[MTB.EV_TYPE] == EV_ABS and
                event[MTB.EV_CODE] == ABS_MT_PRESSURE)

    @classmethod
    def is_finger_data(cls, event):
        return (cls.is_ABS_MT_POSITION_X(event) or
                cls.is_ABS_MT_POSITION_Y(event) or
                cls.is_ABS_MT_PRESSURE(event))

    @classmethod
    def is_EV_KEY(cls, event):
        """Is this an EV_KEY event?"""
        return (not event.get(MTB.SYN_REPORT) and event[MTB.EV_TYPE] == EV_KEY)

    @classmethod
    def is_BTN_LEFT(cls, event):
        """Is this event BTN_LEFT?"""
        return (cls.is_EV_KEY(event) and event[MTB.EV_CODE] == BTN_LEFT)

    @classmethod
    def is_BTN_LEFT_value(cls, event, value):
        """Is this event BTN_LEFT with value equal to the specified value?"""
        return (cls.is_BTN_LEFT(event) and event[MTB.EV_VALUE] == value)

    @classmethod
    def is_BTN_TOOL_FINGER(cls, event):
        """Is this event BTN_TOOL_FINGER?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOOL_FINGER)

    @classmethod
    def is_BTN_TOOL_DOUBLETAP(cls, event):
        """Is this event BTN_TOOL_DOUBLETAP?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOOL_DOUBLETAP)

    @classmethod
    def is_BTN_TOOL_TRIPLETAP(cls, event):
        """Is this event BTN_TOOL_TRIPLETAP?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOOL_TRIPLETAP)

    @classmethod
    def is_BTN_TOOL_QUADTAP(cls, event):
        """Is this event BTN_TOOL_QUADTAP?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOOL_QUADTAP)

    @classmethod
    def is_BTN_TOOL_QUINTTAP(cls, event):
        """Is this event BTN_TOOL_QUINTTAP?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOOL_QUINTTAP)

    @classmethod
    def is_BTN_TOUCH(cls, event):
        """Is this event BTN_TOUCH?"""
        return (cls.is_EV_KEY(event) and
                event[MTB.EV_CODE] == BTN_TOUCH)

    @classmethod
    def is_SYN_REPORT(self, event):
        """Determine if this event is SYN_REPORT."""
        return event.get(MTB.SYN_REPORT, False)


class MtbEvemu:
    """A simplified class provides MTB utilities for evemu event format."""
    def __init__(self, device):
        self.mtb = Mtb(device=device)
        self.num_tracking_ids = 0

    def _convert_event(self, event):
        (tv_sec, tv_usec, ev_type, ev_code, ev_value) = event
        ev_time = float(tv_sec + tv_usec * 0.000001)
        return MtbParser.make_ev_dict((ev_time, ev_type, ev_code, ev_value))

    def all_fingers_leaving(self):
        """Is there no finger on the touch device?"""
        return self.num_tracking_ids <= 0

    def process_event(self, event):
        """Process the event and count existing fingers."""
        converted_event = self._convert_event(event)
        if MtbEvent.is_new_contact(converted_event):
            self.num_tracking_ids += 1
        elif MtbEvent.is_finger_leaving(converted_event):
            self.num_tracking_ids -= 1


class MtbSanity:
    """Sanity check for MTB format correctness.

    The rules to check the sanity will accumulate over time. However, let's
    begin with some simple checks:
    - A finger should not leave before arriving. (Should not set -1 to the
      tracking ID before assigning a positive value to the slot first.)
    - Should not assign X, Y, or PRESSURE data to a slot without a positive
      tracking ID.
    """
    ERR_FINGER_LEAVE = 'FINGER_LEAVING_BEFORE_ARRIVING'
    ERR_ASSIGN_ATTR = 'ASSIGN_ATTRIBUTES_BEFORE_ARRIVING'

    def __init__(self, packets):
        self.packets = packets

    def check(self):
        """Do some sanity checks.

        Note that it takes care of the case that X and Y events may come earlier
        than the TRACKING ID event. We consider it as valid.

        Event: time, type 3 (EV_ABS), code 47 (ABS_MT_SLOT), value 1
        Event: time, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 2632
        Event: time, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 288
        Event: time, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 1017
        ...

        In this case, X and Y events are stored in tmp_errors. When TRACKING ID
        event shows up in the packet, it removes the errors associated with
        this slot 1.
        """
        errors = {self.ERR_FINGER_LEAVE: 0, self.ERR_ASSIGN_ATTR: 0}

        def _errors_factory():
            return copy.deepcopy(
                    {self.ERR_FINGER_LEAVE: 0, self.ERR_ASSIGN_ATTR: 0})

        curr_slot_id = 0
        slot_to_tid = {curr_slot_id: -1}

        for packet in self.packets:
            tmp_errors = defaultdict(_errors_factory)
            for event in packet:
                # slot event
                if MtbEvent.is_ABS_MT_SLOT(event):
                    curr_slot_id = event[MTB.EV_VALUE]
                    if curr_slot_id not in slot_to_tid:
                        slot_to_tid.update({curr_slot_id: -1})

                # finger arriving
                elif MtbEvent.is_new_contact(event):
                    slot_to_tid.update({curr_slot_id: event[MTB.EV_VALUE]})
                    if curr_slot_id in tmp_errors:
                        del tmp_errors[curr_slot_id]

                # finger leaving
                elif MtbEvent.is_finger_leaving(event):
                    if slot_to_tid.get(curr_slot_id, -1) == -1:
                        tmp_errors[curr_slot_id][self.ERR_FINGER_LEAVE] += 1
                    else:
                        slot_to_tid[curr_slot_id] = -1

                # access a slot attribute before finger arriving
                elif MtbEvent.is_finger_data(event):
                    if slot_to_tid.get(curr_slot_id, -1) == -1:
                        tmp_errors[curr_slot_id][self.ERR_ASSIGN_ATTR] += 1

                # At SYN_REPORT, accumulate how many errors occur.
                elif MtbEvent.is_SYN_REPORT(event):
                    for _, slot_errors in tmp_errors.items():
                        for err_string, err_count in slot_errors.items():
                            errors[err_string] += err_count

        return errors


class MtbStateMachine:
    """The state machine for MTB events.

    It traces the slots, tracking IDs, x coordinates, y coordinates, etc. If
    these values are not changed explicitly, the values are kept across events.

    Note that the kernel driver only reports what is changed. Due to its
    internal state machine, it is possible that either x or y in
    self.point[tid] is None initially even though the instance has been created.
    """
    def __init__(self):
        # Set the default slot to 0 as it may not be displayed in the MTB events
        #
        # Some abnormal event files may not display the tracking ID in the
        # beginning. To handle this situation, we need to initialize
        # the following variables:  slot_to_tid, point
        #
        # As an example, refer to the following event file which is one of
        # the golden samples with this problem.
        #   tests/data/stationary_finger_shift_with_2nd_finger_tap.dat
        self.tid = None
        self.slot = 0
        self.slot_to_tid = {self.slot: self.tid}
        self.point = {self.tid: Point()}
        self.pressure = {self.tid: None}
        self.syn_time = None
        self.new_tid = False
        self.number_fingers = 0
        self.leaving_slots = []

    def _del_leaving_slots(self):
        """Delete the leaving slots. Remove the slots and their tracking IDs."""
        for slot in self.leaving_slots:
            del self.slot_to_tid[slot]
            self.number_fingers -= 1
        self.leaving_slots = []

    def _add_new_tracking_id(self, tid):
        self.tid = tid
        self.slot_to_tid[self.slot] = self.tid
        self.new_tid = True
        self.point[self.tid] = Point()
        self.pressure[self.tid] = None
        self.number_fingers += 1

    def add_event(self, event):
        """Update the internal states with the event.

        @param event: an MTB event
        """
        self.new_tid = False

        # Switch the slot.
        if MtbEvent.is_ABS_MT_SLOT(event):
            self.slot = event[MTB.EV_VALUE]

        # Get a new tracking ID.
        elif MtbEvent.is_new_contact(event):
            self._add_new_tracking_id(event[MTB.EV_VALUE])

        # A slot is leaving.
        # Do not delete this slot until this last packet is reported.
        elif MtbEvent.is_finger_leaving(event):
            self.leaving_slots.append(self.slot)

        else:
            # Gracefully handle the case where we weren't given a tracking id
            # by using a default value for these mystery fingers
            if (not self.slot in self.slot_to_tid and
                MtbEvent.is_finger_data(event)):
                self._add_new_tracking_id(999999)

            # Update x value.
            if MtbEvent.is_ABS_MT_POSITION_X(event):
                self.point[self.slot_to_tid[self.slot]].x = event[MTB.EV_VALUE]

            # Update y value.
            elif MtbEvent.is_ABS_MT_POSITION_Y(event):
                self.point[self.slot_to_tid[self.slot]].y = event[MTB.EV_VALUE]

            # Update z value (pressure)
            elif MtbEvent.is_ABS_MT_PRESSURE(event):
                self.pressure[self.slot_to_tid[self.slot]] = event[MTB.EV_VALUE]

            # Use the SYN_REPORT time as the packet time
            elif MtbEvent.is_SYN_REPORT(event):
                self.syn_time = event[MTB.EV_TIME]

    def get_snapshot(self):
        """Get the snapshot of current data of all slots."""
        slots_data = []
        for slot, tid in self.slot_to_tid.items():
            slot_data = [self.syn_time, slot, tid,
                         self.point.get(tid), self.pressure.get(tid)]
            slots_data.append(slot_data)
        return slots_data

    def get_current_tid_data_for_all_tids(self, request_data_ready=True):
        """Get current packet's tid data including the point, the pressure, and
        the syn_time for all tids.

        @param request_data_ready: if set to true, it will not output
                current_tid_data until all data including x, y, pressure,
                syn_time, etc. in the packet have been assigned.
        """
        current_tid_data = []
        for slot, tid in self.slot_to_tid.items():
            point = copy.deepcopy(self.point.get(tid))
            pressure = self.pressure.get(tid)
            # Check if all attributes are non-None values.
            # Note: we cannot use
            #           all([all(point.value()), pressure, self.syn_time])
            #       E.g., for a point = (0, 300), it will return False
            #       which is not what we want. We want it to return False
            #       only when there are None values.
            data_ready = all(map(lambda e: e is not None,
                             list(point.value()) + [pressure, self.syn_time]))

            if (not request_data_ready) or data_ready:
                tid_packet = TidPacket(self.syn_time, point, pressure)
            else:
                tid_packet = None
            # Even tid_packet is None, we would like to report this tid so that
            # its client function get_ordered_finger_paths() could construct
            # an ordered dictionary correctly based on the tracking ID.
            current_tid_data.append((tid, slot, tid_packet))

        self._del_leaving_slots()

        return sorted(current_tid_data)


class Mtb:
    """An MTB class providing MTB format related utility methods."""
    LEN_MOVING_AVERAGE = 2
    LEVEL_JUMP_RATIO = 3
    LEVEL_JUMP_MAXIUM_ALLOWED = 10
    LEN_DISCARD = 5

    def __init__(self, device=None, packets=None):
        self.device = device
        self.packets = packets
        self._define_check_event_func_list()

    def _define_check_event_func_list(self):
        """Define event function lists for various event cycles below."""
        self.check_event_func_list = {}
        self.MAX_FINGERS = 5
        # One-finger touching the device should generate the following events:
        #     BTN_TOUCH, and BTN_TOOL_FINGER: 0 -> 1 -> 0
        self.check_event_func_list[1] = [MtbEvent.is_BTN_TOUCH,
                                         MtbEvent.is_BTN_TOOL_FINGER]

        # Two-finger touching the device should generate the following events:
        #     BTN_TOUCH, and BTN_TOOL_DOUBLETAP: 0 -> 1 -> 0
        self.check_event_func_list[2] = [MtbEvent.is_BTN_TOUCH,
                                         MtbEvent.is_BTN_TOOL_DOUBLETAP]

        # Three-finger touching the device should generate the following events:
        #     BTN_TOUCH, and BTN_TOOL_TRIPLETAP: 0 -> 1 -> 0
        self.check_event_func_list[3] = [MtbEvent.is_BTN_TOUCH,
                                         MtbEvent.is_BTN_TOOL_TRIPLETAP]

        # Four-finger touching the device should generate the following events:
        #     BTN_TOUCH, and BTN_TOOL_QUADTAP: 0 -> 1 -> 0
        self.check_event_func_list[4] = [MtbEvent.is_BTN_TOUCH,
                                         MtbEvent.is_BTN_TOOL_QUADTAP]

        # Five-finger touching the device should generate the following events:
        #     BTN_TOUCH, and BTN_TOOL_QUINTTAP: 0 -> 1 -> 0
        self.check_event_func_list[5] = [MtbEvent.is_BTN_TOUCH,
                                         MtbEvent.is_BTN_TOOL_QUINTTAP]

        # Physical click should generate the following events:
        #     BTN_LEFT: 0 -> 1 -> 0
        self.check_event_func_click = [MtbEvent.is_BTN_LEFT,]



    def _calc_movement_for_axis(self, x, prev_x):
        """Calculate the distance moved in an axis."""
        return abs(x - prev_x) if prev_x is not None else 0

    def _calc_distance(self, (x0, y0), (x1, y1)):
        """Calculate the distance in pixels between two points."""
        dist_x = x1 - x0
        dist_y = y1 - y0
        return math.sqrt(dist_x * dist_x + dist_y * dist_y)

    def _init_dict(self, keys, value):
        """Initialize a dictionary over the keys with the same given value.

        Note: The following command does not always work:
                    dict.fromkeys(keys, value)
              It works when value is a simple type, e.g., an integer.
              However, if value is [] or {}, it does not work correctly.
              The reason is that if the value is [] or {}, all the keys would
              point to the same list or dictionary, which is not expected
              in most cases.
        """
        return dict([(key, copy.deepcopy(value)) for key in keys])

    def get_number_contacts(self):
        """Get the number of contacts (Tracking IDs)."""
        num_contacts = 0
        for packet in self.packets:
            for event in packet:
                if MtbEvent.is_new_contact(event):
                    num_contacts += 1
        return num_contacts

    def get_x_y(self, target_slot):
        """Extract x and y positions in the target slot."""
        # The default slot is slot 0 if no slot number is assigned.
        # The rationale is that evdev is a state machine. It only reports
        # the change. Slot 0 would not be reported by evdev if last time
        # the last finger left the touch device was at slot 0.
        slot = 0

        # Should not write "list_x = list_y = []" below.
        # They would end up with pointing to the same list.
        list_x = []
        list_y = []
        prev_x = prev_y = None
        target_slot_live = False
        initial_default_slot_0 = True
        for packet in self.packets:
            if (slot == target_slot and slot == 0 and not target_slot_live and
                initial_default_slot_0):
                target_slot_live = True
                initial_default_slot_0 = False
            for event in packet:
                if MtbEvent.is_ABS_MT_SLOT(event):
                    slot = event[MTB.EV_VALUE]
                    if slot == target_slot and not target_slot_live:
                        target_slot_live = True
                if slot != target_slot:
                    continue

                # Update x value if available.
                if MtbEvent.is_ABS_MT_POSITION_X(event):
                    prev_x = event[MTB.EV_VALUE]
                # Update y value if available.
                elif MtbEvent.is_ABS_MT_POSITION_Y(event):
                    prev_y = event[MTB.EV_VALUE]
                # Check if the finger at the target_slot is leaving.
                elif MtbEvent.is_finger_leaving(event):
                    target_slot_live = False

            # If target_slot is alive, and both x and y have
            # been assigned values, append the x and y to the list no matter
            # whether x or y position is reported in the current packet.
            # This also handles the initial condition that no previous x or y
            # is reported yet.
            if target_slot_live and prev_x and prev_y:
                list_x.append(prev_x)
                list_y.append(prev_y)
        return (list_x, list_y)

    def get_ordered_finger_paths(self, request_data_ready=True):
        """Construct the finger paths ordered by the occurrences of the
        finger contacts.

        @param request_data_ready: if set to true, it will not output the
                tid_data in a packet until all data including x, y, pressure,
                syn_time, etc. in the packet have been assigned.

        The finger_paths mapping the tid to its finger_path looks like
            {tid1: finger_path1,
             tid2: finger_path2,
             ...
            }
        where every tid represents a finger.

        A finger_path effectively consists of a list of tid_packets of the same
        tid in the event file. An example of its structure looks like
        finger_path:
            slot=0
            tid_packets = [tid_packet0, tid_packet1, tid_packet2, ...]

        A tid_packet looks like
            [100021.342104,         # syn_time
             (66, 100),             # point
             56,                    # pressure
             ...                    # maybe more attributes added later.
            ]

        This method is applicable when fingers are contacting and leaving
        the touch device continuously. The same slot number, e.g., slot 0 or
        slot 1, may be used for multiple times.

        Note that the finger contact starts at 0. The finger contacts look to
        be equal to the slot numbers in practice. However, this assumption
        seems not enforced in any document. For safety, we use the ordered
        finger paths dict here to guarantee that we could access the ith finger
        contact path data correctly.

        Also note that we do not sort finger paths by tracking IDs to derive
        the ordered dict because tracking IDs may wrap around.
        """
        # ordered_finger_paths_dict is an ordered dictionary of
        #     {tid: FingerPath}
        ordered_finger_paths_dict = OrderedDict()
        sm = MtbStateMachine()
        for packet in self.packets:
            # Inject events into the state machine to update its state.
            for event in packet:
                sm.add_event(event)

            # If there are N fingers (tids) in a packet, we will have
            # N tid_packet's in the current packet. The loop below is to
            # append every tid_packet into its corresponding finger_path for
            # every tracking id in the current packet.
            for tid, slot, tid_packet in sm.get_current_tid_data_for_all_tids(
                    request_data_ready):
                finger_path = ordered_finger_paths_dict.setdefault(
                        tid, FingerPath(slot, []))
                if tid_packet:
                    finger_path.tid_packets.append(tid_packet)

        return ordered_finger_paths_dict

    def get_ordered_finger_path(self, finger, attr):
        """Extract the specified attribute from the packets of the ith finger
        contact.

        @param finger: the ith finger contact
        @param attr: an attribute in a tid packet which could be either
                'point', 'pressure', or 'syn_time'
        """
        # finger_paths is a list ordered by the occurrences of finger contacts
        finger_paths = self.get_ordered_finger_paths().values()
        if finger < len(finger_paths) and finger >= 0:
            finger_path = finger_paths[finger]
            return finger_path.get(attr)
        return []

    def get_slot_data(self, slot, attr):
        """Extract the attribute data of the specified slot.

        @param attr: an attribute in a tid packet which could be either
                'point', 'pressure', or 'syn_time'
        """
        for finger_path in self.get_ordered_finger_paths().values():
            if finger_path.slot == slot:
                return finger_path.get(attr)
        return []

    def get_max_distance_of_all_tracking_ids(self):
        """Get the max moving distance of all tracking IDs."""
        return max(max(get_two_min_radii(finger_path.get('point'))
            for finger_path in self.get_ordered_finger_paths().values()))

    def get_list_of_rocs_of_all_tracking_ids(self):
        """For each tracking ID, compute the minimal enclosing circles.

        Return a list of radii of such minimal enclosing circles of
        all tracking IDs.
        Note: rocs denotes the radii of circles
        """
        list_rocs = []
        for finger_path in self.get_ordered_finger_paths().values():
            # Convert the point coordinates in pixels to in mms.
            points_in_mm = [Point(*self.device.pixel_to_mm(p.value()))
                            for p in finger_path.get('point')]
            list_rocs += get_two_min_radii(points_in_mm)
        return list_rocs

    def get_x_y_multiple_slots(self, target_slots):
        """Extract points in multiple slots.

        Only the packets with all specified slots are extracted.
        This is useful to collect packets for pinch to zoom.
        """
        # Initialize slot_exists dictionary to False
        slot_exists = dict.fromkeys(target_slots, False)

        # Set the initial slot number to 0 because evdev is a state machine,
        # and may not present slot 0.
        slot = 0
        # Initialize the following dict to []
        # Don't use "dict.fromkeys(target_slots, [])"
        list_x = self._init_dict(target_slots, [])
        list_y = self._init_dict(target_slots, [])
        x = self._init_dict(target_slots, None)
        y = self._init_dict(target_slots, None)
        for packet in self.packets:
            for event in packet:
                if MtbEvent.is_ABS_MT_SLOT(event):
                    slot = event[MTB.EV_VALUE]
                if slot not in target_slots:
                    continue

                if MtbEvent.is_ABS_MT_TRACKING_ID(event):
                    if MtbEvent.is_new_contact(event):
                        slot_exists[slot] = True
                    elif MtbEvent.is_finger_leaving(event):
                        slot_exists[slot] = False
                elif MtbEvent.is_ABS_MT_POSITION_X(event):
                    x[slot] = event[MTB.EV_VALUE]
                elif MtbEvent.is_ABS_MT_POSITION_Y(event):
                    y[slot] = event[MTB.EV_VALUE]

            # Note:
            # - All slot_exists must be True to append x, y positions for the
            #   slots.
            # - All x and y values for all slots must have been reported once.
            #   (This handles the initial condition that no previous x or y
            #    is reported yet.)
            # - If either x or y positions are reported in the current packet,
            #   append x and y to the list of that slot.
            #   (This handles the condition that only x or y is reported.)
            # - Even in the case that neither x nor y is reported in current
            #   packet, cmt driver constructs and passes hwstate to gestures.
            if (all(slot_exists.values()) and all(x.values()) and
                all(y.values())):
                for s in target_slots:
                    list_x[s].append(x[s])
                    list_y[s].append(y[s])

        return (list_x, list_y)

    def get_points_multiple_slots(self, target_slots):
        """Get the points in multiple slots."""
        list_x, list_y = self.get_x_y_multiple_slots(target_slots)
        points_list = [zip(list_x[slot], list_y[slot]) for slot in target_slots]
        points_dict = dict(zip(target_slots, points_list))
        return points_dict

    def get_relative_motion(self, target_slots):
        """Get the relative motion of the two target slots."""
        # The slots in target_slots could be (0, 1), (1, 2) or other
        # possibilities.
        slot_a, slot_b = target_slots
        points_dict = self.get_points_multiple_slots(target_slots)
        points_slot_a = points_dict[slot_a]
        points_slot_b = points_dict[slot_b]

        # if only 0 or 1 point observed, the relative motion is 0.
        if len(points_slot_a) <= 1 or len(points_slot_b) <= 1:
            return 0

        distance_begin = self._calc_distance(points_slot_a[0], points_slot_b[0])
        distance_end = self._calc_distance(points_slot_a[-1], points_slot_b[-1])
        relative_motion = distance_end - distance_begin
        return relative_motion

    def get_points(self, target_slot):
        """Get the points in the target slot."""
        list_x, list_y = self.get_x_y(target_slot)
        return zip(list_x, list_y)

    def get_distances(self, target_slot):
        """Get the distances of neighbor points in the target slot."""
        points = self.get_points(target_slot)
        distances = []
        for index in range(len(points) - 1):
            distance = self._calc_distance(points[index], points[index + 1])
            distances.append(distance)
        return distances

    def get_range(self):
        """Get the min and max values of (x, y) positions."""
        min_x = min_y = float('infinity')
        max_x = max_y = float('-infinity')
        for packet in self.packets:
            for event in packet:
                if MtbEvent.is_ABS_MT_POSITION_X(event):
                    x = event[MTB.EV_VALUE]
                    min_x = min(min_x, x)
                    max_x = max(max_x, x)
                elif MtbEvent.is_ABS_MT_POSITION_Y(event):
                    y = event[MTB.EV_VALUE]
                    min_y = min(min_y, y)
                    max_y = max(max_y, y)
        return (min_x, max_x, min_y, max_y)

    def get_total_motion(self, target_slot):
        """Get the total motion in the target slot."""
        prev_x = prev_y = None
        accu_x = accu_y = 0
        slot = None
        for packet in self.packets:
            for event in packet:
                if MtbEvent.is_ABS_MT_SLOT(event):
                    slot = event[MTB.EV_VALUE]
                elif (MtbEvent.is_ABS_MT_POSITION_X(event) and
                      slot == target_slot):
                    x = event[MTB.EV_VALUE]
                    accu_x += self._calc_movement_for_axis(x, prev_x)
                    prev_x = x
                elif (MtbEvent.is_ABS_MT_POSITION_Y(event) and
                      slot == target_slot):
                    y = event[MTB.EV_VALUE]
                    accu_y += self._calc_movement_for_axis(y, prev_y)
                    prev_y = y
        return (accu_x, accu_y)

    def get_max_distance(self, slot, unit):
        """Get the max distance between any two points of the specified slot."""
        points = self.get_slot_data(slot, 'point')
        return self.get_max_distance_from_points(points, unit)

    def get_max_distance_from_points(self, points, unit):
        """Get the max distance between any two points."""
        two_farthest_points = get_two_farthest_points(points)
        if len(two_farthest_points) < 2:
            return 0

        # Convert the point coordinates from pixel to mm if necessary.
        if unit == UNIT.MM:
            p1, p2 = [Point(*self.device.pixel_to_mm(p.value()))
                      for p in two_farthest_points]
        else:
            p1, p2 = two_farthest_points

        return p1.distance(p2)

    def get_largest_gap_ratio(self, target_slot):
        """Get the largest gap ratio in the target slot.

        gap_ratio_with_prev = curr_gap / prev_gap
        gap_ratio_with_next = curr_gap / next_gap

        This function tries to find the largest gap_ratio_with_prev
        with the restriction that gap_ratio_with_next is larger than
        RATIO_THRESHOLD_CURR_GAP_TO_NEXT_GAP.

        The ratio threshold is used to prevent the gaps detected in a swipe.
        Note that in a swipe, the gaps tends to become larger and larger.
        """
        RATIO_THRESHOLD_CURR_GAP_TO_NEXT_GAP = 1.2
        GAP_LOWER_BOUND = 10

        gaps = self.get_distances(target_slot)
        gap_ratios = []
        largest_gap_ratio = float('-infinity')
        for index in range(1, len(gaps) - 1):
            prev_gap = max(gaps[index - 1], 1)
            curr_gap = gaps[index]
            next_gap = max(gaps[index + 1], 1)
            gap_ratio_with_prev = curr_gap / prev_gap
            gap_ratio_with_next = curr_gap / next_gap
            if (curr_gap >= GAP_LOWER_BOUND and
                gap_ratio_with_prev > largest_gap_ratio and
                gap_ratio_with_next > RATIO_THRESHOLD_CURR_GAP_TO_NEXT_GAP):
                largest_gap_ratio = gap_ratio_with_prev

        return largest_gap_ratio

    def _is_large(self, numbers, index):
        """Is the number at the index a large number compared to the moving
        average of the previous LEN_MOVING_AVERAGE numbers? This is used to
        determine if a distance is a level jump."""
        if index < self.LEN_MOVING_AVERAGE + 1:
            return False
        moving_sum = sum(numbers[index - self.LEN_MOVING_AVERAGE : index])
        moving_average = float(moving_sum) / self.LEN_MOVING_AVERAGE
        cond1 = numbers[index] >= self.LEVEL_JUMP_RATIO * moving_average
        cond2 = numbers[index] >= self.LEVEL_JUMP_MAXIUM_ALLOWED
        return cond1 and cond2

    def _accumulate_level_jumps(self, level_jumps):
        """Accumulate level jumps."""
        if not level_jumps:
            return []

        is_positive = level_jumps[0] > 0
        tmp_sum = 0
        accumulated_level_jumps = []
        for jump in level_jumps:
            # If current sign is the same as previous ones, accumulate it.
            if (jump > 0) is is_positive:
                tmp_sum += jump
            # If current jump has changed its sign, reset the tmp_sum to
            # accumulate the level_jumps onwards.
            else:
                accumulated_level_jumps.append(tmp_sum)
                tmp_sum = jump
                is_positive = not is_positive

        if tmp_sum != 0:
            accumulated_level_jumps.append(tmp_sum)
        return accumulated_level_jumps

    def get_largest_accumulated_level_jumps(self, displacements):
        """Get the largest accumulated level jumps in displacements."""
        largest_accumulated_level_jumps = 0
        if len(displacements) < self.LEN_MOVING_AVERAGE + 1:
            return largest_accumulated_level_jumps

        # Truncate some elements at both ends of the list which are not stable.
        displacements = displacements[self.LEN_DISCARD: -self.LEN_DISCARD]
        distances = map(abs, displacements)

        # E.g., displacements= [5, 6, 5, 6, 20, 3, 5, 4, 6, 21, 4, ...]
        #       level_jumps = [20, 21, ...]
        level_jumps = [disp for i, disp in enumerate(displacements)
                       if self._is_large(distances, i)]

        # E.g., level_jumps= [20, 21, -18, -25, 22, 18, -19]
        #       accumulated_level_jumps = [41, -43, 40, -19]
        #       largest_accumulated_level_jumps = 43
        accumulated_level_jumps = self._accumulate_level_jumps(level_jumps)
        if accumulated_level_jumps:
            abs_accumulated_level_jumps = map(abs, accumulated_level_jumps)
            largest_accumulated_level_jumps = max(abs_accumulated_level_jumps)

        return largest_accumulated_level_jumps

    def get_displacement(self, target_slot):
        """Get the displacement in the target slot."""
        displace = [map(lambda p0, p1: p1 - p0, axis[:len(axis) - 1], axis[1:])
                    for axis in self.get_x_y(target_slot)]
        displacement_dict = dict(zip((AXIS.X, AXIS.Y), displace))
        return displacement_dict

    def calc_displacement(self, numbers):
        """Calculate the displacements in a list of numbers."""
        if len(numbers) <= 1:
            return []
        return [numbers[i + 1] - numbers[i] for i in range(len(numbers) - 1)]

    def get_displacements_for_slots(self, min_slot):
        """Get the displacements for slots >= min_slot."""
        finger_paths = self.get_ordered_finger_paths()

        # Remove those tracking IDs with slots < min_slot
        for tid, finger_path in finger_paths.items():
            if finger_path.slot < min_slot:
                del finger_paths[tid]

        # Calculate the displacements of the coordinates in the tracking IDs.
        displacements = defaultdict(dict)
        for tid, finger_path in finger_paths.items():
            finger_path_values = [p.value() for p in finger_path.get('point')]
            if finger_path_values:
                list_x, list_y = zip(*finger_path_values)
            else:
                list_x, list_y = [], []
            displacements[tid][MTB.SLOT] = finger_path.slot
            displacements[tid][AXIS.X] = self.calc_displacement(list_x)
            displacements[tid][AXIS.Y] = self.calc_displacement(list_y)

        return displacements

    def _get_segments_by_length(self, src_list, segment_flag, ratio):
        """Get the segments based on segment_flag and the ratio of the
        src_list length (size).

        @param src_list: could be list_x, list_y, or list_t
        @param segment_flag: indicating which segment (the begin, the end, or
                the middle segment) to extract
        @param ratio: the ratio of the time interval of the segment
        """
        end_size = int(round(len(src_list) * ratio))
        if segment_flag == VAL.WHOLE:
            return src_list
        elif segment_flag == VAL.MIDDLE:
            return src_list[end_size: -end_size]
        elif segment_flag == VAL.BEGIN:
            return src_list[: end_size]
        elif segment_flag == VAL.END:
            return src_list[-end_size:]
        elif segment_flag == VAL.BOTH_ENDS:
            bgn_segment = src_list[: end_size]
            end_segment = src_list[-end_size:]
            return bgn_segment + end_segment
        else:
            return None

    def get_segments_x_and_y(self, ax, ay, segment_flag, ratio):
        """Get the segments for both x and y axes."""
        segment_x = self._get_segments_by_length(ax, segment_flag, ratio)
        segment_y = self._get_segments_by_length(ay, segment_flag, ratio)
        return (segment_x, segment_y)

    def get_segments_by_distance(self, list_t, list_coord, segment_flag, ratio):
        """Partition list_coord into the begin, the middle, and the end
        segments based on segment_flag and the ratio. And then use the
        derived indexes to partition list_t.

        @param list_t: the list of syn_report time instants
        @param list_coord: could be list_x, list_y
        @param segment_flag: indicating which segment (the being, the end, or
                the middle segment) to extract
        @param ratio: the ratio of the distance of the begin/end segment
                with the value between 0.0 and 1.0
        """
        def _find_boundary_index(list_coord, boundary_distance):
            """Find the boundary index i such that
               abs(list_coord[i] - list_coord[0]) > boundary_distance

            @param list_coord: a list of coordinates
            @param boundary_distance: the min distance between boundary_coord
                    and list_coord[0]
            """
            for i, c in enumerate(list_coord):
                if abs(c - list_coord[0]) > boundary_distance:
                    return i

        end_to_end_distance = abs(list_coord[-1] - list_coord[0])
        first_idx_mid_seg = _find_boundary_index(
                list_coord, end_to_end_distance * ratio)
        last_idx_mid_seg = _find_boundary_index(
                list_coord, end_to_end_distance * (1 - ratio))

        if segment_flag == VAL.WHOLE:
            segment_coord = list_coord
            segment_t = list_t
        elif segment_flag == VAL.MIDDLE:
            segment_coord = list_coord[first_idx_mid_seg:last_idx_mid_seg]
            segment_t = list_t[first_idx_mid_seg:last_idx_mid_seg]
        elif segment_flag == VAL.BEGIN:
            segment_coord = list_coord[:first_idx_mid_seg]
            segment_t = list_t[:first_idx_mid_seg]
        elif segment_flag == VAL.END:
            segment_coord = list_coord[last_idx_mid_seg:]
            segment_t = list_t[last_idx_mid_seg:]
        else:
            segment_coord = []
            segment_t = []

        return (segment_t, segment_coord)

    def get_segments(self, list_t, list_coord, segment_flag, ratio):
        """Partition list_t and list_coord into the segments specified by
        the segment_flag and the ratio.

        If the list_coord stretches long enough, it represents a normal
        line. We will partition list_t and list_coord by distance.

        On the other hand, if the min and max values in list_coord are
        close to each other, it probably does not represent a line. We will
        partition list_t and list_coord by time in this case.
        E.g., in the follow cases, it is better to partition using length
              instead of using distance.
        list_coord = [105, 105, 105, 105, 105, 105, 105, 105, 105, 105] or
        list_coord = [104, 105, 105, 105, 105, 105, 105, 105, 105, 105] or
        list_coord = [105, 105, 105, 105, 105, 105, 105, 105, 105, 106]

        @param list_t: the list of syn_report time instants
        @param list_coord: could be list_x, list_y
        @param segment_flag: indicating which segment (the being, the end, or
                the middle segment) to extract
        @param ratio: the ratio of the distance of the begin/end segment
                with the value between 0.0 and 1.0
        """
        MIN_STRAIGHT_LINE_DIST = 20
        if (max(list_coord) - min(list_coord)) > MIN_STRAIGHT_LINE_DIST:
            return self.get_segments_by_distance(list_t, list_coord,
                                                 segment_flag, ratio)
        else:
            return (self._get_segments_by_length(list_t, segment_flag, ratio),
                    self._get_segments_by_length(list_coord, segment_flag,
                                                 ratio))

    def get_reversed_motions(self, target_slot, direction,
                             segment_flag=VAL.WHOLE, ratio=None):
        """Get the total reversed motions in the specified direction
           in the target slot.

        Only the reversed motions specified by the segment_flag are taken.
        The segment_flag could be
          VAL.BEGIN: the begin segment
          VAL.MIDDLE : the middle segment
          VAL.END : the end segment
          VAL.BOTH_ENDS : the segments at both ends
          VAL.WHOLE: the whole line

        The ratio represents the ratio of the BEGIN or END segment to the whole
        segment.

        If direction is in HORIZONTAL_DIRECTIONS, consider only x axis.
        If direction is in VERTICAL_DIRECTIONS, consider only y axis.
        If direction is in DIAGONAL_DIRECTIONS, consider both x and y axes.

        Assume that the displacements in GV.LR (moving from left to right)
        in the X axis are:

            [10, 12, 8, -9, -2, 6, 8, 11, 12, 5, 2]

        Its total reversed motion = (-9) + (-2) = -11
        """
        # Define the axis moving directions dictionary
        POSITIVE = 'positive'
        NEGATIVE = 'negative'
        AXIS_MOVING_DIRECTIONS = {
            GV.LR: {AXIS.X: POSITIVE},
            GV.RL: {AXIS.X: NEGATIVE},
            GV.TB: {AXIS.Y: POSITIVE},
            GV.BT: {AXIS.Y: NEGATIVE},
            GV.CR: {AXIS.X: POSITIVE},
            GV.CL: {AXIS.X: NEGATIVE},
            GV.CB: {AXIS.Y: POSITIVE},
            GV.CT: {AXIS.Y: NEGATIVE},
            GV.BLTR: {AXIS.X: POSITIVE, AXIS.Y: NEGATIVE},
            GV.BRTL: {AXIS.X: NEGATIVE, AXIS.Y: NEGATIVE},
            GV.TRBL: {AXIS.X: NEGATIVE, AXIS.Y: POSITIVE},
            GV.TLBR: {AXIS.X: POSITIVE, AXIS.Y: POSITIVE},
        }

        axis_moving_directions = AXIS_MOVING_DIRECTIONS.get(direction)
        func_positive = lambda n: n > 0
        func_negative = lambda n: n < 0
        reversed_functions = {POSITIVE: func_negative, NEGATIVE: func_positive}
        displacement_dict = self.get_displacement(target_slot)
        reversed_motions = {}
        for axis in AXIS.LIST:
            axis_moving_direction = axis_moving_directions.get(axis)
            if axis_moving_direction is None:
                continue
            displacement = displacement_dict[axis]
            displacement_segment = self._get_segments_by_length(
                    displacement, segment_flag, ratio)
            reversed_func = reversed_functions[axis_moving_direction]
            reversed_motions[axis] = sum(filter(reversed_func,
                                                displacement_segment))
        return reversed_motions

    def get_num_packets(self, target_slot):
        """Get the number of packets in the target slot."""
        list_x, list_y = self.get_x_y(target_slot)
        return len(list_x)

    def get_list_syn_time(self, finger):
        """Get the list of syn_time instants from the packets of the ith finger
        contact if finger is not None. Otherwise, use all packets.

        @param finger : the specified ith finger contact.
                If a finger contact is specified, extract only the list of
                syn_time from this finger contact.
                Otherwise, when the finger contact is set to None, take all
                packets into account. Note that the finger contact number
                starts from 0.

        Note: the last event in a packet, represented as packet[-1], is
              'SYN_REPORT' of which the event time is the 'syn_time'.
        """
        return (self.get_ordered_finger_path(finger, 'syn_time')
                if isinstance(finger, int) else
                [packet[-1].get(MTB.EV_TIME) for packet in self.packets])

    def _call_check_event_func(self, event, expected_value, check_event_result,
                               check_event_func):
        """Call all functions in check_event_func and return the results.

        Note that since check_event_result is a dictionary, it is passed
        by reference.
        """
        for func in check_event_func:
            if func(event):
                event_value = event[MTB.EV_VALUE]
                check_event_result[func] = (event_value == expected_value)
                break

    def _get_event_cycles(self, check_event_func):
        """A generic method to get the number of event cycles.

        For a tap, its event cycle looks like:
            (1) finger touching the touch device:
                BTN_TOOL_FINGER: 0-> 1
                BTN_TOUCH: 0 -> 1
            (2) finger leaving the touch device:
                BTN_TOOL_FINGER: 1-> 0
                BTN_TOUCH: 1 -> 0

        For a one-finger physical click, its event cycle looks like:
            (1) finger clicking and pressing:
                BTN_LEFT : 0-> 1
                BTN_TOOL_FINGER: 0-> 1
                BTN_TOUCH: 0 -> 1
            (2) finger leaving:
                BTN_LEFT : 1-> 0
                BTN_TOOL_FINGER: 1-> 0
                BTN_TOUCH: 1 -> 0

        This method counts how many such cycles there are in the packets.
        """
        # Initialize all check_event_result to False
        # when all_events_observed is False and all check_event_result are True
        #      => all_events_observed is set to True
        # when all_events_observed is True and all check_event_result are True
        #      => all_events_observed is set to False, and
        #         count is increased by 1
        check_event_result = self._init_dict(check_event_func, False)
        all_events_observed = False
        count = 0
        for packet in self.packets:
            for event in packet:
                if all_events_observed:
                    expected_value = 0
                    self._call_check_event_func(event, expected_value,
                                                check_event_result,
                                                check_event_func)
                    if all(check_event_result.values()):
                        all_events_observed = False
                        check_event_result = self._init_dict(check_event_func,
                                                             False)
                        count += 1
                else:
                    expected_value = 1
                    self._call_check_event_func(event, expected_value,
                                                check_event_result,
                                                check_event_func)
                    if all(check_event_result.values()):
                        all_events_observed = True
                        check_event_result = self._init_dict(check_event_func,
                                                             False)
        return count

    def _get_event_cycles_for_num_fingers(self, num_fingers):
        return self._get_event_cycles(self.check_event_func_list[num_fingers])

    def verify_exact_number_fingers_touch(self, num_fingers):
        """Verify the exact number of fingers touching the device.

        Example: for a two-finger touch
            2-finger touch cycles should be equal to 1
            3/4/5-finger touch cycles should be equal to 0
            Don't care about 1-finger touch cycles which is not deterministic.
        """
        range_fingers = range(1, self.MAX_FINGERS)
        flag_check = self._init_dict(range_fingers, True)
        for f in range_fingers:
            cycles = self._get_event_cycles_for_num_fingers(f)
            if f == num_fingers:
                flag_check[f] = cycles == 1
            elif f > num_fingers:
                flag_check[f] = cycles == 0
        return all(flag_check)

    def get_physical_clicks(self, num_fingers):
        """Get the count of physical clicks for the given number of fingers."""
        flag_fingers_touch = self.verify_exact_number_fingers_touch(num_fingers)
        click_cycles = self._get_event_cycles(self.check_event_func_click)
        return click_cycles if flag_fingers_touch else 0

    def get_raw_physical_clicks(self):
        """Get how many BTN_LEFT click events have been seen.

        When calculating the raw BTN_LEFT click count, this method does not
        consider whether BTN_LEFT comes with the correct finger (tracking) IDs.
        A correct BTN_LEFT click consists of value 1 followed by value 0.
        """
        click_count = 0
        btn_left_was_pressed = False
        for packet in self.packets:
            for event in packet:
                # when seeing BTN_LEFT value: 0 -> 1
                if (MtbEvent.is_BTN_LEFT_value(event, 1) and
                        not btn_left_was_pressed):
                    btn_left_was_pressed = True
                # when seeing BTN_LEFT value: 1 -> 0
                elif (MtbEvent.is_BTN_LEFT_value(event, 0) and
                        btn_left_was_pressed):
                    btn_left_was_pressed = False
                    click_count += 1
        return click_count

    def get_correct_physical_clicks(self, number_fingers):
        """Get the count of physical clicks correctly overlap with
        the given number of fingers.

        @param num_fingers: the expected number of fingers when a physical
                click is seen
        """
        sm = MtbStateMachine()
        correct_click_count = 0
        click_with_correct_number_fingers = False
        for packet in self.packets:
            btn_left_was_pressed = False
            btn_left_was_released = False
            for event in packet:
                sm.add_event(event)
                if MtbEvent.is_BTN_LEFT_value(event, 1):
                    btn_left_was_pressed = True
                elif MtbEvent.is_BTN_LEFT_value(event, 0):
                    btn_left_was_released = True
            sm.get_current_tid_data_for_all_tids()

            # Check the number of fingers only after all events in this packet
            # have been processed.
            if btn_left_was_pressed or btn_left_was_released:
                click_with_correct_number_fingers |= (sm.number_fingers ==
                                                      number_fingers)

            # If BTN_LEFT was released, reset the flag and increase the count.
            if btn_left_was_released and click_with_correct_number_fingers:
                correct_click_count += 1
                click_with_correct_number_fingers = False

        return correct_click_count


class MtbParser:
    """Touch device MTB event Parser."""

    def __init__(self):
        self._get_event_re_patt()

    def _get_event_re_patt(self):
        """Construct the regular expression search pattern of MTB events.

        An ordinary event looks like
          Event: time 133082.748019, type 3 (EV_ABS), code 0 (ABS_X), value 316
        A SYN_REPORT event looks like
          Event: time 10788.289613, -------------- SYN_REPORT ------------
        """
        # Get the pattern of an ordinary event
        event_patt_time = 'Event:\s*time\s*(\d+\.\d+)'
        event_patt_type = 'type\s*(\d+)\s*\(\w+\)'
        event_patt_code = 'code\s*(\d+)\s*\(\w+\)'
        event_patt_value = 'value\s*(-?\d+)'
        event_sep = ',\s*'
        event_patt = event_sep.join([event_patt_time,
                                     event_patt_type,
                                     event_patt_code,
                                     event_patt_value])
        self.event_re_patt = re.compile(event_patt, re.I)

        # Get the pattern of the SYN_REPORT event
        event_patt_type_SYN_REPORT = '-+\s*SYN_REPORT\s-+'
        event_patt_SYN_REPORT = event_sep.join([event_patt_time,
                                                event_patt_type_SYN_REPORT])
        self.event_re_patt_SYN_REPORT = re.compile(event_patt_SYN_REPORT, re.I)

    def _get_event_dict_ordinary(self, line):
        """Construct the event dictionary for an ordinary event."""
        result = self.event_re_patt.search(line)
        if result is not None:
            return MtbParser.make_ev_dict((float(result.group(1)),
                                          int(result.group(2)),
                                          int(result.group(3)),
                                          int(result.group(4))))
        return {}

    @staticmethod
    def make_ev_dict(event):
        (ev_time, ev_type, ev_code, ev_value) = event
        ev_dict = {MTB.EV_TIME: ev_time,
                   MTB.EV_TYPE: ev_type,
                   MTB.EV_CODE: ev_code,
                   MTB.EV_VALUE: ev_value}
        return ev_dict

    @staticmethod
    def make_syn_report_ev_dict(syn_time):
        """Make the event dictionary for a SYN_REPORT event."""
        ev_dict = {}
        ev_dict[MTB.EV_TIME] = float(syn_time)
        ev_dict[MTB.SYN_REPORT] = True
        return ev_dict

    def _get_event_dict_SYN_REPORT(self, line):
        """Construct the event dictionary for a SYN_REPORT event."""
        result = self.event_re_patt_SYN_REPORT.search(line)
        return ({} if result is None else
                MtbParser.make_syn_report_ev_dict(result.group(1)))

    def _get_event_dict(self, line):
        """Construct the event dictionary."""
        EVENT_FUNC_LIST = [self._get_event_dict_ordinary,
                           self._get_event_dict_SYN_REPORT]
        for get_event_func in EVENT_FUNC_LIST:
            ev_dict = get_event_func(line)
            if ev_dict:
                return ev_dict
        return False

    def _is_SYN_REPORT(self, ev_dict):
        """Determine if this event is SYN_REPORT."""
        return ev_dict.get(MTB.SYN_REPORT, False)

    def parse(self, raw_event):
        """Parse the raw event string into a list of event dictionary."""
        ev_list = []
        packets = []
        start_flag = False
        finger_off = False
        for line in raw_event:
            ev_dict = self._get_event_dict(line)
            if ev_dict:
                start_flag = True

                # Handle the case when a finger-off event is followed
                # immediately by a finger-on event in the same packet.
                if MtbEvent.is_finger_leaving(ev_dict):
                    finger_off = True
                # Append a SYN_REPORT event and flush the ev_list to packets
                # when the case described above does occur.
                elif MtbEvent.is_new_contact(ev_dict) and finger_off:
                    last_ev_time = ev_list[-1][MTB.EV_TIME]
                    ev_list.append(
                        MtbParser.make_syn_report_ev_dict(last_ev_time))
                    packets.append(ev_list)
                    ev_list = []

                ev_list.append(ev_dict)
                if self._is_SYN_REPORT(ev_dict):
                    packets.append(ev_list)
                    ev_list = []
                    finger_off = False

            elif start_flag:
                logging.warning('  Warn: format problem in event:\n  %s' % line)
        return packets

    def parse_file(self, file_name):
        """Parse raw device events in the given file name."""
        packets = None
        if os.path.isfile(file_name):
            with open(file_name) as f:
                packets = self.parse(f)
        return packets


if __name__ == '__main__':
    # Read a device file, and convert it to pretty packet format.
    if len(sys.argv) != 2 or not os.path.exists(sys.argv[1]):
        print 'Usage: %s device_file' % sys.argv[0]
        exit(1)

    with open(sys.argv[1]) as event_file:
        packets = MtbParser().parse(event_file)
    for packet in packets:
        print make_pretty_packet(packet)
