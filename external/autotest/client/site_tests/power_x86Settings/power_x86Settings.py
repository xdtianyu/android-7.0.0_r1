# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import commands, glob, logging, os, re, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_rapl
from autotest_lib.client.cros import power_status
from autotest_lib.client.cros import power_utils
from autotest_lib.client.cros import cros_logging


# Specify registers to check.  The format needs to be:
#   register offset : ('bits', 'expression')
DMI_BAR_CHECKS = {
    'Atom': {
        '0x88':  [('1:0', 3)],
        '0x200': [('27:26', 0)],
        '0x210': [('2:0', 1), ('15:8', 1)],
        '0xc28': [('5:1', 7)],
        '0xc2e': [('5', 1)],
        '0xc30': [('11', 0), ('10:8', 4)],
        '0xc34': [('9:4', 7), ('0', 1)],
        },
    'Non-Atom': {
        # http://www.intel.com/content/dam/doc/datasheet/2nd-gen-core-family-mobile-vol-2-datasheet.pdf
        # PCIE DMI Link Control Register
        # -- [1:0] : ASPM State 0=Disable, 1=L0s, 2=reserved, 3=L0s&L1
        '0x88':  [('1:0', 3)],
        },
    }

# http://www.intel.com/content/dam/www/public/us/en/documents/datasheets/2nd-gen-core-family-mobile-vol-1-datasheet.pdf
# PM_PDWN_Config_
# -- [12]   : Global power-down (GLPDN).  1 == global, 0 == per rank
# -- [11:8] : Power-down mode. 0->0x7.  Higher is lower power
# -- [7:0]  : Power-down idle timer.  Lower is better. Minimum
#             recommended is 0xf
MCH_PM_PDWN_CONFIG = [('12', 0), ('11:8', 0x6, '>='), ('7:0', 0x40, '<='),
                      ('7:0', 0xf, '>=')]
MCH_BAR_CHECKS = {
    'Atom': {},
    'Non-Atom': {
        # mmc0
        '0x40b0': MCH_PM_PDWN_CONFIG,
        # mmc1
        '0x44b0': MCH_PM_PDWN_CONFIG,
        # single mmc
        '0x4cb0': MCH_PM_PDWN_CONFIG,
        },
    }

MSR_CHECKS = {
    'Atom': {
        '0xe2':  [('7', 0), ('2:0', 4)],
        '0x198': [('28:24', 6)],
        '0x1a0': [('33:32', 3), ('26:25', 3), ('16', 1)],
        },
    'Non-Atom': {
        # IA32_ENERGY_PERF_BIAS[3:0] -- 0 == hi-perf, 6 balanced, 15 powersave
        '0x1b0': [('3:0', 6)],
        },
    }

# Give an ASPM exception for these PCI devices. ID is taken from lspci -n.
ASPM_EXCEPTED_DEVICES = {
    'Atom': [
        # Intel 82801G HDA Controller
        '8086:27d8'
        ],
    'Non-Atom': [
        # Intel HDA Controller
        '8086:1c20',
        '8086:1e20'
        ],
    }

GFX_CHECKS = {
    'Non-Atom': {'i915_enable_rc6': -1, 'i915_enable_fbc': 1, 'powersave': 1,
        'semaphores': 1, 'lvds_downclock': 1}
    }

# max & min are in Watts.  Device should presumably be idle.
RAPL_CHECKS = {
    'Non-Atom': {'pkg': {'max': 5.0, 'min': 1.0},
                'pp0': {'max': 1.2, 'min': 0.001},
                'pp1': {'max': 1.0, 'min': 0.000}}
    }

SUBTESTS = ['dmi', 'mch', 'msr', 'pcie_aspm', 'wifi', 'usb', 'storage',
            'audio', 'filesystem', 'graphics', 'rapl']


class power_x86Settings(test.test):
    """Class for power_x86Settings.  See 'control' for details."""
    version = 1


    def initialize(self):
        self._usbpower = power_utils.USBPower()


    def run_once(self):
        cpu_arch = power_utils.get_x86_cpu_arch()
        if not cpu_arch:
            raise error.TestNAError('Unsupported CPU')

        self._cpu_type = 'Atom'
        if cpu_arch is not 'Atom':
            self._cpu_type = 'Non-Atom'

        self._registers = power_utils.Registers()

        status = power_status.get_status()
        if status.on_ac():
            logging.info('AC Power is online')
            self._on_ac = True
        else:
            logging.info('AC Power is offline')
            self._on_ac = False

        failures = ''

        for testname in SUBTESTS:
            logging.info("SUBTEST = %s", testname)
            func = getattr(self, "_verify_%s_power_settings" % testname)
            fail_count = func()
            if fail_count:
                failures += '%s_failures(%d) ' % (testname, fail_count)

        if failures:
            raise error.TestFail(failures)


    def _verify_wifi_power_settings(self):
        if self._on_ac:
            expected_state = 'off'
        else:
            expected_state = 'on'

        iwconfig_out = utils.system_output('iwconfig 2>&1', retain_output=True)
        match = re.search(r'Power Management:(.*)', iwconfig_out)
        if match and match.group(1) == expected_state:
            return 0

        logging.info(iwconfig_out)
        return 1


    def _verify_storage_power_settings(self):
        if self._on_ac:
            return 0

        expected_state = 'min_power'

        dirs_path = '/sys/class/scsi_host/host*'
        dirs = glob.glob(dirs_path)
        if not dirs:
            logging.info('scsi_host paths not found')
            return 1

        for dirpath in dirs:
            link_policy_file = os.path.join(dirpath,
                                            'link_power_management_policy')
            if not os.path.exists(link_policy_file):
                logging.debug('path does not exist: %s', link_policy_file)
                continue

            out = utils.read_one_line(link_policy_file)
            logging.debug('storage: path set to %s for %s',
                           out, link_policy_file)
            if out == expected_state:
                return 0

        return 1


    def _verify_usb_power_settings(self):
        errors = 0
        self._usbpower.query_devices()
        for dev in self._usbpower.devices:
            # whitelist MUST autosuspend
            autosuspend = dev.autosuspend()
            logging.debug("USB %s:%s whitelisted:%s autosuspend:%s",
                          dev.vid, dev.pid, dev.whitelisted, autosuspend)
            if dev.whitelisted and not autosuspend:
                logging.error("Whitelisted USB %s:%s "
                              "has autosuspend disabled", dev.vid, dev.pid)
                errors += 1
            elif not dev.whitelisted:
                # TODO(crbug.com/242228): Deprecate warnings once we can
                # definitively identify preferred USB autosuspend settings
                logging.warning("Non-Whitelisted USB %s:%s present.  "
                                "Should it be whitelisted?", dev.vid, dev.pid)

        return errors


    def _verify_audio_power_settings(self):
        path = '/sys/module/snd_hda_intel/parameters/power_save'
        out = utils.read_one_line(path)
        logging.debug('Audio: %s = %s', path, out)
        power_save_timeout = int(out)

        # Make sure that power_save timeout parameter is zero if on AC.
        if self._on_ac:
            if power_save_timeout == 0:
                return 0
            else:
                logging.debug('Audio: On AC power but power_save = %d', \
                                                            power_save_timeout)
                return 1

        # Make sure that power_save timeout parameter is non-zero if on battery.
        elif power_save_timeout > 0:
            return 0

        logging.debug('Audio: On battery power but power_save = %d', \
                                                            power_save_timeout)
        return 1


    def _verify_filesystem_power_settings(self):
        mount_output = commands.getoutput('mount | fgrep commit=').split('\n')
        if len(mount_output) == 0:
            logging.debug('No file system entries with commit intervals found.')
            return 1

        errors = 0
        # Parse for 'commit' param
        for line in mount_output:
            try:
                commit = int(re.search(r'(commit=)([0-9]*)', line).group(2))
            except:
                errors += 1
                logging.error('Error(%d), reading commit value from \'%s\'',
                              errors, line)
                continue

            # Check for the correct commit interval.
            if commit != 600:
                errors += 1
                logging.error('Error(%d), incorrect commit interval %d', errors,
                              commit)

        return errors

    def _verify_lvds_downclock_mode_added(self):
        """Checks the kernel log for a message that an LVDS downclock mode has
        been added.

        This test is specific to alex/lumpy/parrot/stout since they use the i915
	driver (which has downclocking ability) and use known LCDs. These LCDs
	are special, in that they support a downclocked refresh rate, but don't
        advertise it in the EDID.

        To counteract this, I added a quirk in drm to add a downclocked mode to
        the panel. Unfortunately, upstream doesn't want this patch, and we have
        to carry it locally. The quirk patch was dropped inadvertently from
        chromeos-3.4, so this test ensures we don't regress again.

        I plan on writing an upstream friendly patch sometime in the near
        future, at which point I'll revert my drm hack and this test.

        Returns:
            0 if no errors, otherwise the number of errors that occurred.
        """
        cmd = 'cat /etc/lsb-release | grep CHROMEOS_RELEASE_BOARD'
        output = utils.system_output(cmd)
        if ('lumpy' not in output and 'alex' not in output and
	   'parrot' not in output and 'stout' not in output):
            return 0

        # Get the downclock message from the logs
        reader = cros_logging.LogReader()
        reader.set_start_by_reboot(-1)
        if not reader.can_find('Adding LVDS downclock mode'):
            logging.error('Error, LVDS downclock quirk not applied!')
            return 1

        return 0

    def _verify_graphics_power_settings(self):
        """Verify that power-saving for graphics are configured properly.

        Returns:
            0 if no errors, otherwise the number of errors that occurred.
        """
        errors = 0

        if self._cpu_type in GFX_CHECKS:
            checks = GFX_CHECKS[self._cpu_type]
            for param_name in checks:
                param_path = '/sys/module/i915/parameters/%s' % param_name
                if not os.path.exists(param_path):
                    errors += 1
                    logging.error('Error(%d), %s not found', errors, param_path)
                else:
                    out = utils.read_one_line(param_path)
                    logging.debug('Graphics: %s = %s', param_path, out)
                    value = int(out)
                    if value != checks[param_name]:
                        errors += 1
                        logging.error('Error(%d), %s = %d but should be %d',
                                      errors, param_path, value,
                                      checks[param_name])
        errors += self._verify_lvds_downclock_mode_added()

        # On systems which support RC6 (non atom), check that we get into rc6;
        # idle before doing so, and retry every second for 20 seconds.
        if self._cpu_type == 'Non-Atom':
            tries = 0
            found = False
            while found == False and tries < 20:
                time.sleep(1)
                param_path = "/sys/kernel/debug/dri/0/i915_drpc_info"
                if not os.path.exists(param_path):
                    logging.error('Error(%d), %s not found', errors, param_path)
                    break
                drpc_info_file = open (param_path, "r")
                for line in drpc_info_file:
                    match = re.search(r'Current RC state: (.*)', line)
                    if match:
                        found = match.group(1) != 'on'
                        break

                tries += 1
                drpc_info_file.close()

            if not found:
                errors += 1
                logging.error('Error(%d), did not see the GPU in RC6', errors)

        return errors


    def _verify_pcie_aspm_power_settings(self):
        errors = 0
        out = utils.system_output('lspci -n')
        for line in out.splitlines():
            slot, _, pci_id = line.split()[0:3]
            slot_out = utils.system_output('lspci -s %s -vv' % slot,
                                            retain_output=True)
            match = re.search(r'LnkCtl:(.*);', slot_out)
            if match:
                if pci_id in ASPM_EXCEPTED_DEVICES[self._cpu_type]:
                    continue

                split = match.group(1).split()
                if split[1] == 'Disabled' or \
                   (split[2] == 'Enabled' and split[1] != 'L1'):
                    errors += 1
                    logging.info(slot_out)
                    logging.error('Error(%d), %s ASPM off or no L1 support',
                                  errors, slot)
            else:
                logging.info('PCIe: LnkCtl not found for %s', line)

        return errors


    def _verify_dmi_power_settings(self):
        return self._registers.verify_dmi(DMI_BAR_CHECKS[self._cpu_type])

    def _verify_mch_power_settings(self):
        return self._registers.verify_mch(MCH_BAR_CHECKS[self._cpu_type])

    def _verify_msr_power_settings(self):
        return self._registers.verify_msr(MSR_CHECKS[self._cpu_type])

    def _verify_rapl_power_settings(self):
        errors = 0
        if self._cpu_type not in RAPL_CHECKS:
            return errors

        test_domains = RAPL_CHECKS[self._cpu_type].keys()
        rapls = power_rapl.create_rapl(domains=test_domains)

        time.sleep(2)
        for rapl in rapls:
            power = rapl.refresh()
            domain = rapl.domain
            test_params = RAPL_CHECKS[self._cpu_type][domain]
            logging.info('RAPL %s power during 2secs was: %.3fW',
                          domain, power)
            if power > test_params['max']:
                errors += 1
                logging.error('Error(%d), RAPL %s power > %.3fW',
                              errors, domain, test_params['max'])
            if power < test_params['min']:
                errors += 1
                logging.error('Error(%d), RAPL %s power < %.3fW',
                              errors, domain, test_params['min'])
        return errors
