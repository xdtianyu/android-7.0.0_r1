# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Some utility classes and functions."""

import glob
import os
import re
import sys
import time

import common_util
import test_conf as conf


def get_display_name():
    """Return the display name."""
    return ':0'


def get_screen_size():
    """Get the screen size using xwininfo."""
    cmd = 'DISPLAY=:0 xwininfo -root'
    wininfo = common_util.simple_system_output(cmd)
    # the geometry string looks like:
    #     "  -geometry 3840x1200+0+0"
    geometry_pattern = re.compile('\s*-geometry\s*(\d+)x(\d+)+.*', re.I)
    for line in wininfo.splitlines():
        result = geometry_pattern.search(line)
        if result:
            width = int(result.group(1))
            height = int(result.group(2))
            return (width, height)
    return None


def get_current_time_str():
    """Get the string of current time."""
    time_format = '%Y%m%d_%H%M%S'
    return time.strftime(time_format, time.gmtime())


def get_board():
    """Get board of the Chromebook machine."""
    with open('/etc/lsb-release') as lsb:
        context = lsb.read()
    board = None
    if context is not None:
        for line in context.splitlines():
            if line.startswith('CHROMEOS_RELEASE_BOARD'):
                board_str = line.split('=')[1]
                if '-' in board_str:
                    board = board_str.split('-')[1]
                elif '_' in board_str:
                    board = board_str.split('_')[1]
                else:
                    board = board_str
                # Some boards, e.g. alex, may have board name as alex32
                board = re.search('(\D+)\d*', board, re.I).group(1)
                break
    return board


def get_board_from_filename(filename):
    """Get the board name from a given string which is usually a log file.

    A log file looks like:
        touch_firmware_report-lumpy-fw_11.27-complete-20130610_150540.log

    @param filename: the filename used to extract the board
    """
    pieces = filename.split('-')
    return pieces[1] if len(pieces) >= 2 else None

def get_board_from_directory(directory):
    """Get the board name from the log in the replay directory.

    @param directory: the log directory
    """
    log_files = glob.glob(os.path.join(conf.log_root_dir, directory, '*.log'))
    for log_file in log_files:
        board = get_board_from_filename(os.path.basename(log_file))
        if board:
            return board
    return None


def get_cpu():
    """Get the processor of the machine."""
    return common_util.simple_system_output('uname -m')


def install_pygtk():
    """A temporary dirty hack of installing pygtk related packages."""
    pygtk_dict = {'x86_64': ['pygtk_x86_64.tbz2', 'lib64'],
                  'i686': ['pygtk_x86_32.tbz2', 'lib'],
                  'armv7l': ['pygtk_armv7l.tbz2', 'lib'],
    }
    pygtk_info = pygtk_dict.get(get_cpu().lower())

    if get_board() is None:
        print 'This does not look like a chromebook.'
    elif pygtk_info:
        cmd_remount = 'mount -o remount,rw /'
        if common_util.simple_system(cmd_remount) == 0:
            pygtk_tarball, lib_path = pygtk_info
            cmd_untar = 'tar -jxf pygtk/%s -C /' % pygtk_tarball
            if common_util.simple_system(cmd_untar) == 0:
                # Need to add the gtk path manually. Otherwise, the path
                # may not be in the sys.path for the first time when the
                # tarball is extracted.
                gtk_path = ('/usr/local/%s/python2.7/site-packages/gtk-2.0' %
                            lib_path)
                sys.path.append(gtk_path)
                print 'Successful installation of pygtk.'
                return True
            else:
                print 'Error: Failed to install pygtk.'
        else:
            print 'Failed to remount. Have you removed the write protect?'
    else:
        print 'The pygtk is only supported for %s so far.' % pygtk_dict.keys()
        print 'The other cpus will be supported on demand.'
        print 'The plan is to remove gtk totally and upgrade to Chrome browser.'
    return False


def get_fw_and_date(filename):
    """Get the firmware version and the test date from a log directory
       or a log file.

    An example html filename looks like
        'touch_firmware_report-link-fw_1.0.170-manual-20130426_064849.log'
        return (fw_1.0.170, 20130426_064849)

    An example log directory looks like
        '20130422_020631-fw_1.0.170-manual'
        return (fw_1.0.170, 20130422_020631)
    """
    # The firmware could be fw_1.0.170 or fw_1.0.AA which always comes with
    # 'fw_' as its prefix. The character '-' is used to separate components
    # in the filename.
    result = re.search('-(%s[^-]+?)-' % conf.fw_prefix, filename)
    fw = result.group(1) if result else None

    result = re.search('(\d{8}_\d{6})[-.]', filename)
    date = result.group(1) if result else None

    return (fw, date)


def create_log_dir(firmware_version, mode):
    """Create a directory to save the report and device event files."""
    dir_basename = conf.filename.sep.join([get_current_time_str(),
                                           conf.fw_prefix + firmware_version,
                                           mode])
    log_root_dir = conf.log_root_dir
    log_dir = os.path.join(log_root_dir, dir_basename)
    latest_symlink = os.path.join(log_root_dir, 'latest')

    # Create the log directory.
    try:
        os.makedirs(log_dir)
    except OSError, e:
        print 'Error in create the directory (%s): %s' % (log_dir, e)
        sys.exit(1)

    # Set up the latest symbolic link to the newly created log directory.
    try:
        if os.path.islink(latest_symlink):
            os.remove(latest_symlink)
        os.symlink(log_dir, latest_symlink)
    except OSError, e:
        print 'Error in setup latest symlink (%s): %s' % (latest_symlink, e)
        sys.exit(1)
    return log_dir


def stop_power_management():
    """Stop the power daemon management."""
    ret_d = common_util.simple_system('stop -q powerd')
    if ret_d:
        print 'Error in stopping powerd.'
        print 'The screen may dim during the test.'


def start_power_management():
    """Start the power daemon management."""
    ret_d = common_util.simple_system('start -q powerd')
    if ret_d:
        print 'Error in starting powerd.'
        print 'The screen may not go into suspend mode.'
        print 'If this is a problem, you could reboot the machine.'


class GestureList:
    """A class defines the gesture list."""

    def __init__(self, gesture_names=None):
        self.gesture_names = (gesture_names if gesture_names
                                            else conf.gesture_names_complete)

    def get_gesture_list(self, key=None):
        """Get the list of Gesture objects based on the gesture names."""
        gesture_dict = conf.get_gesture_dict()
        gesture_list = []
        for name in self.gesture_names:
            gesture = gesture_dict.get(name)
            if gesture is None:
                msg = 'Error: the gesture "%s" is not defined in the config.'
                print msg % name
                return []
            gesture_list.append(gesture)
        return sorted(gesture_list, key=key) if key else gesture_list


class Output:
    """A class to handle outputs to the window and to the report."""
    def __init__(self, log_dir, report_name, win, report_html):
        self.log_dir = log_dir
        self.report_name = report_name
        self.report = open(report_name, 'w')
        self.win = win
        self.prefix_space = ' ' * 4
        self.msg = None
        self.report_html = report_html

    def __del__(self):
        self.stop()

    def stop(self):
        """Close the report file and print it on stdout."""
        self.report.close()
        with open(self.report_name) as f:
            for line in f.read().splitlines():
                print line
        report_msg = '\n*** This test report is saved in the file: %s\n'
        print report_msg % self.report_name

    def get_prefix_space(self):
        """Get the prefix space when printing the report."""
        return self.prefix_space

    def print_report_line(self, msg):
        """Print the line with proper indentation."""
        self.report.write(self.prefix_space + str(msg) + os.linesep)

    def print_window(self, msg):
        """Print the message to the result window."""
        if type(msg) is list:
            msg = os.linesep.join(msg)
        self.win.set_result(msg)
        print msg

    def _print_report(self, msg):
        """Print the message to the report."""
        if type(msg) is list:
            for line in msg:
                self.print_report_line(line)
        else:
            self.print_report_line(msg)

    def buffer_report(self, msg):
        """Buffer the message and print it later if not over-written.

        Usage of the method: the validator test result of a gesture may
        be discarded because the user chooses to re-perform the gesture
        again. So it should be able to over-write the message.
        """
        self.msg = msg

    def flush_report(self):
        """Print the buffered message if any."""
        if self.msg:
            self._print_report(self.msg)
            self.msg = None

    def print_report(self, msg):
        """Print the message to the report."""
        # Print any buffered message first.
        self.flush_report()
        # Print this incoming message
        self._print_report(msg)

    def print_all(self, msg):
        """Print the message to both report and to the window."""
        self.print_window(msg)
        self.buffer_report(msg)


class ScreenShot:
    """Handle screen shot."""

    def __init__(self, geometry_str):
        self.geometry_str = geometry_str
        environment_str = 'DISPLAY=:0.0 XAUTHORITY=/home/chronos/.Xauthority '
        dump_util = '/usr/local/bin/import -quality 20'
        self.dump_window_format = ' '.join([environment_str, dump_util,
                                           '-window %s %s.png'])
        self.dump_root_format = ' '.join([environment_str, dump_util,
                                         '-window root -crop %s %s.png'])
        self.get_id_cmd = 'DISPLAY=:0 xwininfo -root -tree'

    def dump_window(self, filename):
        """Dump the screenshot of a window to the specified file name."""
        win_id = self._get_win_id()
        if win_id:
            dump_cmd = self.dump_window_format % (win_id, filename)
            common_util.simple_system(dump_cmd)
        else:
            print 'Warning: cannot get the window id.'

    def dump_root(self, filename):
        """Dump the screenshot of root to the specified file name."""
        dump_cmd = self.dump_root_format % (self.geometry_str, filename)
        common_util.simple_system(dump_cmd)

    def _get_win_id(self):
        """Get the window ID based on the characteristic string."""
        result = common_util.simple_system_output(self.get_id_cmd)
        for line in result.splitlines():
            if self.geometry_str in line:
                return line.split()[0].strip()
        return None
