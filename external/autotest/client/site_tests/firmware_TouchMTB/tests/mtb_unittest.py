# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This module contains unit tests for the classes in the mtb module

import glob
import os
import sys
import unittest

import common_unittest_utils
import fuzzy
import mtb
import test_conf as conf

from common_unittest_utils import create_mocked_devices
from firmware_constants import AXIS, GV, MTB, PLATFORM, UNIT, VAL
from mtb import FingerPath, TidPacket
from geometry.elements import Point, about_eq


unittest_path_lumpy = os.path.join(os.getcwd(), 'tests/logs/lumpy')
mocked_device = create_mocked_devices()


def get_mtb_packets(gesture_filename):
    """Get mtb_packets object by reading the gesture file."""
    parser = mtb.MtbParser()
    packets = parser.parse_file(gesture_filename)
    mtb_packets = mtb.Mtb(packets=packets)
    return mtb_packets


class FakeMtb(mtb.Mtb):
    """A fake MTB class to set up x and y positions directly."""
    def __init__(self, list_x, list_y):
        self.list_x = list_x
        self.list_y = list_y

    def get_x_y(self, target_slot):
        """Return list_x, list_y directly."""
        return (self.list_x, self.list_y)


class MtbTest(unittest.TestCase):
    """Unit tests for mtb.Mtb class."""

    def setUp(self):
        self.test_dir = os.path.join(os.getcwd(), 'tests')
        self.data_dir = os.path.join(self.test_dir, 'data')

    def _get_filepath(self, filename, gesture_dir=''):
        return os.path.join(self.data_dir, gesture_dir, filename)

    def _get_range_middle(self, criteria):
        """Get the middle range of the criteria."""
        fc = fuzzy.FuzzyCriteria(criteria)
        range_min , range_max = fc.get_criteria_value_range()
        range_middle = (range_min + range_max) / 2.0
        return range_middle

    def _call_get_reversed_motions(self, list_x, list_y, expected_x,
                                   expected_y, direction):
        mtb = FakeMtb(list_x, list_y)
        displacement = mtb.get_reversed_motions(0, direction, ratio=0.1)
        self.assertEqual(displacement[AXIS.X], expected_x)
        self.assertEqual(displacement[AXIS.Y], expected_y)

    def test_get_reversed_motions_no_reversed(self):
        list_x = (10, 22 ,36, 54, 100)
        list_y = (1, 2 ,6, 10, 22)
        self._call_get_reversed_motions(list_x, list_y, 0, 0, GV.TLBR)

    def test_get_reversed_motions_reversed_x_y(self):
        list_x = (10, 22 ,36, 154, 100)
        list_y = (1, 2 ,6, 30, 22)
        self._call_get_reversed_motions(list_x, list_y, -54, -8, GV.TLBR)

    def _test_get_x_y(self, filename, slot, expected_value):
        gesture_filename = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(gesture_filename)
        list_x, list_y = mtb_packets.get_x_y(slot)
        points = zip(list_x, list_y)
        self.assertEqual(len(points), expected_value)

    def test_get_x_y(self):
        self._test_get_x_y('one_finger_with_slot_0.dat', 0, 12)
        self._test_get_x_y('one_finger_without_slot_0.dat', 0, 9)
        self._test_get_x_y('two_finger_with_slot_0.dat', 0, 121)
        self._test_get_x_y('two_finger_with_slot_0.dat', 1, 59)
        self._test_get_x_y('two_finger_without_slot_0.dat', 0, 104)
        self._test_get_x_y('two_finger_without_slot_0.dat', 1, 10)

    def test_get_pressure(self):
        """Test get pressure"""
        filename = 'one_finger_with_slot_0.dat'
        gesture_filename = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(gesture_filename)
        finger_paths = mtb_packets.get_ordered_finger_paths()

        # There is only one tracking ID in the file.
        self.assertEqual(len(finger_paths), 1)

        # Verify some of the pressure values
        finger_path = finger_paths.values()[0]
        list_z = finger_path.get('pressure')
        self.assertEqual(list_z[0:5], [59, 57, 56, 58, 60])

    def test_get_x_y_multiple_slots(self):
        filename = 'x_y_multiple_slots.dat'
        filepath = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(filepath)
        slots = (0, 1)
        list_x, list_y = mtb_packets.get_x_y_multiple_slots(slots)
        expected_list_x = {}
        expected_list_y = {}
        expected_list_x[0] = [1066, 1068, 1082, 1183, 1214, 1285, 1322, 1351,
                              1377, 1391]
        expected_list_y[0] = [561, 559, 542, 426, 405, 358, 328, 313, 304, 297]
        expected_list_x[1] = [770, 769, 768, 758, 697, 620, 585, 565, 538, 538]
        expected_list_y[1] = [894, 894, 895, 898, 927, 968, 996, 1003, 1013,
                              1013]
        for slot in slots:
            self.assertEqual(list_x[slot], expected_list_x[slot])
            self.assertEqual(list_y[slot], expected_list_y[slot])

    def test_get_x_y_multiple_slots2(self):
        """Test slot state machine.

        When the last slot in the previous packet is slot 0, and the first
        slot in the current packet is also slot 0, the slot 0 will not be
        displayed explicitly. This test ensures that the slot stat machine
        is tracked properly.
        """
        filename = 'pinch_to_zoom.zoom_in.dat'
        filepath = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(filepath)
        slots = (0, 1)
        list_x, list_y = mtb_packets.get_x_y_multiple_slots(slots)
        expected_final_x = {}
        expected_final_y = {}
        expected_final_x[0] = 1318
        expected_final_y[0] = 255
        expected_final_x[1] = 522
        expected_final_y[1] = 1232
        for slot in slots:
            self.assertEqual(list_x[slot][-1], expected_final_x[slot])
            self.assertEqual(list_y[slot][-1], expected_final_y[slot])

    def _test_get_all_finger_paths_about_numbers_of_packets(
            self, filename, expected_numbers):
        mtb_packets = get_mtb_packets(self._get_filepath(filename))
        finger_paths = mtb_packets.get_ordered_finger_paths()
        for tid, expected_len in expected_numbers.items():
            self.assertEqual(len(finger_paths[tid].tid_packets), expected_len)

    def test_get_ordered_finger_paths_about_number_of_packets(self):
        self._test_get_all_finger_paths_about_numbers_of_packets(
                'two_finger_with_slot_0.dat', {2101: 122, 2102: 60})
        self._test_get_all_finger_paths_about_numbers_of_packets(
                'two_finger_without_slot_0.dat', {2097: 105, 2098: 11})

    def test_data_ready(self):
        """Test data_ready flag when point.x could be 0."""
        filename = ('20130506_030025-fw_11.27-robot_sim/'
                    'one_finger_to_edge.center_to_left.slow-lumpy-fw_11.27-'
                    'robot_sim-20130506_031554.dat')
        filepath = os.path.join(unittest_path_lumpy, filename)
        mtb_packets = get_mtb_packets(filepath)
        points = mtb_packets.get_ordered_finger_path(0, 'point')
        # Note:
        # 1. In the first packet, there exists the event ABS_PRESSURE
        #    but no ABS_MT_PRESSURE.
        # 2. The last packet with ABS_MT_TRACKING_ID = -1 is also counted.
        self.assertEqual(len(points), 78)

    def _test_drumroll(self, filename, expected_max_distance):
        """expected_max_distance: unit in pixel"""
        gesture_filename = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(gesture_filename)
        actual_max_distance = mtb_packets.get_max_distance_of_all_tracking_ids()
        self.assertTrue(about_eq(actual_max_distance, expected_max_distance))

    def test_drumroll(self):
        expected_max_distance = 52.0216301167
        self._test_drumroll('drumroll_lumpy.dat', expected_max_distance)

    def test_drumroll1(self):
        expected_max_distance = 43.5660418216
        self._test_drumroll('drumroll_lumpy_1.dat', expected_max_distance)

    def test_drumroll_link(self):
        expected_max_distance = 25.6124969497
        self._test_drumroll('drumroll_link.dat', expected_max_distance)

    def test_no_drumroll_link(self):
        expected_max_distance = 2.91547594742
        self._test_drumroll('no_drumroll_link.dat', expected_max_distance)

    def test_no_drumroll_link(self):
        expected_max_distance = 24.8243831746
        self._test_drumroll('drumroll_link_2.dat', expected_max_distance)

    def _test_finger_path(self, filename, tid, expected_slot, expected_data,
                          request_data_ready=True):
        """Test the data in a finger path"""
        # Instantiate the expected finger_path
        expected_finger_path = FingerPath(expected_slot,
                                          [TidPacket(time, Point(*xy), z)
                                          for time, xy, z in expected_data])

        # Derive the actual finger_path for the specified tid
        mtb_packets = get_mtb_packets(self._get_filepath(filename))
        finger_paths = mtb_packets.get_ordered_finger_paths(request_data_ready)
        actual_finger_path = finger_paths[tid]

        # Assert that the packet lengths are the same.
        self.assertEqual(len(expected_finger_path.tid_packets),
                         len(actual_finger_path.tid_packets))

        # Assert that all tid data (including syn_time, point, pressure, etc.)
        # in the tid packets are the same.
        for i in range(len(actual_finger_path.tid_packets)):
            expected_packet = expected_finger_path.tid_packets[i]
            actual_packet = actual_finger_path.tid_packets[i]
            self.assertEqual(expected_packet.syn_time, actual_packet.syn_time)
            self.assertTrue(expected_packet.point == actual_packet.point)
            self.assertEqual(expected_packet.pressure, actual_packet.pressure)

    def test_get_ordered_finger_paths(self):
        """Test get_ordered_finger_paths

        Tracking ID 95: slot 0 (no explicit slot 0 assigned).
                        This is the only slot in the packet.
        """
        filename = 'drumroll_link_2.dat'
        tid = 95
        expected_slot = 0
        expected_data = [# (syn_time,    (x,   y),   z)
                         (238154.686034, (789, 358), 59),
                         (238154.691606, (789, 358), 60),
                         (238154.697058, (789, 358), 57),
                         (238154.702576, (789, 358), 59),
                         (238154.713731, (789, 358), 57),
                         (238154.719160, (789, 359), 57),
                         (238154.724791, (789, 359), 56),
                         (238154.730111, (789, 359), 58),
                         (238154.735588, (788, 359), 53),
                         (238154.741068, (788, 360), 53),
                         (238154.746569, (788, 360), 49),
                         (238154.752108, (787, 360), 40),
                         (238154.757705, (787, 361), 27),
                         (238154.763075, (490, 903), 46),
                         (238154.768532, (486, 892), 61),
                         (238154.774695, (484, 895), 57),
                         (238154.780192, (493, 890), 56),
                         (238154.785651, (488, 893), 55),
                         (238154.791140, (488, 893), 56),
                         (238154.802080, (489, 893), 55),
                         (238154.807578, (490, 893), 50),
                         (238154.818573, (490, 893), 46),
                         (238154.824066, (491, 893), 36),
                         (238154.829525, (492, 893), 22),
                         (238154.849958, (492, 893), 22),
        ]
        self._test_finger_path(filename, tid, expected_slot, expected_data)

    def test_get_ordered_finger_paths2(self):
        """Test get_ordered_finger_paths

        Tracking ID 104: slot 0 (explicit slot 0 assigned).
                         This is the 2nd slot in the packet.
                         A slot 1 has already existed.
        """

        filename = 'drumroll_link_2.dat'
        tid = 104
        expected_slot = 0
        expected_data = [# (syn_time,    (x,   y),   z)
                         (238157.994296, (780, 373), 75),
                         (238158.001110, (780, 372), 75),
                         (238158.007128, (780, 372), 76),
                         (238158.012617, (780, 372), 73),
                         (238158.018112, (780, 373), 69),
                         (238158.023600, (780, 373), 68),
                         (238158.029542, (781, 373), 51),
                         (238158.049605, (781, 373), 51),
        ]
        self._test_finger_path(filename, tid, expected_slot, expected_data)

    def test_get_ordered_finger_paths2b(self):
        """Test get_ordered_finger_paths

        Tracking ID 103: slot 1 (explicit slot 1 assigned).
                         This tracking ID overlaps with two distinct
                         tracking IDs of which the slot is the same slot 0.
                         This is a good test as a multiple-finger case.

                         tid 102, slot 0 arrived
                         tid 103, slot 1 arrived
                         tid 102, slot 0 left
                         tid 104, slot 0 arrived
                         tid 103, slot 1 left
                         tid 104, slot 0 left
        """
        filename = 'drumroll_link_2.dat'
        tid = 103
        expected_slot = 1
        expected_data = [# (syn_time,    (x,   y),   z)
                         (238157.906405, (527, 901), 71),
                         (238157.911749, (527, 901), 74),
                         (238157.917247, (527, 901), 73),
                         (238157.923152, (527, 902), 71),
                         (238157.928317, (527, 902), 72),
                         (238157.934492, (527, 902), 71),
                         (238157.939984, (527, 902), 69),
                         (238157.945485, (527, 902), 65),
                         (238157.950984, (527, 902), 66),
                         (238157.956482, (527, 902), 70),
                         (238157.961976, (527, 902), 65),
                         (238157.973768, (527, 902), 64),
                         (238157.980491, (528, 901), 61),
                         (238157.987140, (529, 899), 60),
                         (238157.994296, (531, 896), 52),
                         (238158.001110, (534, 892), 34),
                         (238158.007128, (534, 892), 34),
                         (238158.012617, (534, 892), 34),
                         (238158.018112, (534, 892), 34),
                         (238158.023600, (534, 892), 34),
                         (238158.029542, (534, 892), 34),
        ]
        self._test_finger_path(filename, tid, expected_slot, expected_data)

    def test_get_ordered_finger_paths3(self):
        """Test get_ordered_finger_paths

        This is a good test sample.
        - An unusual slot 9
        - This is the 2nd slot in the packet. A slot 8 has already existed.
        - Its ABS_MT_PRESSURE is missing in the first packet.
        - Slot 8 terminates a few packets earlier than this slot.
        - Some of the ABS_MT_POSITION_X/Y and ABS_MT_PRESSURE are not shown.
        """
        filename = 'drumroll_3.dat'
        tid = 582
        expected_slot = 9
        expected_data = [# (syn_time,  (x,   y),   z)
                         (6411.371613, (682, 173), None),
                         (6411.382541, (667, 186), 35),
                         (6411.393355, (664, 189), 37),
                         (6411.404310, (664, 190), 38),
                         (6411.413015, (664, 189), 38),
                         (6411.422118, (665, 189), 38),
                         (6411.430792, (665, 189), 37),
                         (6411.439764, (667, 188), 36),
                         (6411.448484, (675, 185), 29),
                         (6411.457212, (683, 181), 17),
                         (6411.465843, (693, 172), 5),
                         (6411.474749, (469, 381), 6),
                         (6411.483702, (471, 395), 26),
                         (6411.492369, (471, 396), 13),
                         (6411.499916, (471, 396), 13),
        ]
        self._test_finger_path(filename, tid, expected_slot, expected_data,
                               request_data_ready=False)

    def test_get_ordered_finger_paths4(self):
        """Test get_ordered_finger_paths

        This test is to verify if it could handle the case when a finger-off
        event is followed immediately by a finger-on event in the same packet.
        This situation may occur occasionally in two_close_fingers_tracking
        gestures. Basically, this could be considered as a firmware bug.
        However, our test should be able to handle the situation gracefully.

        A problematic packet may look like:

        Event: time .., type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value -1
        Event: time .., type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 202
        Event: time .., type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 1577
        Event: time .., type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 1018
        Event: time .., type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 99
        Event: time .., type 3 (EV_ABS), code 48 (ABS_MT_TOUCH_MAJOR), value 19
        Event: time .., type 3 (EV_ABS), code 49 (ABS_MT_TOUCH_MINOR), value 19
        Event: time .., type 3 (EV_ABS), code 0 (ABS_X), value 1577
        Event: time .., type 3 (EV_ABS), code 1 (ABS_Y), value 1018
        Event: time .., type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 99
        Event: time .., -------------- SYN_REPORT ------------
        """
        # Get the actual finger_paths from the gesture data file.
        filename = 'two_close_fingers_tracking.dat'
        mtb_packets = get_mtb_packets(self._get_filepath(filename))
        finger_paths = mtb_packets.get_ordered_finger_paths(
                request_data_ready=False)

        data_list = [
                # (tid, packet_idx, syn_time, (x, y), z, number_packets)
                (197, -1, 1395784288.323233, (1619, 1019), 98, 435),
                (202, 0, 1395784288.323233, (1577, 1018), 99, 261),
        ]

        for tid, packet_idx, syn_time, xy, z, number_packets in data_list:
            expected_packet = TidPacket(syn_time, Point(*xy), z)

            # Derive the actual finger path and the actual packet.
            actual_finger_path = finger_paths[tid]
            actual_packet = actual_finger_path.tid_packets[packet_idx]

            # Assert that the number of packets in the actual finger path
            # is equal to the specified number.
            self.assertEqual(number_packets,
                             len(actual_finger_path.tid_packets))

            # Assert that the expected packet is equal to the actual packet.
            self.assertEqual(expected_packet.syn_time, actual_packet.syn_time)
            self.assertTrue(expected_packet.point == actual_packet.point)
            self.assertEqual(expected_packet.pressure, actual_packet.pressure)


    def test_get_slot_data(self):
        """Test if it can get the data from the correct slot.

        slot 0 and slot 1 start at the same packet. This test verifies if the
        method uses the correct corresponding slot numbers.
        """
        filename = 'two_finger_tracking.diagonal.slow.dat'
        gesture_filename = self._get_filepath(filename)
        mtb_packets = get_mtb_packets(gesture_filename)

        # There are more packets. Use just a few of them to verify.
        xy_pairs = {
            # Slot 0
            0: [(1142, 191), (1144, 201), (1144, 200)],
            # Slot 1
            1: [(957, 105), (966, 106), (960, 104)],
        }

        number_packets = {
            # Slot 0
            0: 190,
            # Slot 1
            1: 189,
        }

        slots = [0, 1]
        for slot in slots:
            points = mtb_packets.get_slot_data(slot, 'point')
            # Verify the number of packets in each slot
            self.assertEqual(len(points), number_packets[slot])
            # Verify a few packets in each slot
            for i, xy_pair in enumerate(xy_pairs[slot]):
                self.assertTrue(Point(*xy_pair) == points[i])

    def test_convert_to_evemu_format(self):
        evemu_filename = self._get_filepath('one_finger_swipe.evemu.dat')
        mtplot_filename = self._get_filepath('one_finger_swipe.dat')
        packets = mtb.MtbParser().parse_file(mtplot_filename)
        evemu_converted_iter = iter(mtb.convert_to_evemu_format(packets))
        with open(evemu_filename) as evemuf:
            for line_evemu_original in evemuf:
                evemu_original = line_evemu_original.split()
                evemu_converted_str = next(evemu_converted_iter, None)
                self.assertNotEqual(evemu_converted_str, None)
                if evemu_converted_str:
                    evemu_converted = evemu_converted_str.split()
                self.assertEqual(len(evemu_original), 5)
                self.assertEqual(len(evemu_converted), 5)
                # Skip the timestamps for they are different in both formats.
                # Prefix, type, code, and value should be the same.
                for i in [0, 2, 3, 4]:
                    self.assertEqual(evemu_original[i], evemu_converted[i])

    def test_get_largest_gap_ratio(self):
        """Test get_largest_gap_ratio for one-finger and two-finger gestures."""
        # The following files come with noticeable large gaps.
        list_large_ratio = [
            'one_finger_tracking.left_to_right.slow_1.dat',
            'two_finger_gaps.vertical.dat',
            'two_finger_gaps.horizontal.dat',
            'resting_finger_2nd_finger_moving_segment_gaps.dat',
            'gap_new_finger_arriving_or_departing.dat',
            'one_stationary_finger_2nd_finger_moving_gaps.dat',
            'resting_finger_2nd_finger_moving_gaps.dat',
        ]
        gesture_slots = {
            'one_finger': [0,],
            'two_finger': [0, 1],
            'resting_finger': [1,],
            'gap_new_finger': [0,],
            'one_stationary_finger': [1,],
        }

        range_middle = self._get_range_middle(conf.no_gap_criteria)
        gap_data_dir = self._get_filepath('gaps')
        gap_data_filenames = glob.glob(os.path.join(gap_data_dir, '*.dat'))
        for filename in gap_data_filenames:
            mtb_packets = get_mtb_packets(filename)
            base_filename = os.path.basename(filename)

            # What slots to check are based on the gesture name.
            slots = []
            for gesture in gesture_slots:
                if base_filename.startswith(gesture):
                    slots = gesture_slots[gesture]
                    break

            for slot in slots:
                largest_gap_ratio = mtb_packets.get_largest_gap_ratio(slot)
                if base_filename in list_large_ratio:
                    self.assertTrue(largest_gap_ratio >= range_middle)
                else:
                    self.assertTrue(largest_gap_ratio < range_middle)

    def test_get_largest_accumulated_level_jumps(self):
        """Test get_largest_accumulated_level_jumps."""
        dir_level_jumps = 'drag_edge_thumb'

        filenames = [
            # filenames with level jumps
            # ----------------------------------
            'drag_edge_thumb.horizontal.dat',
            'drag_edge_thumb.horizontal_2.dat',
            # test no points in some tracking ID
            'drag_edge_thumb.horizontal_3.no_points.dat',
            'drag_edge_thumb.vertical.dat',
            'drag_edge_thumb.vertical_2.dat',
            'drag_edge_thumb.diagonal.dat',
            # Change tracking IDs quickly.
            'drag_edge_thumb.horizontal_4.change_ids_quickly.dat',

            # filenames without level jumps
            # ----------------------------------
            'drag_edge_thumb.horizontal.curvy.dat',
            'drag_edge_thumb.horizontal_2.curvy.dat',
            'drag_edge_thumb.vertical.curvy.dat',
            'drag_edge_thumb.vertical_2.curvy.dat',
            # Rather small level jumps
            'drag_edge_thumb.horizontal_5.small_level_jumps.curvy.dat',
        ]

        largest_level_jumps = {
            # Large jumps
            'drag_edge_thumb.horizontal.dat': {AXIS.X: 0, AXIS.Y: 97},
            # Smaller jumps
            'drag_edge_thumb.horizontal_2.dat': {AXIS.X: 0, AXIS.Y: 24},
            # test no points in some tracking ID
            'drag_edge_thumb.horizontal_3.no_points.dat':
                    {AXIS.X: 97, AXIS.Y: 88},
            # Change tracking IDs quickly.
            'drag_edge_thumb.horizontal_4.change_ids_quickly.dat':
                    {AXIS.X: 0, AXIS.Y: 14},
            # Large jumps
            'drag_edge_thumb.vertical.dat': {AXIS.X: 54, AXIS.Y: 0},
            # The first slot 0 comes with smaller jumps only.
            'drag_edge_thumb.vertical_2.dat': {AXIS.X: 20, AXIS.Y: 0},
            # Large jumps
            'drag_edge_thumb.diagonal.dat': {AXIS.X: 84, AXIS.Y: 58},
        }

        target_slot = 0
        for filename in filenames:
            filepath = self._get_filepath(filename, gesture_dir=dir_level_jumps)
            packets = get_mtb_packets(filepath)
            displacements = packets.get_displacements_for_slots(target_slot)

            # There are no level jumps in a curvy line.
            file_with_level_jump = 'curvy' not in filename

            # Check the first slot only
            tids = displacements.keys()
            tids.sort()
            tid = tids[0]
            # Check both axis X and axis Y
            for axis in AXIS.LIST:
                disp = displacements[tid][axis]
                jump = packets.get_largest_accumulated_level_jumps(disp)
                # Verify that there are no jumps in curvy files, and
                #        that there are jumps in the other files.
                expected_jump = (0 if not file_with_level_jump
                                   else largest_level_jumps[filename][axis])
                self.assertTrue(jump == expected_jump)

    def test_get_max_distance_from_points(self):
        """Test get_max_distance_from_points"""
        # Two farthest points: (15, 16) and (46, 70)
        list_coordinates_pairs = [
            (20, 25), (21, 35), (15, 16), (25, 22), (30, 32), (46, 70),
            (35, 68), (42, 53), (50, 30), (43, 69), (16, 17), (14, 30),
        ]
        points = [Point(*pairs) for pairs in list_coordinates_pairs]
        mtb_packets = mtb.Mtb(device=mocked_device[PLATFORM.LUMPY])

        # Verify the max distance in pixels
        max_distance_px = mtb_packets.get_max_distance_from_points(points,
                                                                   UNIT.PIXEL)
        expected_max_distance_px = ((46 - 15) ** 2 + (70 - 16) ** 2) ** 0.5
        self.assertAlmostEqual(max_distance_px, expected_max_distance_px)

        # Verify the max distance in mms
        max_distance_mm = mtb_packets.get_max_distance_from_points(points,
                                                                   UNIT.MM)
        expected_max_distance_mm = (((46 - 15) / 12.0) ** 2 +
                                    ((70 - 16) / 10.0) ** 2) ** 0.5
        self.assertAlmostEqual(max_distance_mm, expected_max_distance_mm)

    def _test_get_segments(self, list_t, list_coord, expected_segments, ratio):
        """Test get_segments

        @param expected_segments: a dictionary of
                {segment_flag: expected_segment_indexes}
        """
        mtb_packets = mtb.Mtb(device=mocked_device[PLATFORM.LUMPY])
        for segment_flag, (expected_segment_t, expected_segment_coord) in \
                expected_segments.items():
            segment_t, segment_coord = mtb_packets.get_segments(
                    list_t, list_coord, segment_flag, ratio)
            self.assertEqual(segment_t, expected_segment_t)
            self.assertEqual(segment_coord, expected_segment_coord)

    def test_get_segments_by_distance(self):
        """Test get_segments_by_distance

        In the test case below,
            min_coord = 100
            max_coord = 220
            max_distance = max_coord - min_coord = 220 - 100 = 120
            ratio = 0.1
            120 * 0.1 = 12
            begin segment: 100 ~ 112
            end segment: 208 ~ 220
        """
        list_coord = [102, 101, 101, 100, 100, 103, 104, 110, 118, 120,
                      122, 124, 131, 140, 150, 160, 190, 210, 217, 220]
        list_t = [1000 + 0.012 * i for i in range(len(list_coord))]
        ratio = 0.1
        expected_segments= {
                VAL.WHOLE: (list_t, list_coord),
                VAL.MIDDLE: (list_t[8:17], list_coord[8:17]),
                VAL.BEGIN: (list_t[:8], list_coord[:8]),
                VAL.END: (list_t[17:], list_coord[17:]),
        }
        self._test_get_segments(list_t, list_coord, expected_segments, ratio)

    def test_get_segments_by_length(self):
        """Test get_segments_by_length"""
        list_coords = [
                [105, 105, 105, 105, 105, 105, 105, 105, 105, 105],
                [104, 105, 105, 105, 105, 105, 105, 105, 105, 105],
                [105, 105, 105, 105, 105, 105, 105, 105, 105, 106],
        ]
        ratio = 0.1
        for list_c in list_coords:
            list_t = [1000 + 0.012 * i for i in range(len(list_c))]
            expected_segments= {
                    VAL.WHOLE: (list_t, list_c),
                    VAL.MIDDLE: (list_t[1:9], list_c[1:9]),
                    VAL.BEGIN: (list_t[:1], list_c[:1]),
                    VAL.END: (list_t[9:], list_c[9:]),
            }
            self._test_get_segments(list_t, list_c, expected_segments, ratio)


if __name__ == '__main__':
  unittest.main()
