# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

_KERN_WARNING = 4

_WHITELIST_COMMON = [
    r"used greatest stack depth: \d+ bytes left",
    "Kernel-defined memdesc doesn't match the one from EFI!",
    "Use a HIGHMEM enabled kernel.",
   "GPT: Use GNU Parted to correct GPT errors.",
    r"GPT:\d+ != \d+",
    "GPT:Alternate GPT header not at the end of the disk.",
    "GPT:Primary header thinks Alt. header is not at the end of the disk.",
    r"GPT:partition_entry_array_crc32 values don't match: 0x[\da-f]+ !="
    " 0x[\da-f]+",
    r"Warning only \d+MB will be used.",
    "\[drm:intel_init_bios\] \*ERROR\* VBT signature missing",
    "i2c i2c-2: The new_device interface is still experimental and may change "
    "in a near future",
    "i915 0000:00:02.0: Invalid ROM contents",
    "industrialio: module is from the staging directory, the quality is "
    "unknown, you have been warned.",
    "pnp 00:01: io resource \(0x164e-0x164f\) overlaps 0000:00:1c.0 "
    "BAR 7 \(0x1000-0x1fff\), disabling",
    r"sd \d:\d:\d:\d: \[sd[a-z]\] Assuming drive cache: write through",
    "tsl[\da-z]+: module is from the staging directory, the quality is "
    "unknown, you have been warned.",
    "usb 1-2: unknown number of interfaces: 4",
]

_WHITELIST_TARGETS = {
    'Alex' : [
        r"CE: hpet increasing min_delta_ns to \d+ nsec",
        r"Measured \d+ cycles TSC warp between CPUs, turning off TSC clock.",
        "pci 0000:01:00.0: BAR 6: no parent found for of device "
        "\[0xffff0000-0xffffffff]",
        "tsl258x 2-0029: taos_get_lux data not valid",
        "usb 1-2: config 1 has an invalid interface number: 1 but max is 0",
        "usb 1-2: config 1 has no interface number 0",
        ],
    'Mario' : [
        "chromeos_acpi: failed to retrieve MLST \(5\)",
        r"btusb_[a-z]{4}_complete: hci\d urb [\da-f]+ failed to resubmit \(1\)",
        ]
}

""" Interesting fields from meminfo that we want to log
    If you add fields here, you must add them to the constraints
    in the control file
"""
_meminfo_fields = { 'MemFree'   : 'coldboot_memfree_mb',
                    'AnonPages' : 'coldboot_anonpages_mb',
                    'Buffers'   : 'coldboot_buffers_mb',
                    'Cached'    : 'coldboot_cached_mb',
                    'Active'    : 'coldboot_active_mb',
                    'Inactive'  : 'coldboot_inactive_mb',
                    }

class kernel_BootMessagesServer(test.test):
    version = 1


    def _read_dmesg(self, filename):
        """Put the contents of 'dmesg -r' into the given file.

        @param filename: The file to write 'dmesg -r' into.
        """
        f = open(filename, 'w')
        self._client.run('dmesg -r', stdout_tee=f)
        f.close()

        return utils.read_file(filename)

    def _reboot_machine(self):
        """Reboot the client machine.

        We'll wait until the client is down, then up again.
        """
        self._client.run('reboot')
        self._client.wait_down()
        self._client.wait_up()

    def _read_meminfo(self, filename):
        """Fetch /proc/meminfo from client and return lines in the file

        @param filename: The file to write 'cat /proc/meminfo' into.
        """

        f = open(filename, 'w')
        self._client.run('cat /proc/meminfo', stdout_tee=f)
        f.close()

        return utils.read_file(filename)

    def _parse_meminfo(self, meminfo, perf_vals):
        """ Parse the contents of each line of meminfo
            if the line matches one of the interesting keys
            save it into perf_vals in terms of megabytes

            @param filelines: list of lines in meminfo
            @param perf_vals: dictionary of performance metrics
        """

        for line in meminfo.splitlines():
            stuff = re.match('(.*):\s+(\d+)', line)
            stat  = stuff.group(1)
            if stat in _meminfo_fields:
                value  = int(stuff.group(2))/ 1024
                metric = _meminfo_fields[stat]
                perf_vals[metric] = value

    def _check_acpi_output(self, text, fwid):
        # This dictionary is the database of expected strings in dmesg output.
        # The keys are platform names, the values are two tuples, the first
        # element is the regex to filter the messages, the second element is a
        # set of strings to be found in the filtered dmesg set.
        message_db = {
            'Alex' : (r'(chromeos_acpi:|ChromeOS )', (
                    'chromeos_acpi: registering CHSW 0',
                    'chromeos_acpi: registering VBNV 0',
                    'chromeos_acpi: registering VBNV 1',
                    r'chromeos_acpi: truncating buffer from \d+ to \d+',
                    'chromeos_acpi: installed',
                    'ChromeOS firmware detected')),

            'Mario' : (r'(chromeos_acpi|ChromeOS )', (
                    'chromeos_acpi: falling back to default list of methods',
                    'chromeos_acpi: registering CHSW 0',
                    'chromeos_acpi: registering CHNV 0',
                    'chromeos_acpi: failed to retrieve MLST \(5\)',
                    'chromeos_acpi: installed',
                    'Legacy ChromeOS firmware detected'))
            }

        if fwid not in message_db:
            msg = 'Unnown platform %s, acpi dmesg set not defined.' % fwid
            logging.error(msg)
            raise error.TestFail(msg)

        rv = utils.verify_mesg_set(text,
                                   message_db[fwid][0],
                                   message_db[fwid][1])
        if rv:
            logging.error('ACPI mismatch\n%s:' % rv)
            raise error.TestFail('ACPI dmesg mismatch')

    def run_once(self, host=None):
        """Run the test.

        @param host: The client machine to connect to; should be a Host object.
        """
        assert host is not None, "The host must be specified."

        self._client = host

        # get the firmware identifier from Crossystem
        cs = utils.Crossystem(self._client)
        cs.init()
        fwid = cs.fwid().split('.')[0]

        dmesg_filename = os.path.join(self.resultsdir, 'dmesg')
        meminfo_filename = os.path.join(self.resultsdir, 'meminfo')
        perf_vals = {}

        self._reboot_machine()
        meminfo = self._read_meminfo(meminfo_filename)
        self._parse_meminfo(meminfo, perf_vals)
        dmesg = self._read_dmesg(dmesg_filename)

        if fwid not in _WHITELIST_TARGETS:
            msg = 'Unnown platform %s, whitelist dmesg set not defined.' % fwid
            logging.error(msg)
            raise error.TestFail(msg)

        unexpected = utils.check_raw_dmesg(
            dmesg, _KERN_WARNING, _WHITELIST_COMMON + _WHITELIST_TARGETS[fwid])

        if unexpected:
            f = open(os.path.join(self.resultsdir, 'dmesg.err'), 'w')
            for line in unexpected:
                logging.error('UNEXPECTED DMESG: %s' % line)
                f.write('%s\n' % line)
            f.close()
            raise error.TestFail("Unexpected dmesg warnings and/or errors.")

        self.write_perf_keyval(perf_vals)

        self._check_acpi_output(dmesg, fwid)
