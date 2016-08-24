# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import tempfile
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class Device(object):
    """Information about a specific input device."""
    def __init__(self, input_type):
        self.input_type = input_type  # e.g. 'touchpad'
        self.emulated = False  # Whether device is real or not
        self.emulation_process = None  # Process of running emulation
        self.name = 'unknown'  # e.g. 'Atmel maXTouch Touchpad'
        self.fw_id = None  # e.g. '6.0'
        self.hw_id = None  # e.g. '90.0'
        self.node = None  # e.g. '/dev/input/event4'
        self.device_dir = None  # e.g. '/sys/class/input/event4/device/device'

    def __str__(self):
        s = '%s:' % self.input_type
        s += '\n  Name: %s' % self.name
        s += '\n  Node: %s' % self.node
        s += '\n  hw_id: %s' % self.hw_id
        s += '\n  fw_id: %s' % self.fw_id
        s += '\n  Emulated: %s' % self.emulated
        return s


class InputPlayback(object):
    """
    Provides an interface for playback and emulating peripherals via evemu-*.

    Example use: player = InputPlayback()
                 player.emulate(property_file=path_to_file)
                 player.find_connected_inputs()
                 player.playback(path_to_file)
                 player.blocking_playback(path_to_file)
                 player.close()

    """

    _DEFAULT_PROPERTY_FILES = {'mouse': 'mouse.prop',
                               'keyboard': 'keyboard.prop'}
    _PLAYBACK_COMMAND = 'evemu-play --insert-slot0 %s < %s'

    # Define a keyboard as anything with any keys #2 to #248 inclusive,
    # as defined in the linux input header.  This definition includes things
    # like the power button, so reserve the "keyboard" label for things with
    # letters/numbers and define the rest as "other_keyboard".
    _MINIMAL_KEYBOARD_KEYS = ['1', 'Q', 'SPACE']
    _KEYBOARD_KEYS = [
            '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'MINUS', 'EQUAL',
            'BACKSPACE', 'TAB', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
            'P', 'LEFTBRACE', 'RIGHTBRACE', 'ENTER', 'LEFTCTRL', 'A', 'S', 'D',
            'F', 'G', 'H', 'J', 'K', 'L', 'SEMICOLON', 'APOSTROPHE', 'GRAVE',
            'LEFTSHIFT', 'BACKSLASH', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
            'COMMA', 'DOT', 'SLASH', 'RIGHTSHIFT', 'KPASTERISK', 'LEFTALT',
            'SPACE', 'CAPSLOCK', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8',
            'F9', 'F10', 'NUMLOCK', 'SCROLLLOCK', 'KP7', 'KP8', 'KP9',
            'KPMINUS', 'KP4', 'KP5', 'KP6', 'KPPLUS', 'KP1', 'KP2', 'KP3',
            'KP0', 'KPDOT', 'ZENKAKUHANKAKU', '102ND', 'F11', 'F12', 'RO',
            'KATAKANA', 'HIRAGANA', 'HENKAN', 'KATAKANAHIRAGANA', 'MUHENKAN',
            'KPJPCOMMA', 'KPENTER', 'RIGHTCTRL', 'KPSLASH', 'SYSRQ', 'RIGHTALT',
            'LINEFEED', 'HOME', 'UP', 'PAGEUP', 'LEFT', 'RIGHT', 'END', 'DOWN',
            'PAGEDOWN', 'INSERT', 'DELETE', 'MACRO', 'MUTE', 'VOLUMEDOWN',
            'VOLUMEUP', 'POWER', 'KPEQUAL', 'KPPLUSMINUS', 'PAUSE', 'SCALE',
            'KPCOMMA', 'HANGEUL', 'HANGUEL', 'HANJA', 'YEN', 'LEFTMETA',
            'RIGHTMETA', 'COMPOSE', 'STOP', 'AGAIN', 'PROPS', 'UNDO', 'FRONT',
            'COPY', 'OPEN', 'PASTE', 'FIND', 'CUT', 'HELP', 'MENU', 'CALC',
            'SETUP', 'WAKEUP', 'FILE', 'SENDFILE', 'DELETEFILE', 'XFER',
            'PROG1', 'PROG2', 'WWW', 'MSDOS', 'COFFEE', 'SCREENLOCK',
            'DIRECTION', 'CYCLEWINDOWS', 'MAIL', 'BOOKMARKS', 'COMPUTER',
            'BACK', 'FORWARD', 'CLOSECD', 'EJECTCD', 'EJECTCLOSECD', 'NEXTSONG',
            'PLAYPAUSE', 'PREVIOUSSONG', 'STOPCD', 'RECORD', 'REWIND', 'PHONE',
            'ISO', 'CONFIG', 'HOMEPAGE', 'REFRESH', 'EXIT', 'MOVE', 'EDIT',
            'SCROLLUP', 'SCROLLDOWN', 'KPLEFTPAREN', 'KPRIGHTPAREN', 'NEW',
            'REDO', 'F13', 'F14', 'F15', 'F16', 'F17', 'F18', 'F19', 'F20',
            'F21', 'F22', 'F23', 'F24', 'PLAYCD', 'PAUSECD', 'PROG3', 'PROG4',
            'DASHBOARD', 'SUSPEND', 'CLOSE', 'PLAY', 'FASTFORWARD', 'BASSBOOST',
            'PRINT', 'HP', 'CAMERA', 'SOUND', 'QUESTION', 'EMAIL', 'CHAT',
            'SEARCH', 'CONNECT', 'FINANCE', 'SPORT', 'SHOP', 'ALTERASE',
            'CANCEL', 'BRIGHTNESSDOWN', 'BRIGHTNESSUP', 'MEDIA',
            'SWITCHVIDEOMODE', 'KBDILLUMTOGGLE', 'KBDILLUMDOWN', 'KBDILLUMUP',
            'SEND', 'REPLY', 'FORWARDMAIL', 'SAVE', 'DOCUMENTS', 'BATTERY',
            'BLUETOOTH', 'WLAN', 'UWB', 'UNKNOWN', 'VIDEO_NEXT', 'VIDEO_PREV',
            'BRIGHTNESS_CYCLE', 'BRIGHTNESS_AUTO', 'BRIGHTNESS_ZERO',
            'DISPLAY_OFF', 'WWAN', 'WIMAX', 'RFKILL', 'MICMUTE']


    def __init__(self):
        self.devices = {}
        self._emulated_device = None


    def has(self, input_type):
        """Return True/False if device has a input of given type.

        @param input_type: string of type, e.g. 'touchpad'

        """
        return input_type in self.devices


    def _get_input_events(self):
        """Return a list of all input event nodes."""
        return utils.run('ls /dev/input/event*').stdout.strip().split()


    def emulate(self, input_type='mouse', property_file=None):
        """
        Emulate the given input (or default for type) with evemu-device.

        Emulating more than one of the same device type will only allow playback
        on the last one emulated.  The name of the last-emulated device is
        noted to be sure this is the case.

        Property files are made with the evemu-describe command,
        e.g. 'evemu-describe /dev/input/event12 > property_file'.

        @param input_type: 'mouse' or 'keyboard' to use default property files.
                           Need not be specified if supplying own file.
        @param property_file: Property file of device to be emulated.  Generate
                              with 'evemu-describe' command on test image.

        """
        new_device = Device(input_type)
        new_device.emulated = True

        # Checks for any previous emulated device and kills the process
        self.close()

        if not property_file:
            if input_type not in self._DEFAULT_PROPERTY_FILES:
                raise error.TestError('Please supply a property file for input '
                                      'type %s' % input_type)
            current_dir = os.path.dirname(os.path.realpath(__file__))
            property_file = os.path.join(
                    current_dir, self._DEFAULT_PROPERTY_FILES[input_type])
        if not os.path.isfile(property_file):
            raise error.TestError('Property file %s not found!' % property_file)

        logging.info('Emulating %s %s', input_type, property_file)
        num_events_before = len(self._get_input_events())
        new_device.emulation_process = subprocess.Popen(
                ['evemu-device', property_file], stdout=subprocess.PIPE)
        utils.poll_for_condition(
                lambda: len(self._get_input_events()) > num_events_before,
                exception=error.TestError('Error emulating %s!' % input_type))

        with open(property_file) as fh:
            name_line = fh.readline()  # Format "N: NAMEOFDEVICE"
            new_device.name = name_line[3:-1]

        self._emulated_device = new_device


    def _find_device_properties(self, device):
        """Return string of properties for given node.

        @return: string of properties.

        """
        with tempfile.NamedTemporaryFile() as temp_file:
            filename = temp_file.name
            evtest_process = subprocess.Popen(['evtest', device],
                                              stdout=temp_file)

            def find_exit():
                """Polling function for end of output."""
                interrupt_cmd = 'grep "interrupt to exit" %s | wc -l' % filename
                line_count = utils.run(interrupt_cmd).stdout.strip()
                return line_count != '0'

            utils.poll_for_condition(find_exit)
            evtest_process.kill()
            temp_file.seek(0)
            props = temp_file.read()
        return props


    def _determine_input_type(self, props):
        """Find input type (if any) from a string of properties.

        @return: string of type, or None

        """
        if props.find('REL_X') >= 0 and props.find('REL_Y') >= 0:
            if (props.find('ABS_MT_POSITION_X') >= 0 and
                props.find('ABS_MT_POSITION_Y') >= 0):
                return 'multitouch_mouse'
            else:
                return 'mouse'
        if props.find('ABS_X') >= 0 and props.find('ABS_Y') >= 0:
            if (props.find('BTN_STYLUS') >= 0 or
                props.find('BTN_STYLUS2') >= 0 or
                props.find('BTN_TOOL_PEN') >= 0):
                return 'tablet'
            if (props.find('ABS_PRESSURE') >= 0 or
                props.find('BTN_TOUCH') >= 0):
                if (props.find('BTN_LEFT') >= 0 or
                    props.find('BTN_MIDDLE') >= 0 or
                    props.find('BTN_RIGHT') >= 0 or
                    props.find('BTN_TOOL_FINGER') >= 0):
                    return 'touchpad'
                else:
                    return 'touchscreen'
            if props.find('BTN_LEFT') >= 0:
                return 'touchscreen'
        if props.find('KEY_') >= 0:
            for key in self._MINIMAL_KEYBOARD_KEYS:
                if props.find('KEY_%s' % key) >= 0:
                    return 'keyboard'
            for key in self._KEYBOARD_KEYS:
                if props.find('KEY_%s' % key) >= 0:
                    return 'other_keyboard'
        return


    def _get_contents_of_file(self, filepath):
        """Return the contents of the given file.

        @param filepath: string of path to file

        @returns: contents of file.  Assumes file exists.

        """
        return utils.run('cat %s' % filepath).stdout.strip()


    def _find_device_ids(self, device_dir, input_type):
        """Find the fw_id and hw_id for the given device directory.

        Finding fw_id and hw_id applicable only for touchpads and touchscreens.

        @param device_dir: the device directory.
        @param input_type: string of input type.

        @returns: firmware id, hardware id

        """
        fw_id, hw_id = None, None

        if not device_dir or input_type not in ['touchpad', 'touchscreen']:
            return fw_id, hw_id

        # Touch devices with custom drivers save this info as a file.
        fw_filenames = ['fw_version', 'firmware_version', 'firmware_id']
        for fw_filename in fw_filenames:
            fw_path = os.path.join(device_dir, fw_filename)
            if os.path.exists(fw_path):
                fw_id = self._get_contents_of_file(fw_path)
                break

        hw_filenames = ['hw_version', 'product_id', 'board_id']
        for hw_filename in hw_filenames:
            hw_path = os.path.join(device_dir, hw_filename)
            if os.path.exists(hw_path):
                hw_id = self._get_contents_of_file(hw_path)
                break

        # Hw_ids for Weida and 2nd gen Synaptics are different.
        if not hw_id:
            id_folder = os.path.abspath(os.path.join(device_dir, '..', 'id'))
            product_path = os.path.join(id_folder, 'product')
            vendor_path = os.path.join(id_folder, 'vendor')

            if os.path.isfile(product_path):
                product = self._get_contents_of_file(product_path)
                if input_type == 'touchscreen':
                    if os.path.isfile(vendor_path):
                        vendor = self._get_contents_of_file(vendor_path)
                        hw_id = vendor + product
                else:
                    hw_id = product

        # Fw_ids for 2nd gen Synaptics can only be found via rmi4update.
        # See if any /dev/hidraw* link to this device's input event.
        if not fw_id:
            input_name_path = os.path.join(device_dir, 'input')
            input_name = utils.run('ls %s' % input_name_path,
                                   ignore_status=True).stdout.strip()
            hidraws = utils.run('ls /dev/hidraw*').stdout.strip().split()
            for hidraw in hidraws:
                class_folder = hidraw.replace('dev', 'sys/class/hidraw')
                input_folder_path = os.path.join(class_folder, 'device',
                                                 'input', input_name)
                if os.path.exists(input_folder_path):
                    fw_id = utils.run('rmi4update -p -d %s' % hidraw,
                                      ignore_status=True).stdout.strip()
                    if fw_id == '':
                        fw_id = None

        return fw_id, hw_id


    def find_connected_inputs(self):
        """Determine the nodes of all present input devices, if any.

        Cycle through all possible /dev/input/event* and find which ones
        are touchpads, touchscreens, mice, keyboards, etc.
        These nodes can be used for playback later.
        If the type of input is already emulated, prefer that device. Otherwise,
        prefer the last node found of that type (e.g. for multiple touchpads).
        Record the found devices in self.devices.

        """
        self.devices = {}  # Discard any previously seen nodes.

        input_events = self._get_input_events()
        for event in input_events:
            properties = self._find_device_properties(event)
            input_type = self._determine_input_type(properties)
            if input_type:
                new_device = Device(input_type)
                new_device.node = event

                class_folder = event.replace('dev', 'sys/class')
                name_file = os.path.join(class_folder, 'device', 'name')
                if os.path.isfile(name_file):
                    name = self._get_contents_of_file(name_file)
                logging.info('Found %s: %s at %s.', input_type, name, event)

                # If a particular device is expected, make sure name matches.
                if (self._emulated_device and
                    self._emulated_device.input_type == input_type):
                    if self._emulated_device.name != name:
                        continue
                    else:
                        new_device.emulated = True
                        process = self._emulated_device.emulation_process
                        new_device.emulation_process = process
                new_device.name = name

                # Find the devices folder containing power info
                # e.g. /sys/class/event4/device/device
                # Search that folder for hwid and fwid
                device_dir = os.path.join(class_folder, 'device', 'device')
                if os.path.exists(device_dir):
                    new_device.device_dir = device_dir
                    fw_id, hw_id = self._find_device_ids(device_dir, input_type)
                    new_device.fw_id, new_device.hw_id = fw_id, hw_id

                if new_device.emulated:
                    self._emulated_device = new_device

                self.devices[input_type] = new_device
                logging.debug(self.devices[input_type])


    def playback(self, filepath, input_type='touchpad'):
        """Playback a given input file.

        Create input file using evemu-record.
        E.g. 'evemu-record $NODE -1 > $FILENAME'

        @param filepath: path to the input file on the DUT.
        @param input_type: name of device type; 'touchpad' by default.
                           Types are returned by the _determine_input_type()
                           function.
                           input_type must be known. Check using has().

        """
        assert(input_type in self.devices)
        node = self.devices[input_type].node
        logging.info('Playing back finger-movement on %s, file=%s.', node,
                     filepath)
        utils.run(self._PLAYBACK_COMMAND % (node, filepath))


    def blocking_playback(self, filepath, input_type='touchpad'):
        """Playback a given set of inputs and sleep for duration.

        The input file is of the format <name>\nE: <time> <input>\nE: ...
        Find the total time by the difference between the first and last input.

        @param filepath: path to the input file on the DUT.
        @param input_type: name of device type; 'touchpad' by default.
                           Types are returned by the _determine_input_type()
                           function.
                           input_type must be known. Check using has().

        """
        with open(filepath) as fh:
            lines = fh.readlines()
            start = float(lines[0].split(' ')[1])
            end = float(lines[-1].split(' ')[1])
            sleep_time = end - start
        self.playback(filepath, input_type)
        logging.info('Sleeping for %s seconds during playback.', sleep_time)
        time.sleep(sleep_time)


    def blocking_playback_of_default_file(self, filename, input_type='mouse'):
        """Playback a default file and sleep for duration.

        Use a default gesture file for the default keyboard/mouse, saved in
        this folder.
        Device should be emulated first.

        @param filename: the name of the file (path is to this folder).
        @param input_type: name of device type; 'mouse' by default.
                           Types are returned by the _determine_input_type()
                           function.
                           input_type must be known. Check using has().

        """
        current_dir = os.path.dirname(os.path.realpath(__file__))
        gesture_file = os.path.join(current_dir, filename)
        self.blocking_playback(gesture_file, input_type=input_type)


    def close(self):
        """Kill emulation if necessary."""
        if self._emulated_device:
            self._emulated_device.emulation_process.kill()


    def __exit__(self):
        self.close()
