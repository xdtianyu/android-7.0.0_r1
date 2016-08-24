# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json, logging, os, pwd, shutil, subprocess, time

import dbus

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import semiauto_framework
from autotest_lib.client.cros import sys_power

_USER_TIMEOUT_TIME = 321  # Seconds a tester has to respond to prompts
_DEVICE_TIMEOUT_TIME = 321  # Seconds a tester has to pair or connect device
_ADAPTER_INTERFACE = 'org.bluez.Adapter1' # Name of adapter in DBus interface
_DEVICE_INTERFACE = 'org.bluez.Device1' # Name of a device in DBus interface
_TIME_FORMAT = '%d %b %Y %H:%M:%S' # Human-readable time format for logs
_SECTION_BREAK = '='*75


class BluetoothSemiAutoHelper(semiauto_framework.semiauto_test):
    """Generic Bluetooth SemiAutoTest.

    Contains functions needed to implement an actual Bluetooth SemiAutoTest,
    such as accessing the state of Bluetooth adapter/devices via dbus,
    opening dialogs with tester via Telemetry browser, and getting log data.
    """
    version = 1

    # Boards without Bluetooth support.
    _INVALID_BOARDS = ['x86-alex', 'x86-alex_he', 'lumpy']

    def _err(self, message):
        """Raise error after first collecting more information.

        @param message: error message to raise and add to logs.

        """
        self.collect_logs('ERROR HAS OCCURED: %s' % message)
        raise error.TestError(message)

    def supports_bluetooth(self):
        """Return True if this device has Bluetooth capabilities; else False."""
        device = utils.get_board()
        if device in self._INVALID_BOARDS:
            logging.info('%s does not have Bluetooth.', device)
            return False
        return True

    def _get_objects(self):
        """Return the managed objects for this chromebook."""
        manager = dbus.Interface(
                self._bus.get_object('org.bluez', '/'),
                dbus_interface='org.freedesktop.DBus.ObjectManager')
        return manager.GetManagedObjects()

    def _get_adapter_info(self):
        """Return the adapter interface objects, or None if not found."""
        objects = self._get_objects()
        for path, interfaces in objects.items():
            if _ADAPTER_INTERFACE in interfaces:
                self._adapter_path = path
                return interfaces[_ADAPTER_INTERFACE]
        return None

    def _get_device_info(self, addr):
        """Return the device interface objects, or None if not found."""
        objects = self._get_objects()
        for _, interfaces in objects.items():
            if _DEVICE_INTERFACE in interfaces:
                if interfaces[_DEVICE_INTERFACE]['Address'] == addr:
                    return interfaces[_DEVICE_INTERFACE]
        return None

    def _verify_adapter_power(self, adapter_power_status):
        """Return True/False if adapter power status matches given value."""
        info = self._get_adapter_info()
        if not info:
            self._err('No adapter found!')
        return True if info['Powered'] == adapter_power_status else False

    def _verify_device_connection(self, addr, paired_status=True,
                                  connected_status=True):
        """Return True/False if device statuses match given values."""
        def _check_info():
            info = self._get_device_info(addr)
            if info:
                if (info['Paired'] != paired_status or
                    info['Connected'] != connected_status):
                    return False
                return True
            # Return True if no entry was found for an unpaired device
            return not paired_status and not connected_status

        results = _check_info()

        # To avoid spotting brief connections, sleep and check again.
        if results:
            time.sleep(0.5)
            results = _check_info()
        return results

    def set_adapter_power(self, adapter_power_status):
        """Set adapter power status to match given value via dbus call.

        Block until the power is set.

        @param adapter_power_status: True to turn adapter on; False for off.

        """
        info = self._get_adapter_info()
        if not info:
            self._err('No adapter found!')
        properties = dbus.Interface(
                self._bus.get_object('org.bluez', self._adapter_path),
                dbus_interface='org.freedesktop.DBus.Properties')
        properties.Set(_ADAPTER_INTERFACE, 'Powered', adapter_power_status)

        self.poll_adapter_power(adapter_power_status)

    def poll_adapter_presence(self):
        """Raise error if adapter is not found after some time."""
        complete = lambda: self._get_adapter_info() is not None
        try:
            utils.poll_for_condition(
                    condition=complete, timeout=15, sleep_interval=1)
        except utils.TimeoutError:
            self._err('No adapter found after polling!')

    def poll_adapter_power(self, adapter_power_status=True):
        """Wait until adapter power status matches given value.

        @param adapter_power_status: True for adapter is on; False for off.

        """
        complete = lambda: self._verify_adapter_power(
                adapter_power_status=adapter_power_status)
        adapter_str = 'ON' if adapter_power_status else 'OFF'
        utils.poll_for_condition(
                condition=complete, timeout=_DEVICE_TIMEOUT_TIME,
                sleep_interval=1,
                desc=('Timeout for Bluetooth Adapter to be %s' % adapter_str))

    def _poll_connection(self, addr, paired_status, connected_status):
        """Wait until device statuses match given values."""
        paired_str = 'PAIRED' if paired_status else 'NOT PAIRED'
        conn_str = 'CONNECTED' if connected_status else 'NOT CONNECTED'
        message = 'Waiting for device %s to be %s and %s' % (addr, paired_str,
                                                             conn_str)
        logging.info(message)

        complete = lambda: self._verify_device_connection(
                addr, paired_status=paired_status,
                connected_status=connected_status)
        utils.poll_for_condition(
                condition=complete, timeout=_DEVICE_TIMEOUT_TIME,
                sleep_interval=1, desc=('Timeout while %s' % message))

    def poll_connections(self, paired_status=True, connected_status=True):
        """Wait until all Bluetooth devices have the given statues.

        @param paired_status: True for device paired; False for unpaired.
        @param connected_status: True for device connected; False for not.

        """
        for addr in self._addrs:
            self._poll_connection(addr, paired_status=paired_status,
                                  connected_status=connected_status)

    def login_and_open_browser(self):
        """Log in to machine, open browser, and navigate to dialog template.

        Assumes the existence of 'client/cros/audio/music.mp3' file, and will
        fail if not found.

        """
        # Open browser and interactive tab
        self.login_and_open_interactive_tab()

        # Find mounted home directory
        user_home = None
        for udir in os.listdir(os.path.join('/', 'home', 'user')):
            d = os.path.join('/', 'home', 'user', udir)
            if os.path.ismount(d):
                user_home = d
        if user_home is None:
            raise error.TestError('Could not find mounted home directory')

        # Setup Audio File
        audio_dir = os.path.join(self.bindir, '..', '..', 'cros', 'audio')
        loop_file = os.path.join(audio_dir, 'loop.html')
        music_file = os.path.join(audio_dir, 'music.mp3')
        dl_dir = os.path.join(user_home, 'Downloads')
        self._added_loop_file = os.path.join(dl_dir, 'loop.html')
        self._added_music_file = os.path.join(dl_dir, 'music.mp3')
        shutil.copyfile(loop_file, self._added_loop_file)
        shutil.copyfile(music_file, self._added_music_file)
        uid = pwd.getpwnam('chronos').pw_uid
        gid = pwd.getpwnam('chronos').pw_gid
        os.chmod(self._added_loop_file, 0755)
        os.chmod(self._added_music_file, 0755)
        os.chown(self._added_loop_file, uid, gid)
        os.chown(self._added_music_file, uid, gid)

        # Open Test Dialog tab, Settings tab, and Audio file
        self._settings_tab = self._browser.tabs.New()
        self._settings_tab.Navigate('chrome://settings/search#Bluetooth')
        music_tab = self._browser.tabs.New()
        music_tab.Navigate('file:///home/chronos/user/Downloads/loop.html')

    def ask_user(self, message):
        """Ask the user a yes or no question in an open tab.

        Reset dialog page to be a question (message param) with 'PASS' and
        'FAIL' buttons.  Wait for answer.  If no, ask for more information.

        @param message: string sent to the user via browswer interaction.

        """
        logging.info('Asking user "%s"', message)
        sandbox = 'SANDBOX:<input type="text"/>'
        html = '<h3>%s</h3>%s' % (message, sandbox)
        self.set_tab_with_buttons(html, buttons=['PASS', 'FAIL'])

        # Intepret results.
        result = self.wait_for_tab_result(timeout=_USER_TIMEOUT_TIME)
        if result == 1:
            # Ask for more information on error.
            html='<h3>Please provide more info:</h3>'
            self.set_tab_with_textbox(html)

            # Get explanation of error, clear output, and raise error.
            result = self.wait_for_tab_result(timeout=_USER_TIMEOUT_TIME)
            self.clear_output()
            self._err('Testing %s. "%s".' % (self._test_type, result))
        elif result != 0:
            raise error.TestError('Bad dialog value: %s' % result)
        logging.info('Answer was PASS')

        # Clear user screen.
        self.clear_output()

    def tell_user(self, message):
        """Tell the user the given message in an open tab.

        @param message: the text string to be displayed.

        """
        logging.info('Telling user "%s"', message)
        html = '<h3>%s</h3>' % message
        self.set_tab(html)

    def check_working(self, message=None):
        """Steps to check that all devices are functioning.

        Ask user to connect all devices, verify connections, and ask for
        user input if they are working.

        @param message: string of text the user is asked.  Defaults to asking
                        the user to connect all devices.

        """
        if not message:
            message = ('Please connect all devices.<br>(You may need to '
                       'click mice, press keyboard keys, or use the '
                       'Connect button in Settings.)')
        self.tell_user(message)
        self.poll_adapter_power(True)
        self.poll_connections(paired_status=True, connected_status=True)
        self.ask_user('Are all Bluetooth devices working?<br>'
                       'Is audio playing only through Bluetooth devices?<br>'
                       'Do onboard keyboard and trackpad work?')

    def ask_not_working(self):
        """Ask the user pre-defined message about NOT working."""
        self.ask_user('No Bluetooth devices work.<br>Audio is NOT playing '
                      'through onboard speakers or wired headphones.')

    def start_dump(self, message=''):
        """Run btmon in subprocess.

        Kill previous btmon (if needed) and start new one using current
        test type as base filename.  Dumps stored in results folder.

        @param message: string of text added to top of log entry.

        """
        if hasattr(self, '_dump') and self._dump:
            self._dump.kill()
        if not hasattr(self, '_test_type'):
            self._test_type = 'test'
        logging.info('Starting btmon')
        filename = '%s_btmon' % self._test_type
        path = os.path.join(self.resultsdir, filename)
        with open(path, 'a') as f:
            f.write('%s\n' % _SECTION_BREAK)
            f.write('%s: Starting btmon\n' % time.strftime(_TIME_FORMAT))
            f.write('%s\n' % message)
            f.flush()
            btmon_path = '/usr/bin/btmon'
            try:
                self._dump = subprocess.Popen([btmon_path], stdout=f,
                                              stderr=subprocess.PIPE)
            except Exception as e:
                raise error.TestError('btmon: %s' % e)

    def collect_logs(self, message=''):
        """Store results of dbus GetManagedObjects and hciconfig.

        Use current test type as base filename.  Stored in results folder.

        @param message: string of text added to top of log entry.

        """
        logging.info('Collecting dbus info')
        if not hasattr(self, '_test_type'):
            self._test_type = 'test'
        filename = '%s_dbus' % self._test_type
        path = os.path.join(self.resultsdir, filename)
        with open(path, 'a') as f:
            f.write('%s\n' % _SECTION_BREAK)
            f.write('%s: %s\n' % (time.strftime(_TIME_FORMAT), message))
            f.write(json.dumps(self._get_objects().items(), indent=2))
            f.write('\n')

        logging.info('Collecting hciconfig info')
        filename = '%s_hciconfig' % self._test_type
        path = os.path.join(self.resultsdir, filename)
        with open(path, 'a') as f:
            f.write('%s\n' % _SECTION_BREAK)
            f.write('%s: %s\n' % (time.strftime(_TIME_FORMAT), message))
            f.flush()
            hciconfig_path = '/usr/bin/hciconfig'
            try:
                subprocess.check_call([hciconfig_path, '-a'], stdout=f)
            except Exception as e:
                raise error.TestError('hciconfig: %s' % e)

    def os_idle_time_set(self, reset=False):
        """Function to set short idle time or to reset to normal.

        Not using sys_power so that user can use Bluetooth to wake machine.

        @param reset: true to reset to normal idle time, false for short.

        """
        powerd_path = '/usr/bin/set_short_powerd_timeouts'
        flag = '--reset' if reset else ''
        try:
            subprocess.check_call([powerd_path, flag])
        except Exception as e:
            raise error.TestError('idle cmd: %s' % e)

    def os_suspend(self):
        """Function to suspend ChromeOS using sys_power."""
        sys_power.do_suspend(5)

        # Sleep
        time.sleep(5)

    def initialize(self):
        self._bus = dbus.SystemBus()

    def warmup(self, addrs='', test_phase='client', close_browser=True):
        """Warmup setting paramters for semi-automated Bluetooth Test.

        Actual test steps are implemened in run_once() function.

        @param: addrs: list of MAC address of Bluetooth devices under test.
        @param: test_phase: for use by server side tests to, for example, call
                            the same test before and after a reboot.
        @param: close_browser: True if client side test should close browser
                               at end of test.

        """
        self.login_and_open_browser()

        self._addrs = addrs
        self._test_type = 'start'
        self._test_phase = test_phase
        self._will_close_browser = close_browser

    def cleanup(self):
        """Cleanup of various files/processes opened during test.

        Closes running btmon, closes browser (if asked to at start), and
        deletes files added during test.

        """
        if hasattr(self, '_dump'):
            self._dump.kill()
        if hasattr(self, '_will_close_browser') and self._will_close_browser:
            self.close_browser()
        if (hasattr(self, '_added_loop_file')
                and os.path.exists(self._added_loop_file)):
            os.remove(self._added_loop_file)
        if (hasattr(self, '_added_music_file')
                and os.path.exists(self._added_music_file)):
            os.remove(self._added_music_file)
