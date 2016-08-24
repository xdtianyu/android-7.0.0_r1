# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, struct, time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cros_logging

# Kernel 3.8 to 3.14 has cur_delay_info, 3.18+ has frequency_info.
CLOCK_PATHS = ['/sys/kernel/debug/dri/0/i915_frequency_info',
               '/sys/kernel/debug/dri/0/i915_cur_delayinfo']
# Kernel 3.8 has i915_fbc, kernel > 3.8 i915_fbc_status.
FBC_PATHS = ['/sys/kernel/debug/dri/0/i915_fbc',
             '/sys/kernel/debug/dri/0/i915_fbc_status']
GEM_PATHS = ['/sys/kernel/debug/dri/0/i915_gem_active']
PSR_PATHS = ['/sys/kernel/debug/dri/0/i915_edp_psr_status']
RC6_PATHS = ['/sys/kernel/debug/dri/0/i915_drpc_info']


class graphics_Idle(test.test):
    """Class for graphics_Idle.  See 'control' for details."""
    version = 1
    _gpu_type = None
    _cpu_type = None
    _board = None

    def run_once(self):
        # Try to protect against runaway previous tests.
        if not utils.wait_for_idle_cpu(20.0, 0.1):
            logging.warning('Could not get idle CPU before running tests.')
        # We use kiosk mode to make sure Chrome is idle.
        with chrome.Chrome(logged_in=False, extra_browser_args=['--kiosk']):
            self._gpu_type = utils.get_gpu_family()
            self._cpu_type = utils.get_cpu_soc_family()
            self._board = utils.get_board()
            errors = ''
            errors += self.verify_graphics_dvfs()
            errors += self.verify_graphics_fbc()
            errors += self.verify_graphics_psr()
            errors += self.verify_graphics_gem_idle()
            errors += self.verify_graphics_i915_min_clock()
            errors += self.verify_graphics_rc6()
            errors += self.verify_lvds_downclock()
            errors += self.verify_short_blanking()
            if errors:
                raise error.TestFail(errors)

    def get_valid_path(self, paths):
        for path in paths:
            if os.path.exists(path):
                return path
        logging.error('Error: %s not found.', ' '.join(paths))
        return None

    def verify_lvds_downclock(self):
        """On systems which support LVDS downclock, checks the kernel log for
        a message that an LVDS downclock mode has been added."""
        logging.info('Running verify_lvds_downclock')
        board = utils.get_board()
        if not (board == 'alex' or board == 'lumpy' or board == 'stout'):
            return ''

        # Get the downclock message from the logs.
        reader = cros_logging.LogReader()
        reader.set_start_by_reboot(-1)
        if not reader.can_find('Adding LVDS downclock mode'):
            logging.error('Error: LVDS downclock quirk not applied.')
            return 'LVDS downclock quirk not applied. '

        return ''

    def verify_short_blanking(self):
        """On baytrail systems with a known panel, checks the kernel log for a
        message that a short blanking mode has been added."""
        logging.info('Running verify_short_blanking')
        if self._gpu_type != 'baytrail' or utils.has_no_monitor():
            return ''

        # Open the EDID to find the panel model.
        param_path = '/sys/class/drm/card0-eDP-1/edid'
        if not os.path.exists(param_path):
            logging.error('Error: %s not found.', param_path)
            return 'Short blanking not added (no EDID found). '

        with open(param_path, 'r') as edp_edid_file:
            edp_edid_file.seek(8)
            data = edp_edid_file.read(2)
            manufacturer = int(struct.unpack('<H', data)[0])
            data = edp_edid_file.read(2)
            product_code = int(struct.unpack('<H', data)[0])

        # This is not the panel we are looking for (AUO B116XTN02.2)
        if manufacturer != 0xaf06 or product_code != 0x225c:
            return ''

        # Get the downclock message from the logs.
        reader = cros_logging.LogReader()
        reader.set_start_by_reboot(-1)
        if not reader.can_find('Modified preferred into a short blanking mode'):
            logging.error('Error: short blanking not added.')
            return 'Short blanking not added. '

        return ''

    def verify_graphics_rc6(self):
        """ On systems which support RC6 (non atom), check that we are able to
        get into rc6; idle before doing so, and retry every second for 20
        seconds."""
        logging.info('Running verify_graphics_rc6')
        # TODO(ihf): Implement on baytrail/braswell using residency counters.
        # But note the format changed since SNB, so this will be complex.
        if (utils.get_cpu_soc_family() == 'x86_64' and
                self._gpu_type != 'pinetrail' and
                self._gpu_type != 'baytrail' and self._gpu_type != 'braswell'):
            tries = 0
            found = False
            param_path = self.get_valid_path(RC6_PATHS)
            if not param_path:
                return 'RC6_PATHS not found.'
            while found == False and tries < 20:
                time.sleep(1)
                with open(param_path, 'r') as drpc_info_file:
                    for line in drpc_info_file:
                        match = re.search(r'Current RC state: (.*)', line)
                        if match and match.group(1) == 'RC6':
                            found = True
                            break

                tries += 1

            if not found:
                utils.log_process_activity()
                logging.error('Error: did not see the GPU in RC6.')
                return 'Did not see the GPU in RC6. '

        return ''

    def verify_graphics_i915_min_clock(self):
        """ On i915 systems, check that we get into the lowest clock frequency;
        idle before doing so, and retry every second for 20 seconds."""
        logging.info('Running verify_graphics_i915_min_clock')
        if (utils.get_cpu_soc_family() == 'x86_64' and
                self._gpu_type != 'pinetrail'):
            tries = 0
            found = False
            param_path = self.get_valid_path(CLOCK_PATHS)
            if not param_path:
                return 'CLOCK_PATHS not found.'
            while not found and tries < 80:
                time.sleep(0.25)

                with open(param_path, 'r') as delayinfo_file:
                    for line in delayinfo_file:
                        # This file has a different format depending on the
                        # board, so we parse both. Also, it would be tedious
                        # to add the minimum clock for each board, so instead
                        # we use 650MHz which is the max of the minimum clocks.
                        match = re.search(r'CAGF: (.*)MHz', line)
                        if match and int(match.group(1)) <= 650:
                            found = True
                            break

                        match = re.search(r'current GPU freq: (.*) MHz', line)
                        if match and int(match.group(1)) <= 650:
                            found = True
                            break

                tries += 1

            if not found:
                utils.log_process_activity()
                logging.error('Error: did not see the min i915 clock')
                return 'Did not see the min i915 clock. '

        return ''

    def verify_graphics_dvfs(self):
        """ On systems which support DVFS, check that we get into the lowest
        clock frequency; idle before doing so, and retry every second for 20
        seconds."""
        logging.info('Running verify_graphics_dvfs')
        if self._gpu_type == 'mali':
            if self._cpu_type == 'exynos5':
                node = '/sys/devices/11800000.mali/'
                enable_node = 'dvfs'
                enable_value = 'on'
            elif self._cpu_type == 'rockchip':
                node = '/sys/devices/ffa30000.gpu/'
                enable_node = 'dvfs_enable'
                enable_value = '1'
            else:
                logging.error('Error: Unknown CPU type (%s) for mali GPU.',
                              self._cpu_type)
                return 'Unknown CPU type for mali GPU. '

            clock_path = utils.locate_file('clock', node)
            enable_path = utils.locate_file(enable_node, node)
            freqs_path = utils.locate_file('available_frequencies', node)

            enable = utils.read_one_line(enable_path)
            logging.info('DVFS enable = %s', enable)
            if not enable == enable_value:
                logging.error('Error: DVFS is not enabled')
                return 'DVFS is not enabled. '

            # available_frequencies are always sorted in ascending order
            lowest_freq = int(utils.read_one_line(freqs_path))

            # daisy_* (exynos5250) boards set idle frequency to 266000000
            # See: crbug.com/467401 and crosbug.com/p/19710
            if self._board.startswith('daisy'):
                lowest_freq = 266000000

            logging.info('Expecting idle DVFS clock = %u', lowest_freq)

            tries = 0
            found = False
            while not found and tries < 80:
                time.sleep(0.25)
                clock = int(utils.read_one_line(clock_path))
                if clock <= lowest_freq:
                    logging.info('Found idle DVFS clock = %u', clock)
                    found = True
                    break

                tries += 1

            if not found:
                utils.log_process_activity()
                logging.error('Error: DVFS clock (%u) > min (%u)', clock,
                              lowest_freq)
                return 'Did not see the min DVFS clock. '

        return ''

    def verify_graphics_fbc(self):
        """ On systems which support FBC, check that we can get into FBC;
        idle before doing so, and retry every second for 20 seconds."""
        logging.info('Running verify_graphics_fbc')

        # Link's FBC is disabled (crbug.com/338588).
        # TODO(marcheu): remove this when/if we fix this bug.
        board = utils.get_board()
        if board == 'link':
            return ''

        # Machines which don't have a monitor can't get FBC.
        if utils.has_no_monitor():
            return ''

        if (self._gpu_type == 'haswell' or self._gpu_type == 'ivybridge' or
                self._gpu_type == 'sandybridge'):
            tries = 0
            found = False
            param_path = self.get_valid_path(FBC_PATHS)
            if not param_path:
                return 'FBC_PATHS not found.'
            while not found and tries < 20:
                time.sleep(1)
                with open(param_path, 'r') as fbc_info_file:
                    for line in fbc_info_file:
                        if re.search('FBC enabled', line):
                            found = True
                            break

                tries += 1

            if not found:
                logging.error('Error: did not see FBC enabled.')
                return 'Did not see FBC enabled. '

        return ''

    def verify_graphics_psr(self):
        """ On systems which support PSR, check that we can get into PSR;
        idle before doing so, and retry every second for 20 seconds."""
        logging.info('Running verify_graphics_psr')

        board = utils.get_board()
        if board != 'samus':
            return ''
        tries = 0
        found = False
        param_path = self.get_valid_path(PSR_PATHS)
        if not param_path:
            return 'PSR_PATHS not found.'
        while not found and tries < 20:
            time.sleep(1)
            with open(param_path, 'r') as fbc_info_file:
                for line in fbc_info_file:
                    match = re.search(r'Performance_Counter: (.*)', line)
                    if match and int(match.group(1)) > 0:
                        found = True
                        break

            tries += 1

        if not found:
            logging.error('Error: did not see PSR enabled.')
            return 'Did not see PSR enabled. '

        return ''

    def verify_graphics_gem_idle(self):
        """ On systems which have i915, check that we can get all gem objects
        to become idle (i.e. the i915_gem_active list need to go to 0);
        idle before doing so, and retry every second for 20 seconds."""
        logging.info('Running verify_graphics_gem_idle')
        if (utils.get_cpu_soc_family() == 'x86_64' and
                self._gpu_type != 'pinetrail'):
            tries = 0
            found = False
            gem_path = self.get_valid_path(GEM_PATHS)
            if not gem_path:
                return 'GEM_PATHS not found.'
            while not found and tries < 240:
                time.sleep(0.25)
                with open(gem_path, 'r') as gem_file:
                    for line in gem_file:
                        if re.search('Total 0 objects', line):
                            found = True
                            break

                tries += 1

            if not found:
                utils.log_process_activity()
                logging.error('Error: did not reach 0 gem actives.')
                return 'Did not reach 0 gem actives. '

        return ''
