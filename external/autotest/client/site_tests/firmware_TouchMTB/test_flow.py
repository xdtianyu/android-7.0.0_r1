# -*- coding: utf-8 -*-

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Guide the user to perform gestures. Record and validate the gestures."""

import fcntl
import glob
import os
import subprocess
import sys

import common_util
import firmware_log
import firmware_utils
import fuzzy
import mini_color
import mtb
import touchbotII_robot_wrapper as robot_wrapper
import test_conf as conf
import validators

from firmware_utils import GestureList

sys.path.append('../../bin/input')
import input_device

# Include some constants
from firmware_constants import DEV, GV, MODE, OPTIONS, TFK


class TestFlow:
    """Guide the user to perform gestures. Record and validate the gestures."""

    def __init__(self, device_geometry, device, keyboard, win, parser, output,
                 test_version, board, firmware_version, options):
        self.device_geometry = device_geometry
        self.device = device
        self.device_node = self.device.device_node
        self.keyboard = keyboard
        self.firmware_version = firmware_version
        self.output = output
        self.board = board
        self.test_version = test_version
        self.output.print_report('%s' % test_version)
        self._get_record_cmd()
        self.win = win
        self.parser = parser
        self.packets = None
        self.gesture_file_name = None
        self.prefix_space = self.output.get_prefix_space()
        self.scores = []
        self.mode = options[OPTIONS.MODE]
        self.fngenerator_only = options[OPTIONS.FNGENERATOR]
        self.iterations = options[OPTIONS.ITERATIONS]
        self.replay_dir = options[OPTIONS.REPLAY]
        self.resume_dir = options[OPTIONS.RESUME]
        self.recording = not any([bool(self.replay_dir), bool(self.resume_dir)])
        self.device_type = (DEV.TOUCHSCREEN if options[OPTIONS.TOUCHSCREEN]
                                            else DEV.TOUCHPAD)

        self.robot = robot_wrapper.RobotWrapper(self.board, options)
        self.robot_waiting = False

        self.gv_count = float('infinity')
        gesture_names = self._get_gesture_names()
        order = None
        if self._is_robot_mode():
            order = lambda x: conf.finger_tips_required[x.name]
        self.gesture_list = GestureList(gesture_names).get_gesture_list(order)
        self._get_all_gesture_variations(options[OPTIONS.SIMPLIFIED])

        self.init_flag = False
        self.system_device = self._non_blocking_open(self.device_node)
        self.evdev_device = input_device.InputEvent()
        self.screen_shot = firmware_utils.ScreenShot(self.geometry_str)
        self.mtb_evemu = mtb.MtbEvemu(device)

        self._rename_old_log_and_html_files()
        self._set_static_prompt_messages()
        self.gesture_image_name = None
        self.gesture_continues_flag = False
        self.use_existent_event_file_flag = False

    def __del__(self):
        self.system_device.close()

    def _rename_old_log_and_html_files(self):
        """When in replay or resume mode, rename the old log and html files."""
        if self.replay_dir or self.resume_dir:
            for file_type in ['*.log', '*.html']:
                path_names = os.path.join(self.output.log_dir, file_type)
                for old_path_name in glob.glob(path_names):
                    new_path_name = '.'.join([old_path_name, 'old'])
                    os.rename(old_path_name, new_path_name)

    def _is_robot_mode(self):
        return self.robot.is_robot_action_mode() or self.mode == MODE.ROBOT_SIM

    def _get_gesture_names(self):
        """Determine the gesture names based on the mode."""
        if self.mode == MODE.QUICKSTEP:
            return conf.gesture_names_quickstep
        elif self.mode == MODE.NOISE:
            return conf.gesture_names_noise_extended
        elif self._is_robot_mode():
            # The mode could be MODE.ROBOT or MODE.ROBOT_SIM.
            # The same gesture names list is used in both modes.
            return conf.gesture_names_robot[self.device_type]
        elif self.mode == MODE.MANUAL:
            # Define the manual list which is gesture_names_complete:
            # gesture_names_robot - gesture_names_equipment_required
            manual_set = (set(conf.gesture_names_complete[self.device_type]) -
                          set(conf.gesture_names_robot[self.device_type]))
            return list(manual_set - set(conf.gesture_names_fngenerator_required))

        elif self.mode == MODE.CALIBRATION:
            return conf.gesture_names_calibration
        else:
            # Filter out tests that need a function generator for COMPLETE mode
            # unless they've indicated that they have one
            return [n for n in conf.gesture_names_complete[self.device_type]
                    if (self.fngenerator_only or
                        n not in conf.gesture_names_fngenerator_required)]

    def _non_blocking_open(self, filename):
        """Open the file in non-blocing mode."""
        fd = open(filename)
        fcntl.fcntl(fd, fcntl.F_SETFL, os.O_NONBLOCK)
        return fd

    def _non_blocking_read(self, dev, fd):
        """Non-blocking read on fd."""
        try:
            dev.read(fd)
            event = (dev.tv_sec, dev.tv_usec, dev.type, dev.code, dev.value)
        except Exception, e:
            event = None
        return event

    def _reopen_system_device(self):
        """Close the device and open a new one."""
        self.system_device.close()
        self.system_device = open(self.device_node)
        self.system_device = self._non_blocking_open(self.device_node)

    def _set_static_prompt_messages(self):
        """Set static prompt messages."""
        # Prompt for next gesture.
        self._prompt_next = (
                "Press SPACE to save this file and go to next test,\n"
                "      'm'   to save this file and record again,\n"
                "      'd'   to delete this file and try again,\n"
                "      'x'   to discard this file and exit.")

        # Prompt to see test result through timeout callback.
        self._prompt_result = (
                "Perform the gesture now.\n"
                "See the test result on the right after finger lifted.\n"
                "Or press 'x' to exit.")

    def _get_prompt_abnormal_gestures(self, warn_msg):
        """Prompt for next gesture."""
        prompt = '\n'.join(
                ["It is very likely that you perform a WRONG gesture!",
                 warn_msg,
                 "Press 'd'   to delete this file and try again (recommended),",
                 "      SPACE to save this file if you are sure it's correct,",
                 "      'x'   to discard this file and exit."])
        return prompt

    def _get_prompt_no_data(self):
        """Prompt to remind user of performing gestures."""
        prompt = ("You need to perform the specified gestures "
                  "before pressing SPACE.\n")
        return prompt + self._prompt_result

    def _get_record_cmd(self):
        """Get the device event record command."""
        # Run mtplot with settings to disable clearing the display if the robot
        # clicks the pad, and adding a visible click indicator in the output
        self.record_program = 'mtplot -s1 -c0 -m0'
        if not common_util.program_exists(self.record_program):
            msg = 'Error: the program "%s" does not exist in $PATH.'
            self.output.print_report(msg % self.record_program)
            exit(1)

        display_name = firmware_utils.get_display_name()
        self.geometry_str = '%dx%d+%d+%d' % self.device_geometry
        format_str = '%s %s -d %s -g %s'
        self.record_cmd = format_str % (self.record_program,
                                        self.device_node,
                                        display_name,
                                        self.geometry_str)
        self.output.print_report('Record program: %s' % self.record_cmd)

    def _span_seq(self, seq1, seq2):
        """Span sequence seq1 over sequence seq2.

        E.g., seq1 = (('a', 'b'), 'c')
              seq2 = ('1', ('2', '3'))
              res = (('a', 'b', '1'), ('a', 'b', '2', '3'),
                     ('c', '1'), ('c', '2', '3'))
        E.g., seq1 = ('a', 'b')
              seq2 = ('1', '2', '3')
              res  = (('a', '1'), ('a', '2'), ('a', '3'),
                      ('b', '1'), ('b', '2'), ('b', '3'))
        E.g., seq1 = (('a', 'b'), ('c', 'd'))
              seq2 = ('1', '2', '3')
              res  = (('a', 'b', '1'), ('a', 'b', '2'), ('a', 'b', '3'),
                      ('c', 'd', '1'), ('c', 'd', '2'), ('c', 'd', '3'))
        """
        to_list = lambda s: list(s) if isinstance(s, tuple) else [s]
        return tuple(tuple(to_list(s1) + to_list(s2)) for s1 in seq1
                                                      for s2 in seq2)

    def span_variations(self, seq):
        """Span the variations of a gesture."""
        if seq is None:
            return (None,)
        elif isinstance(seq[0], tuple):
            return reduce(self._span_seq, seq)
        else:
            return seq

    def _stop(self):
        """Terminate the recording process."""
        self.record_proc.poll()
        # Terminate the process only when it was not terminated yet.
        if self.record_proc.returncode is None:
            self.record_proc.terminate()
            self.record_proc.wait()
        self.output.print_window('')

    def _get_gesture_image_name(self):
        """Get the gesture file base name without file extension."""
        filepath = os.path.splitext(self.gesture_file_name)[0]
        self.gesture_image_name = filepath + '.png'
        return filepath

    def _close_gesture_file(self):
        """Close the gesture file."""
        if self.gesture_file.closed:
            return

        filename = self.gesture_file.name
        self.gesture_file.close()

        # Strip off the header of the gesture file.
        #
        # Input driver version is 1.0.1
        # Input device ID: bus 0x18 vendor 0x0 product 0x0 version 0x0
        # Input device name: "Atmel maXTouch Touchpad"
        # ...
        # Testing ... (interrupt to exit)
        # Event: time 519.855, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID),
        #                                       value 884
        #
        tmp_filename = filename + '.tmp'
        os.rename(filename, tmp_filename)
        with open(tmp_filename) as src_f:
            with open(filename, 'w') as dst_f:
                for line in src_f:
                    if line.startswith('Event:'):
                        dst_f.write(line)
        os.remove(tmp_filename)

    def _stop_record_and_post_image(self):
        """Terminate the recording process."""
        if self.record_new_file:
            self._close_gesture_file()
            self.screen_shot.dump_root(self._get_gesture_image_name())
            self.record_proc.terminate()
            self.record_proc.wait()
        else:
            self._get_gesture_image_name()
        self.win.set_image(self.gesture_image_name)

    def _create_prompt(self, test, variation):
        """Create a color prompt."""
        prompt = test.prompt
        if isinstance(variation, tuple):
            subprompt = reduce(lambda s1, s2: s1 + s2,
                               tuple(test.subprompt[s] for s in variation))
        elif variation is None or test.subprompt is None:
            subprompt = None
        else:
            subprompt = test.subprompt[variation]

        if subprompt is None:
            color_prompt = prompt
            monochrome_prompt = prompt
        else:
            color_prompt = mini_color.color_string(prompt, '{', '}', 'green')
            color_prompt = color_prompt.format(*subprompt)
            monochrome_prompt = prompt.format(*subprompt)

        color_msg_format = mini_color.color_string('\n<%s>:\n%s%s', '<', '>',
                                                   'blue')
        color_msg = color_msg_format % (test.name, self.prefix_space,
                                        color_prompt)
        msg = '%s: %s' % (test.name, monochrome_prompt)

        glog = firmware_log.GestureLog()
        glog.name = test.name
        glog.variation = variation
        glog.prompt = monochrome_prompt

        return (msg, color_msg, glog)

    def _choice_exit(self):
        """Procedure to exit."""
        self._stop()
        if os.path.exists(self.gesture_file_name):
            os.remove(self.gesture_file_name)
            self.output.print_report(self.deleted_msg)

    def _stop_record_and_rm_file(self):
        """Stop recording process and remove the current gesture file."""
        self._stop()
        if os.path.exists(self.gesture_file_name):
            os.remove(self.gesture_file_name)
            self.output.print_report(self.deleted_msg)

    def _create_gesture_file_name(self, gesture, variation):
        """Create the gesture file name based on its variation.

        Examples of different levels of file naming:
            Primary name:
                pinch_to_zoom.zoom_in-lumpy-fw_11.27
            Root name:
                pinch_to_zoom.zoom_in-lumpy-fw_11.27-manual-20130221_050510
            Base name:
                pinch_to_zoom.zoom_in-lumpy-fw_11.27-manual-20130221_050510.dat
        """
        if variation is None:
            gesture_name = gesture.name
        else:
            if type(variation) is tuple:
                name_list = [gesture.name,] + list(variation)
            else:
                name_list = [gesture.name, variation]
            gesture_name = '.'.join(name_list)

        self.primary_name = conf.filename.sep.join([
                gesture_name,
                self.board,
                conf.fw_prefix + self.firmware_version])
        root_name = conf.filename.sep.join([
                self.primary_name,
                self.mode,
                firmware_utils.get_current_time_str()])
        basename = '.'.join([root_name, conf.filename.ext])
        return basename

    def _add_scores(self, new_scores):
        """Add the new scores of a single gesture to the scores list."""
        if new_scores is not None:
            self.scores += new_scores

    def _final_scores(self, scores):
        """Print the final score."""
        # Note: conf.score_aggregator uses a function in fuzzy module.
        final_score = eval(conf.score_aggregator)(scores)
        self.output.print_report('\nFinal score: %s\n' % str(final_score))

    def _robot_action(self):
        """Control the robot to perform the action."""
        if self._is_robot_mode() or self.robot.is_manual_noise_test_mode():
            self.robot.configure_noise(self.gesture, self.variation)

        if self._is_robot_mode():
            self.robot.control(self.gesture, self.variation)
            # Once the script terminates start a timeout to clean up if one
            # hasn't already been set to keep the test suite from hanging.
            if not self.gesture_begins_flag:
                self.win.register_timeout_add(self.gesture_timeout_callback,
                                              self.gesture.timeout)

    def _handle_user_choice_save_after_parsing(self, next_gesture=True):
        """Handle user choice for saving the parsed gesture file."""
        self.output.print_window('')
        if self.use_existent_event_file_flag or self.recording:
            if self.saved_msg:
                self.output.print_report(self.saved_msg)
            if self.new_scores:
                self._add_scores(self.new_scores)
            self.output.report_html.insert_image(self.gesture_image_name)
            self.output.report_html.flush()
        # After flushing to report_html, reset the gesture_image_name so that
        # it will not be reused by next gesture variation accidentally.
        self.gesture_image_name = None

        if self._pre_setup_this_gesture_variation(next_gesture=next_gesture):
            # There are more gestures.
            self._setup_this_gesture_variation()
            self._robot_action()
        else:
            # No more gesture.
            self._final_scores(self.scores)
            self.output.stop()
            self.output.report_html.stop()
            self.win.stop()
        self.packets = None

    def _handle_user_choice_discard_after_parsing(self):
        """Handle user choice for discarding the parsed gesture file."""
        self.output.print_window('')
        self._setup_this_gesture_variation()
        self._robot_action()
        self.packets = None

    def _handle_user_choice_exit_after_parsing(self):
        """Handle user choice to exit after the gesture file is parsed."""
        self._stop_record_and_rm_file()
        self.output.stop()
        self.output.report_html.stop()
        self.win.stop()

    def check_for_wrong_number_of_fingers(self, details):
        flag_found = False
        try:
            position = details.index('CountTrackingIDValidator')
        except ValueError as e:
            return None

        # An example of the count of tracking IDs:
        #     '    count of trackid IDs: 1'
        number_tracking_ids = int(details[position + 1].split()[-1])
        # An example of the criteria_str looks like:
        #     '    criteria_str: == 2'
        criteria = int(details[position + 2].split()[-1])
        if number_tracking_ids < criteria:
            print '  CountTrackingIDValidator: '
            print '  number_tracking_ids: ', number_tracking_ids
            print '  criteria: ', criteria
            print '  number_tracking_ids should be larger!'
            msg = 'Number of Tracking IDs should be %d instead of %d'
            return msg % (criteria, number_tracking_ids)
        return None

    def _empty_packets_is_legal_result(self):
        return ('tap' in self.gesture.name and self._is_robot_mode())

    def _handle_user_choice_validate_before_parsing(self):
        """Handle user choice for validating before gesture file is parsed."""
        # Parse the device events. Make sure there are events.
        self.packets = self.parser.parse_file(self.gesture_file_name)
        if self.packets or self._empty_packets_is_legal_result():
            # Validate this gesture and get the results.
            (self.new_scores, msg_list, vlogs) = validators.validate(
                    self.packets, self.gesture, self.variation)

            # If the number of tracking IDs is less than the expected value,
            # the user probably made a wrong gesture.
            error = self.check_for_wrong_number_of_fingers(msg_list)
            if error:
                prompt = self._get_prompt_abnormal_gestures(error)
                color = 'red'
            else:
                prompt = self._prompt_next
                color = 'black'

            self.output.print_window(msg_list)
            self.output.buffer_report(msg_list)
            self.output.report_html.insert_validator_logs(vlogs)
            self.win.set_prompt(prompt, color=color)
            print prompt
            self._stop_record_and_post_image()
        else:
            self.win.set_prompt(self._get_prompt_no_data(), color='red')

    def _handle_user_choice_exit_before_parsing(self):
        """Handle user choice to exit before the gesture file is parsed."""
        self._close_gesture_file()
        self._handle_user_choice_exit_after_parsing()

    def _is_parsing_gesture_file_done(self):
        """Is parsing the gesture file done?"""
        return self.packets is not None

    def _is_arrow_key(self, choice):
        """Is this an arrow key?"""
        return (choice in TFK.ARROW_KEY_LIST)

    def user_choice_callback(self, fd, condition):
        """A callback to handle the key pressed by the user.

        This is the primary GUI event-driven method handling the user input.
        """
        choice = self.keyboard.get_key_press_event(fd)
        if choice:
            self._handle_keyboard_event(choice)
        return True

    def _handle_keyboard_event(self, choice):
        """Handle the keyboard event."""
        if self._is_arrow_key(choice):
            self.win.scroll(choice)
        elif self.robot_waiting:
            # The user wants the robot to start its action.
            if choice in (TFK.SAVE, TFK.SAVE2):
                self.robot_waiting = False
                self._robot_action()
            # The user wants to exit.
            elif choice == TFK.EXIT:
                self._handle_user_choice_exit_after_parsing()
        elif self._is_parsing_gesture_file_done():
            # Save this gesture file and go to next gesture.
            if choice in (TFK.SAVE, TFK.SAVE2):
                self._handle_user_choice_save_after_parsing()
            # Save this file and perform the same gesture again.
            elif choice == TFK.MORE:
                self._handle_user_choice_save_after_parsing(next_gesture=False)
            # Discard this file and perform the gesture again.
            elif choice == TFK.DISCARD:
                self._handle_user_choice_discard_after_parsing()
            # The user wants to exit.
            elif choice == TFK.EXIT:
                self._handle_user_choice_exit_after_parsing()
            # The user presses any wrong key.
            else:
                self.win.set_prompt(self._prompt_next, color='red')
        else:
            if choice == TFK.EXIT:
                self._handle_user_choice_exit_before_parsing()
            # The user presses any wrong key.
            else:
                self.win.set_prompt(self._prompt_result, color='red')

    def _get_all_gesture_variations(self, simplified):
        """Get all variations for all gestures."""
        gesture_variations_list = []
        self.variations_dict = {}
        for gesture in self.gesture_list:
            variations_list = []
            variations = self.span_variations(gesture.variations)
            for variation in variations:
                gesture_variations_list.append((gesture, variation))
                variations_list.append(variation)
                if simplified:
                    break
            self.variations_dict[gesture.name] = variations_list
        self.gesture_variations = iter(gesture_variations_list)

    def gesture_timeout_callback(self):
        """A callback watching whether a gesture has timed out."""
        if self.replay_dir:
            # There are event files to replay for this gesture variation.
            if self.use_existent_event_file_flag:
                self._handle_user_choice_validate_before_parsing()
            self._handle_user_choice_save_after_parsing(next_gesture=True)
            return False

        # A gesture is stopped only when two conditions are met simultaneously:
        # (1) there are no reported packets for a timeout interval, and
        # (2) the number of tracking IDs is 0.
        elif (self.gesture_continues_flag or
            not self.mtb_evemu.all_fingers_leaving()):
            self.gesture_continues_flag = False
            return True

        else:
            self._handle_user_choice_validate_before_parsing()
            self.win.remove_event_source(self.gesture_file_watch_tag)
            if self._is_robot_mode():
                self._handle_keyboard_event(TFK.SAVE)
            return False

    def gesture_file_watch_callback(self, fd, condition, evdev_device):
        """A callback to watch the device input."""
        # Read the device node continuously until end
        event = True
        while event:
            event = self._non_blocking_read(evdev_device, fd)
            if event:
                self.mtb_evemu.process_event(event)

        self.gesture_continues_flag = True
        if (not self.gesture_begins_flag):
            self.gesture_begins_flag = True
            self.win.register_timeout_add(self.gesture_timeout_callback,
                                          self.gesture.timeout)
        return True

    def init_gesture_setup_callback(self, widget, event):
        """A callback to set up environment before a user starts a gesture."""
        if not self.init_flag:
            self.init_flag = True
            self._pre_setup_this_gesture_variation()
            self._setup_this_gesture_variation()
            self._robot_action()

    def _get_existent_event_files(self):
        """Get the existent event files that starts with the primary_name."""
        primary_pathnames = os.path.join(self.output.log_dir,
                                         self.primary_name + '*.dat')
        self.primary_gesture_files = glob.glob(primary_pathnames)
        # Reverse sorting the file list so that we could pop from the tail.
        self.primary_gesture_files.sort()
        self.primary_gesture_files.reverse()

    def _use_existent_event_file(self):
        """If the replay flag is set in the command line, and there exists a
        file(s) with the same primary name, then use the existent file(s)
        instead of recording a new one.
        """
        if self.primary_gesture_files:
            self.gesture_file_name = self.primary_gesture_files.pop()
            return True
        return False

    def _pre_setup_this_gesture_variation(self, next_gesture=True):
        """Get gesture, variation, filename, prompt, etc."""
        next_gesture_first_time = False
        if next_gesture:
            if self.gv_count < self.iterations:
                self.gv_count += 1
            else:
                self.gv_count = 1
                gesture_variation = next(self.gesture_variations, None)
                if gesture_variation is None:
                    return False
                self.gesture, self.variation = gesture_variation
                next_gesture_first_time = True

        basename = self._create_gesture_file_name(self.gesture, self.variation)
        if next_gesture_first_time:
            self._get_existent_event_files()

        if self.replay_dir or self.resume_dir:
            self.use_existent_event_file_flag = self._use_existent_event_file()

        if ((not self.replay_dir and not self.resume_dir) or
                (self.resume_dir and not self.use_existent_event_file_flag)):
            self.gesture_file_name = os.path.join(self.output.log_dir, basename)
            self.saved_msg = '(saved: %s)\n' % self.gesture_file_name
            self.deleted_msg = '(deleted: %s)\n' % self.gesture_file_name
        else:
            self.saved_msg = None
            self.deleted_msg = None
        self.new_scores = None

        if self.robot.is_robot_action_mode() or self.robot.is_manual_noise_test_mode():
            self.robot.turn_off_noise()

        (msg, color_msg, glog) = self._create_prompt(self.gesture,
                                                     self.variation)
        self.win.set_gesture_name(msg)
        self.output.report_html.insert_gesture_log(glog)
        print color_msg
        self.output.print_report(color_msg)
        return True

    def _setup_this_gesture_variation(self):
        """Set up the recording process or use an existent event data file."""
        if self.replay_dir:
            self.record_new_file = False
            self.win.register_timeout_add(self.gesture_timeout_callback, 0)
            return

        if self.resume_dir and self.use_existent_event_file_flag:
            self.record_new_file = False
            self._handle_user_choice_validate_before_parsing()
            self._handle_keyboard_event(TFK.SAVE)
            return

        # Initiate the MtbSanityValidator. Note that this should be done each
        # time just before recording the gesture file since it requires a
        # snapshot of the input device before any finger touching the device.
        self.gesture.mtb_sanity_validator = validators.MtbSanityValidator()

        # Now, we will record a new gesture event file.
        # Fork a new process for mtplot. Add io watch for the gesture file.
        self.record_new_file = True
        self.gesture_file = open(self.gesture_file_name, 'w')
        self.record_proc = subprocess.Popen(self.record_cmd.split(),
                                            stdout=self.gesture_file)

        # Watch if data come in to the monitored file.
        self.gesture_begins_flag = False
        self._reopen_system_device()
        self.gesture_file_watch_tag = self.win.register_io_add_watch(
                self.gesture_file_watch_callback, self.system_device,
                self.evdev_device)
