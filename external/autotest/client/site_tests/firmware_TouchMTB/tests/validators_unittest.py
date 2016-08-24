# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""This module contains unit tests for the classes in the validators module."""

import glob
import os.path
import unittest

import common_unittest_utils
import common_util
import test_conf as conf
import validators

from common_unittest_utils import create_mocked_devices, parse_tests_data
from firmware_constants import AXIS, GV, MTB, PLATFORM, VAL
from firmware_log import MetricNameProps
from geometry.elements import Point
from touch_device import TouchDevice
from validators import (CountPacketsValidator,
                        CountTrackingIDValidator,
                        DiscardInitialSecondsValidator,
                        DrumrollValidator,
                        HysteresisValidator,
                        LinearityValidator,
                        MtbSanityValidator,
                        NoGapValidator,
                        NoLevelJumpValidator,
                        NoReversedMotionValidator,
                        PhysicalClickValidator,
                        PinchValidator,
                        RangeValidator,
                        ReportRateValidator,
                        StationaryFingerValidator,
                        StationaryTapValidator,
)


unittest_path_lumpy = os.path.join(os.getcwd(), 'tests/logs/lumpy')
mocked_device = create_mocked_devices()

# Make short aliases for supported platforms
alex = mocked_device[PLATFORM.ALEX]
lumpy = mocked_device[PLATFORM.LUMPY]
link = mocked_device[PLATFORM.LINK]
# Some tests do not care what device is used.
dontcare = 'dontcare'


class CountTrackingIDValidatorTest(unittest.TestCase):
    """Unit tests for CountTrackingIDValidator class."""

    def _test_count_tracking_id(self, filename, criteria, device):
        packets = parse_tests_data(filename)
        validator = CountTrackingIDValidator(criteria, device=device)
        vlog = validator.check(packets)
        return vlog.score

    def test_two_finger_id_change(self):
        """Two two fingers id change.

        Issue 7867: Cyapa : Two finger scroll, tracking ids change
        """
        filename = 'two_finger_id_change.dat'
        score = self._test_count_tracking_id(filename, '== 2', lumpy)
        self.assertTrue(score == 0)

    def test_one_finger_fast_swipe_id_split(self):
        """One finger fast swipe resulting in IDs split.

        Issue: 7869: Lumpy: Tracking ID reassigned during quick-2F-swipe
        """
        filename = 'one_finger_fast_swipe_id_split.dat'
        score = self._test_count_tracking_id(filename, '== 1', lumpy)
        self.assertTrue(score == 0)

    def test_two_fingers_fast_flick_id_split(self):
        """Two figners fast flick resulting in IDs split.

        Issue: 7869: Lumpy: Tracking ID reassigned during quick-2F-swipe
        """
        filename = 'two_finger_fast_flick_id_split.dat'
        score = self._test_count_tracking_id(filename, '== 2', lumpy)
        self.assertTrue(score == 0)


class DrumrollValidatorTest(unittest.TestCase):
    """Unit tests for DrumrollValidator class."""

    def setUp(self):
        self.criteria = conf.drumroll_criteria

    def _test_drumroll(self, filename, criteria, device):
        packets = parse_tests_data(filename)
        validator = DrumrollValidator(criteria, device=device)
        vlog = validator.check(packets)
        return vlog.score

    def _get_drumroll_metrics(self, filename, criteria, device):
        packets = parse_tests_data(filename, gesture_dir=unittest_path_lumpy)
        validator = DrumrollValidator(criteria, device=device)
        metrics = validator.check(packets).metrics
        return metrics

    def test_drumroll_lumpy(self):
        """Should catch the drumroll on lumpy.

        Issue 7809: Lumpy: Drumroll bug in firmware
        Max distance: 52.02 px
        """
        filename = 'drumroll_lumpy.dat'
        score = self._test_drumroll(filename, self.criteria, lumpy)
        self.assertTrue(score == 0)

    def test_drumroll_lumpy_1(self):
        """Should catch the drumroll on lumpy.

        Issue 7809: Lumpy: Drumroll bug in firmware
        Max distance: 43.57 px
        """
        filename = 'drumroll_lumpy_1.dat'
        score = self._test_drumroll(filename, self.criteria, lumpy)
        self.assertTrue(score <= 0.15)

    def test_no_drumroll_link(self):
        """Should pass (score == 1) when there is no drumroll.

        Issue 7809: Lumpy: Drumroll bug in firmware
        Max distance: 2.92 px
        """
        filename = 'no_drumroll_link.dat'
        score = self._test_drumroll(filename, self.criteria, link)
        self.assertTrue(score == 1)

    def test_drumroll_metrics(self):
        """Test the drumroll metrics."""
        expected_max_values = {
            '20130506_030025-fw_11.27-robot_sim/'
            'drumroll.fast-lumpy-fw_11.27-manual-20130528_044804.dat':
            2.29402908535,

            '20130506_030025-fw_11.27-robot_sim/'
            'drumroll.fast-lumpy-fw_11.27-manual-20130528_044820.dat':
            0.719567771497,

            '20130506_031746-fw_11.27-robot_sim/'
            'drumroll.fast-lumpy-fw_11.27-manual-20130528_044728.dat':
            0.833491481592,

            '20130506_032458-fw_11.23-robot_sim/'
            'drumroll.fast-lumpy-fw_11.23-manual-20130528_044856.dat':
            1.18368539364,

            '20130506_032458-fw_11.23-robot_sim/'
            'drumroll.fast-lumpy-fw_11.23-manual-20130528_044907.dat':
            0.851161282019,

            '20130506_032659-fw_11.23-robot_sim/'
            'drumroll.fast-lumpy-fw_11.23-manual-20130528_044933.dat':
            2.64245519251,

            '20130506_032659-fw_11.23-robot_sim/'
            'drumroll.fast-lumpy-fw_11.23-manual-20130528_044947.dat':
            0.910624022916,
        }
        criteria = self.criteria
        for filename, expected_max_value in expected_max_values.items():
            metrics = self._get_drumroll_metrics(filename, criteria, lumpy)
            actual_max_value = max([m.value for m in metrics])
            self.assertAlmostEqual(expected_max_value, actual_max_value)


class LinearityValidatorTest(unittest.TestCase):
    """Unit tests for LinearityValidator class."""

    def setUp(self):
        self.validator = LinearityValidator(conf.linearity_criteria,
                                            device=lumpy, finger=0)
        self.validator.init_check()

    def test_simple_linear_regression0(self):
        """A perfect y-t line from bottom left to top right"""
        list_y = [20, 40, 60, 80, 100, 120, 140, 160]
        list_t = [i * 0.1 for i in range(len(list_y))]
        (max_err_px, rms_err_px) = self.validator._calc_errors_single_axis(
                list_t, list_y)
        self.assertAlmostEqual(max_err_px, 0)
        self.assertAlmostEqual(rms_err_px, 0)

    def test_simple_linear_regression0b(self):
        """An imperfect y-t line from bottom left to top right with
        the first and the last entries as outliers.

        In this test case:
          begin segment = [1,]
          end segment = [188, 190]
          middle segment = [20, 40, 60, 80, 100, 120, 140, 160]

          the simple linear regression line is calculated based on the
          middle segment, and is
            y = 20 * t
          the error = [1, 0, 0, 0, 0, 0, 0, 0, 0, 8, 10]
        """
        list_y = [1, 20, 40, 60, 80, 100, 120, 140, 160, 188, 190]
        list_t = range(len(list_y))

        expected_errs_dict = {
            VAL.WHOLE: [1, 0, 0, 0, 0, 0, 0, 0, 0, 8, 10],
            VAL.BEGIN: [1, ],
            VAL.END: [8, 10],
            VAL.BOTH_ENDS: [1, 8, 10],
        }

        for segment_flag, expected_errs in expected_errs_dict.items():
            self.validator._segments= segment_flag
            (max_err, rms_err) = self.validator._calc_errors_single_axis(list_t,
                                                                         list_y)
            expected_max_err = max(expected_errs)
            expected_rms_err = (sum([i ** 2 for i in expected_errs]) /
                                len(expected_errs)) ** 0.5
            self.assertAlmostEqual(max_err, expected_max_err)
            self.assertAlmostEqual(rms_err, expected_rms_err)

    def test_log_details_and_metrics(self):
        """Test the axes in _log_details_and_metrics"""
        # gesture_dir: tests/data/linearity
        gesture_dir = 'linearity'
        filenames_axes = {
            'two_finger_tracking.right_to_left.slow-lumpy-fw_11.27-robot-'
                '20130227_204458.dat': [AXIS.X],
            'one_finger_to_edge.center_to_top.slow-lumpy-fw_11.27-robot-'
                '20130227_203228.dat': [AXIS.Y],
            'two_finger_tracking.bottom_left_to_top_right.normal-lumpy-'
                'fw_11.27-robot-20130227_204902.dat': [AXIS.X, AXIS.Y],
        }
        for filename, expected_axes in filenames_axes.items():
            packets = parse_tests_data(filename, gesture_dir=gesture_dir)
            # get the direction of the gesture
            direction = [filename.split('-')[0].split('.')[1]]
            self.validator.check(packets, direction)
            actual_axes = sorted(self.validator.list_coords.keys())
            self.assertEqual(actual_axes, expected_axes)

    def _test_simple_linear_regression1(self):
        """A y-t line taken from a real example.

        Refer to the "Numerical example" in the wiki page:
            http://en.wikipedia.org/wiki/Simple_linear_regression
        """
        list_t = [1.47, 1.50, 1.52, 1.55, 1.57, 1.60, 1.63, 1.65, 1.68, 1.70,
                  1.73, 1.75, 1.78, 1.80, 1.83]
        list_y = [52.21, 53.12, 54.48, 55.84, 57.20, 58.57, 59.93, 61.29,
                  63.11, 64.47, 66.28, 68.10, 69.92, 72.19, 74.46]
        expected_max_err = 1.3938545467809007
        expected_rms_err = 0.70666155991311708
        (max_err, rms_err) = self.validator._calc_errors_single_axis(
                list_t, list_y)
        self.assertAlmostEqual(max_err, expected_max_err)
        self.assertAlmostEqual(rms_err, expected_rms_err)


class NoGapValidatorTest(unittest.TestCase):
    """Unit tests for NoGapValidator class."""
    GAPS_SUBDIR = 'gaps'

    def setUp(self):
        self.criteria = conf.no_gap_criteria

    def _test_no_gap(self, filename, criteria, device, slot):
        file_subpath = os.path.join(self.GAPS_SUBDIR, filename)
        packets = parse_tests_data(file_subpath)
        validator = NoGapValidator(criteria, device=device, slot=slot)
        vlog = validator.check(packets)
        return vlog.score

    def test_two_finger_scroll_gaps(self):
        """Test that there are gaps in the two finger scroll gesture.

        Issue 7552: Cyapa : two finger scroll motion produces gaps in tracking
        """
        filename = 'two_finger_gaps.horizontal.dat'
        score0 = self._test_no_gap(filename, self.criteria, lumpy, 0)
        score1 = self._test_no_gap(filename, self.criteria, lumpy, 1)
        self.assertTrue(score0 <= 0.1)
        self.assertTrue(score1 <= 0.1)

    def test_gap_new_finger_arriving_or_departing(self):
        """Test gap when new finger arriving or departing.

        Issue: 8005: Cyapa : gaps appear when new finger arrives or departs
        """
        filename = 'gap_new_finger_arriving_or_departing.dat'
        score = self._test_no_gap(filename, self.criteria, lumpy, 0)
        self.assertTrue(score <= 0.3)

    def test_one_stationary_finger_2nd_finger_moving_gaps(self):
        """Test one stationary finger resulting in 2nd finger moving gaps."""
        filename = 'one_stationary_finger_2nd_finger_moving_gaps.dat'
        score = self._test_no_gap(filename, self.criteria, lumpy, 1)
        self.assertTrue(score <= 0.1)

    def test_resting_finger_2nd_finger_moving_gaps(self):
        """Test resting finger resulting in 2nd finger moving gaps.

        Issue 7648: Cyapa : Resting finger plus one finger move generates a gap
        """
        filename = 'resting_finger_2nd_finger_moving_gaps.dat'
        score = self._test_no_gap(filename, self.criteria, lumpy, 1)
        self.assertTrue(score <= 0.3)


class PhysicalClickValidatorTest(unittest.TestCase):
    """Unit tests for PhysicalClickValidator class."""

    def setUp(self):
        self.device = lumpy
        self.criteria = '== 1'
        self.mnprops = MetricNameProps()

    def _test_physical_clicks(self, gesture_dir, files, expected_score):
        gesture_path = os.path.join(unittest_path_lumpy, gesture_dir)
        for filename, fingers in files.items():
            packets = parse_tests_data(os.path.join(gesture_path, filename))
            validator = PhysicalClickValidator(self.criteria,
                                               fingers=fingers,
                                               device=self.device)
            vlog = validator.check(packets)
            actual_score = vlog.score
            self.assertTrue(actual_score == expected_score)

    def test_physical_clicks_success(self):
        """All physcial click files in the gesture_dir should pass."""
        gesture_dir = '20130506_030025-fw_11.27-robot_sim'
        gesture_path = os.path.join(unittest_path_lumpy, gesture_dir)

        # Get all 1f physical click files.
        file_prefix = 'one_finger_physical_click'
        fingers = 1
        files1 = [(filepath, fingers) for filepath in glob.glob(
            os.path.join(gesture_path, file_prefix + '*.dat'))]

        # Get all 2f physical click files.
        file_prefix = 'two_fingers_physical_click'
        fingers = 2
        files2 = [(filepath, fingers) for filepath in glob.glob(
            os.path.join(gesture_path, file_prefix + '*.dat'))]

        # files is a dictionary of {filename: fingers}
        files = dict(files1 + files2)
        expected_score = 1.0
        self._test_physical_clicks(gesture_dir, files, expected_score)

    def test_physical_clicks_failure(self):
        """All physcial click files specified below should fail."""
        gesture_dir = '20130506_032458-fw_11.23-robot_sim'
        # files is a dictionary of {filename: fingers}
        files = {
            'one_finger_physical_click.bottom_side-lumpy-fw_11.23-complete-'
                '20130614_065744.dat': 1,
            'one_finger_physical_click.center-lumpy-fw_11.23-complete-'
                '20130614_065727.dat': 1,
            'two_fingers_physical_click-lumpy-fw_11.23-complete-'
                '20130614_065757.dat': 2,
        }
        expected_score = 0.0
        self._test_physical_clicks(gesture_dir, files, expected_score)

    def test_physical_clicks_by_finger_IDs(self):
        """Test that some physical clicks may come with or without correct
        finger IDs.
        """
        # files is a dictionary of {
        #     filename: (number_fingers, (actual clicks, expected clicks))}
        files = {
                # An incorrect case with 1 finger: the event sequence comprises
                #   Event: ABS_MT_TRACKING_ID, value 284
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: BTN_LEFT, value 1
                #   Event: BTN_LEFT, value 0
                # In this case, the BTN_LEFT occurs when there is no finger.
                '1f_click_incorrect_behind_tid.dat': (1, (0, 1)),

                # A correct case with 1 finger: the event sequence comprises
                #   Event: ABS_MT_TRACKING_ID, value 284
                #   Event: BTN_LEFT, value 1
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: BTN_LEFT, value 0
                # In this case, the BTN_LEFT occurs when there is no finger.
                '1f_click.dat': (1, (1, 1)),

                # An incorrect case with 2 fingers: the event sequence comprises
                #   Event: ABS_MT_TRACKING_ID, value 18
                #   Event: BTN_LEFT, value 1
                #   Event: BTN_LEFT, value 0
                #   Event: ABS_MT_TRACKING_ID, value 19
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: ABS_MT_TRACKING_ID, value -1
                # In this case, the BTN_LEFT occurs when there is only 1 finger.
                '2f_clicks_incorrect_before_2nd_tid.dat': (2, (0, 1)),

                # An incorrect case with 2 fingers: the event sequence comprises
                #   Event: ABS_MT_TRACKING_ID, value 18
                #   Event: ABS_MT_TRACKING_ID, value 19
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: BTN_LEFT, value 1
                #   Event: BTN_LEFT, value 0
                # In this case, the BTN_LEFT occurs when there is only 1 finger.
                '2f_clicks_incorrect_behind_2_tids.dat': (2, (0, 1)),

                # A correct case with 2 fingers: the event sequence comprises
                #   Event: ABS_MT_TRACKING_ID, value 18
                #   Event: ABS_MT_TRACKING_ID, value 19
                #   Event: BTN_LEFT, value 1
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: ABS_MT_TRACKING_ID, value -1
                #   Event: BTN_LEFT, value 0
                # In this case, the BTN_LEFT occurs when there is only 1 finger.
                '2f_clicks.dat': (2, (1, 1)),
        }
        for filename, (fingers, expected_value) in files.items():
            packets = parse_tests_data(filename)
            validator = PhysicalClickValidator(self.criteria, fingers=fingers,
                                               device=dontcare)
            vlog = validator.check(packets)
            metric_name = self.mnprops.CLICK_CHECK_TIDS.format(fingers)
            for metric in vlog.metrics:
                if metric.name == metric_name:
                    self.assertEqual(metric.value, expected_value)


class RangeValidatorTest(unittest.TestCase):
    """Unit tests for RangeValidator class."""

    def setUp(self):
        self.device = lumpy

    def _test_range(self, filename, expected_short_of_range_px):
        filepath = os.path.join(unittest_path_lumpy, filename)
        packets = parse_tests_data(filepath)
        validator = RangeValidator(conf.range_criteria, device=self.device)

        # Extract the gesture variation from the filename
        variation = (filename.split('/')[-1].split('.')[1],)

        # Determine the axis based on the direction in the gesture variation
        axis = (self.device.axis_x if validator.is_horizontal(variation)
                else self.device.axis_y if validator.is_vertical(variation)
                else None)
        self.assertTrue(axis is not None)

        # Convert from pixels to mms.
        expected_short_of_range_mm = self.device.pixel_to_mm_single_axis(
                expected_short_of_range_px, axis)

        vlog = validator.check(packets, variation)

        # There is only one metric in the metrics list.
        self.assertEqual(len(vlog.metrics), 1)
        actual_short_of_range_mm = vlog.metrics[0].value
        self.assertEqual(actual_short_of_range_mm, expected_short_of_range_mm)

    def test_range(self):
        """All physical click files specified below should fail."""
        # files_px is a dictionary of {filename: short_of_range_px}
        files_px = {
            '20130506_030025-fw_11.27-robot_sim/'
            'one_finger_to_edge.center_to_left.slow-lumpy-fw_11.27-'
                'robot_sim-20130506_031554.dat': 0,

            '20130506_030025-fw_11.27-robot_sim/'
            'one_finger_to_edge.center_to_left.slow-lumpy-fw_11.27-'
                'robot_sim-20130506_031608.dat': 0,

            '20130506_032458-fw_11.23-robot_sim/'
            'one_finger_to_edge.center_to_left.slow-lumpy-fw_11.23-'
                'robot_sim-20130506_032538.dat': 1,

            '20130506_032458-fw_11.23-robot_sim/'
            'one_finger_to_edge.center_to_left.slow-lumpy-fw_11.23-'
                'robot_sim-20130506_032549.dat': 1,
        }

        for filename, short_of_range_px in files_px.items():
            self._test_range(filename, short_of_range_px)


class StationaryFingerValidatorTest(unittest.TestCase):
    """Unit tests for StationaryFingerValidator class."""

    def setUp(self):
        self.criteria = conf.stationary_finger_criteria

    def _get_max_distance(self, filename, criteria, device):
        packets = parse_tests_data(filename)
        validator = StationaryFingerValidator(criteria, device=device)
        vlog = validator.check(packets)
        return vlog.metrics[0].value

    def test_stationary_finger_shift(self):
        """Test that the stationary shift due to 2nd finger tapping.

        Issue 7442: Cyapa : Second finger tap events influence stationary finger
        position
        """
        filename = 'stationary_finger_shift_with_2nd_finger_tap.dat'
        max_distance = self._get_max_distance(filename, self.criteria, lumpy)
        self.assertAlmostEqual(max_distance, 5.464430436926)

    def test_stationary_strongly_affected_by_2nd_moving_finger(self):
        """Test stationary finger strongly affected by 2nd moving finger with
        gaps.

        Issue 5812: [Cypress] reported positions of stationary finger strongly
        affected by nearby moving finger
        """
        filename = ('stationary_finger_strongly_affected_by_2nd_moving_finger_'
                    'with_gaps.dat')
        max_distance = self._get_max_distance(filename, self.criteria, lumpy)
        self.assertAlmostEqual(max_distance, 4.670861210146)


class StationaryTapValidatorTest(unittest.TestCase):
    """Unit tests for StationaryTapValidator class."""

    def setUp(self):
        self.criteria = conf.stationary_tap_criteria

    def test_stationary_tap(self):
        filenames = {'1f_click.dat': 1.718284027744,
                     '1f_clickb.dat': 0.577590781705}
        for filename, expected_max_distance in filenames.items():
            packets = parse_tests_data(filename)
            validator = StationaryTapValidator(self.criteria, device=lumpy)
            vlog = validator.check(packets)
            actual_max_distance = vlog.metrics[0].value
            self.assertAlmostEqual(actual_max_distance, expected_max_distance)


class NoLevelJumpValidatorTest(unittest.TestCase):
    """Unit tests for NoLevelJumpValidator class."""

    def setUp(self):
        self.criteria = conf.no_level_jump_criteria
        self.gesture_dir = 'drag_edge_thumb'

    def _get_score(self, filename, device):
        validator = NoLevelJumpValidator(self.criteria, device=device,
                                         slots=[0,])
        packets = parse_tests_data(filename, gesture_dir=self.gesture_dir)
        vlog = validator.check(packets)
        score = vlog.score
        return score

    def test_level_jumps(self):
        """Test files with level jumps."""
        filenames = [
            'drag_edge_thumb.horizontal.dat',
            'drag_edge_thumb.horizontal_2.dat',
            'drag_edge_thumb.horizontal_3.no_points.dat',
            'drag_edge_thumb.vertical.dat',
            'drag_edge_thumb.vertical_2.dat',
            'drag_edge_thumb.diagonal.dat',
        ]
        for filename in filenames:
            self.assertTrue(self._get_score(filename, lumpy) <= 0.6)

    def test_no_level_jumps(self):
        """Test files without level jumps."""
        filenames = [
            'drag_edge_thumb.horizontal.curvy.dat',
            'drag_edge_thumb.horizontal_2.curvy.dat',
            'drag_edge_thumb.vertical.curvy.dat',
            'drag_edge_thumb.vertical_2.curvy.dat',
        ]
        for filename in filenames:
            self.assertTrue(self._get_score(filename, lumpy) == 1.0)


class ReportRateValidatorTest(unittest.TestCase):
    """Unit tests for ReportRateValidator class."""
    def setUp(self):
        self.criteria = '>= 60'

    def _get_score(self, filename, device):
        validator = ReportRateValidator(self.criteria, device=device,
                                        chop_off_pauses=False)
        packets = parse_tests_data(filename)
        vlog = validator.check(packets)
        score = vlog.score
        return score

    def test_report_rate_scores(self):
        """Test the score of the report rate."""
        filename = '2f_scroll_diagonal.dat'
        self.assertTrue(self._get_score(filename, device=lumpy) <= 0.5)

        filename = 'one_finger_with_slot_0.dat'
        self.assertTrue(self._get_score(filename, device=lumpy) >= 0.9)

        filename = 'two_close_fingers_merging_changed_ids_gaps.dat'
        self.assertTrue(self._get_score(filename, device=lumpy) <= 0.5)

    def test_report_rate_without_slot(self):
        """Test report rate without specifying any slot."""
        filename_report_rate_pair = [
            ('2f_scroll_diagonal.dat', 40.31),
            ('one_finger_with_slot_0.dat', 148.65),
            ('two_close_fingers_merging_changed_ids_gaps.dat', 53.12),
        ]
        for filename, expected_report_rate in filename_report_rate_pair:
            validator = ReportRateValidator(self.criteria, device=dontcare,
                                            chop_off_pauses=False)
            validator.check(parse_tests_data(filename))
            actual_report_rate = round(validator.report_rate, 2)
            self.assertAlmostEqual(actual_report_rate, expected_report_rate)

    def test_report_rate_with_slot(self):
        """Test report rate with slot=1"""
        # Compute actual_report_rate
        filename = ('stationary_finger_strongly_affected_by_2nd_moving_finger_'
                    'with_gaps.dat')
        validator = ReportRateValidator(self.criteria, device=dontcare,
                                        finger=1, chop_off_pauses=False)
        validator.check(parse_tests_data(filename))
        actual_report_rate = validator.report_rate
        # Compute expected_report_rate
        first_syn_time = 2597.682925
        last_syn_time = 2604.543335
        num_packets = 592 - 1
        expected_report_rate = num_packets / (last_syn_time - first_syn_time)
        self.assertAlmostEqual(actual_report_rate, expected_report_rate)

    def _test_report_rate_metrics(self, filename, expected_values):
        packets = parse_tests_data(filename)
        validator = ReportRateValidator(self.criteria, device=lumpy,
                                        chop_off_pauses=False)
        vlog = validator.check(packets)

        # Verify that there are 3 metrics
        number_metrics = 3
        self.assertEqual(len(vlog.metrics), number_metrics)

        # Verify the values of the 3 metrics.
        for i in range(number_metrics):
            actual_value = vlog.metrics[i].value
            if isinstance(actual_value, tuple):
                self.assertEqual(actual_value, expected_values[i])
            else:
                self.assertAlmostEqual(actual_value, expected_values[i])

    def test_report_rate_metrics(self):
        """Test the metrics of the report rates."""
        # files is a dictionary of
        #       {filename: ((# long_intervals, # all intervals),
        #                    ave_interval, max_interval)}
        files = {
            '2f_scroll_diagonal.dat':
                ((33, 33), 24.8057272727954, 26.26600000075996),
            'one_finger_with_slot_0.dat':
                ((1, 12), 6.727166666678386, 20.411999998032115),
            'two_close_fingers_merging_changed_ids_gaps.dat':
                ((13, 58), 18.82680942272318, 40.936946868896484),
        }

        for filename, values in files.items():
            self._test_report_rate_metrics(filename, values)

    def _test_chop_off_both_ends(self, xy_pairs, distance, expected_middle):
        """Verify if the actual middle is equal to the expected middle."""
        points = [Point(*xy) for xy in xy_pairs]
        validator = ReportRateValidator(self.criteria, device=dontcare)
        actual_middle = validator._chop_off_both_ends(points, distance)
        self.assertEqual(actual_middle, expected_middle)

    def test_chop_off_both_ends0(self):
        """Test chop_off_both_ends() with distinct distances."""
        xy_pairs = [
                # pauses
                (100, 20), (100, 21), (101, 22), (102, 24), (103, 26),
                # moving segment
                (120, 30), (122, 29), (123, 32), (123, 33), (126, 35),
                (126, 32), (142, 29), (148, 30), (159, 31), (162, 30),
                (170, 32), (183, 32), (194, 32), (205, 32), (208, 32),
                # pauses
                (230, 30), (231, 31), (232, 30), (231, 30), (230, 30),
        ]

        distance = 20
        expected_begin_index = 5
        expected_end_index = 19
        expected_middle = [expected_begin_index, expected_end_index]
        self._test_chop_off_both_ends(xy_pairs, distance, expected_middle)

        distance = 0
        expected_begin_index = 0
        expected_end_index = len(xy_pairs) - 1
        expected_middle = [expected_begin_index, expected_end_index]
        self._test_chop_off_both_ends(xy_pairs, distance, expected_middle)

    def test_chop_off_both_ends1(self):
        """Test chop_off_both_ends() with some corner cases"""
        distance = 20
        xy_pairs = [(120, 50), (120, 50)]
        expected_middle = None
        self._test_chop_off_both_ends(xy_pairs, distance, expected_middle)

        xy_pairs = [(120, 50), (150, 52), (200, 51)]
        expected_middle = [1, 1]
        self._test_chop_off_both_ends(xy_pairs, distance, expected_middle)

        xy_pairs = [(120, 50), (120, 51), (200, 52), (200, 51)]
        expected_middle = None
        self._test_chop_off_both_ends(xy_pairs, distance, expected_middle)


class HysteresisValidatorTest(unittest.TestCase):
    """Unit tests for HysteresisValidator class."""

    def setUp(self):
        self.criteria = conf.hysteresis_criteria

    def test_hysteresis(self):
        """Test that the hysteresis causes an initial jump."""
        filenames = {'center_to_right_normal_link.dat': 4.6043458,
                     'center_to_right_slow_link.dat': 16.8671278}

        for filename, expected_value in filenames.items():
            packets = parse_tests_data(filename)
            validator = HysteresisValidator(self.criteria, device=link)
            vlog = validator.check(packets)
            self.assertAlmostEqual(vlog.metrics[0].value, expected_value)

    def test_click_data(self):
        """Test that the validator handles None distances well.

        In this test, distance1 = None and distance2 = None.
        This results in ratio = infinity. There should be no error incurred.
        """
        packets = parse_tests_data('2f_clicks_test_hysteresis.dat')
        validator = HysteresisValidator(self.criteria, device=link)
        vlog = validator.check(packets)
        self.assertEqual(vlog.metrics[0].value, float('infinity'))


class MtbSanityValidatorTest(unittest.TestCase):
    """Unit tests for MtbSanityValidator class."""

    def setUp(self):
        import fake_input_device
        self.fake_device_info = fake_input_device.FakeInputDevice()

    def _get_number_errors(self, filename):
        packets = parse_tests_data(filename)
        validator = MtbSanityValidator(device=link,
                                       device_info=self.fake_device_info)
        vlog = validator.check(packets)
        number_errors, _ = vlog.metrics[1].value
        return number_errors

    def test_sanity_found_errors(self):
        """Test that the tracking id is set to -1 before being assigned a
        positive value.
        """
        filenames = ['finger_crossing.top_right_to_bottom_left.slow.dat',
                     'two_finger_tap.vertical.dat']
        for filename in filenames:
            number_errors = self._get_number_errors(filename)
            self.assertTrue(number_errors > 0)

    def test_sanity_pass(self):
        """Test that the MTB format is correct."""
        filenames = ['2f_scroll_diagonal.dat',
                     'drumroll_lumpy.dat']
        for filename in filenames:
            number_errors = self._get_number_errors(filename)
            self.assertTrue(number_errors == 0)


class DiscardInitialSecondsValidatorTest(unittest.TestCase):
    """Unit tests for DiscardInitialSecondsValidator class."""

    def setUp(self):
        import fake_input_device
        self.fake_device_info = fake_input_device.FakeInputDevice()

    def _get_score(self, filename, criteria_str):
        packets = parse_tests_data(filename)
        validator = DiscardInitialSecondsValidator(
            validator=CountTrackingIDValidator(criteria_str, device=link),
            device=link)
        vlog = validator.check(packets)
        return vlog.score

    def test_single_finger_hold(self):
        """Test that the state machine reads one finger if
        only one finger was held for over a second."""

        filename = 'one_finger_long_hold.dat'
        score = self._get_score(filename, '== 1')
        self.assertTrue(score == 1)

    def test_double_finger_hold(self):
        """Test that the state machine reads two fingers if
        two fingers were held for over a second."""

        filename = 'two_finger_long_hold.dat'
        score = self._get_score(filename, '== 2')
        self.assertTrue(score == 1)

    def test_double_tap_single_hold(self):
        """Test that the validator discards the initial double tap and only
        validates on the single finger long hold at the end.
        """

        filename = 'two_finger_tap_one_finger_hold.dat'
        score = self._get_score(filename, '== 1')
        self.assertTrue(score == 1)

    def test_discard_initial_seconds(self):
        """Test that discard_initial_seconds() cuts at the proper packet.

        Note: to print the final_state_packet, use the following statements:
            import mtb
            print mtb.make_pretty_packet(final_state_packet)
        """
        packets = parse_tests_data('noise_stationary_extended.dat')
        validator = DiscardInitialSecondsValidator(
            validator=CountTrackingIDValidator('== 1', device=link),
            device=link)
        validator.init_check(packets)
        packets = validator._discard_initial_seconds(packets, 1)
        final_state_packet = packets[0]

        self.assertTrue(len(final_state_packet) == 11)
        # Assert the correctness of the 1st finger data in the order of
        #     SLOT, TRACKING_ID, POSITION_X, POSITION_Y, PRESSURE
        self.assertTrue(final_state_packet[0][MTB.EV_VALUE] == 2)
        self.assertTrue(final_state_packet[1][MTB.EV_VALUE] == 2427)
        self.assertTrue(final_state_packet[2][MTB.EV_VALUE] == 670)
        self.assertTrue(final_state_packet[3][MTB.EV_VALUE] == 361)
        self.assertTrue(final_state_packet[4][MTB.EV_VALUE] == 26)
        # Assert the correctness of the 2nd finger data in the order of
        #     SLOT, TRACKING_ID, POSITION_X, POSITION_Y, PRESSURE
        self.assertTrue(final_state_packet[5][MTB.EV_VALUE] == 3)
        self.assertTrue(final_state_packet[6][MTB.EV_VALUE] == 2426)
        self.assertTrue(final_state_packet[7][MTB.EV_VALUE] == 670)
        self.assertTrue(final_state_packet[8][MTB.EV_VALUE] == 368)
        self.assertTrue(final_state_packet[9][MTB.EV_VALUE] == 21)
        # EVENT TIME
        self.assertTrue(final_state_packet[0][MTB.EV_TIME] == 1412021965.723953)

    def test_get_snapshot_after_discarding_init_packets(self):
        """Test that get_snapshot() handles non-ready packet properly
        after discard_initial_seconds(). A non-ready packet is one that
        the attributes such as X, Y, and Z are not all ready.
        """
        packets = parse_tests_data('non_ready_events_in_final_state_packet.dat')
        validator = DiscardInitialSecondsValidator(
            validator=CountTrackingIDValidator('== 1', device=link),
            device=link)
        validator.init_check(packets)
        packets = validator._discard_initial_seconds(packets, 1)
        final_state_packet = packets[0]

        self.assertTrue(len(final_state_packet) == 4)
        # Assert the correctness of the finger data in the order of
        #     SLOT, TRACKING_ID, and POSITION_Y
        self.assertTrue(final_state_packet[0][MTB.EV_VALUE] == 0)
        self.assertTrue(final_state_packet[1][MTB.EV_VALUE] == 102)
        self.assertTrue(final_state_packet[2][MTB.EV_VALUE] == 1316)
        # EVENT TIME
        self.assertTrue(final_state_packet[0][MTB.EV_TIME] == 1412888977.716634)

    def test_noise_line_with_all_fingers_left(self):
        """In this test case, all fingers left. The final_state_packet is []."""
        packets=parse_tests_data('noise_line.dat')
        validator = DiscardInitialSecondsValidator(ReportRateValidator('>= 60'))
        validator.init_check(packets)
        packets = validator._discard_initial_seconds(packets, 1)
        validator.validator.init_check(packets)
        list_syn_time = validator.validator.packets.get_list_syn_time([])
        self.assertEqual(len(packets), 84)
        self.assertEqual(len(list_syn_time), 84)


if __name__ == '__main__':
  unittest.main()
