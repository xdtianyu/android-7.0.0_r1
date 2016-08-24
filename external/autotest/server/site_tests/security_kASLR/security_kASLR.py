# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class security_kASLR(test.test):
    """Tests the kASLR entropy seen across many reboots of a device
    """
    version = 1
    target_symbol = 'sys_exit'
    reboot_count = 100
    reboot_timeout = 60

    def _reboot_machine(self):
        """Reboot the client machine.

        We'll wait until the client is down, then up again.
        """
        boot_id = self._client.get_boot_id()
        self._client.run('reboot &')
        self._client.wait_for_restart(old_boot_id=boot_id,
                                      timeout=self.reboot_timeout,
                                      down_timeout=self.reboot_timeout,
                                      down_warning=self.reboot_timeout)

    def _read_kallsyms(self, filename):
        """Fetch /proc/kallsyms from client and return lines in the file

        @param filename: The file to write 'cat /proc/kallsyms' into.
        """

        f = open(filename, 'w')
        self._client.run('cat /proc/kallsyms', stdout_tee=f)
        f.close()

        return utils.read_file(filename)

    def _parse_kallsyms(self, kallsyms):
        """ Parse the contents of each line of kallsyms, extracting
            symbol addresses into a returned hash.

            @param kallsyms: string of kallsyms contents
        """
        symbols = {}
        for line in kallsyms.splitlines():
            addr, symtype, symbol = line.strip().split(' ', 2)
            # Just keep regular text symbols for now.
            if symtype != "T":
                continue
            symbols.setdefault(symbol, addr)
        return symbols

    def run_once(self, host=None):
        """Run the test.

        @param host: The client machine to connect to; should be a Host object.
        """
        assert host is not None, "The host must be specified."

        self._client = host

        # Report client configuration, to help debug any problems.
        kernel_ver = self._client.run('uname -r').stdout.rstrip()
        arch = utils.get_arch(self._client.run)
        logging.info("Starting kASLR tests for '%s' on '%s'",
                     kernel_ver, arch)

        # Make sure we're expecting kernel ASLR at all.
        if utils.compare_versions(kernel_ver, "3.8") < 0:
            logging.info("kASLR not available on this kernel")
            return
        if arch.startswith('arm'):
            logging.info("kASLR not available on this architecture")
            return

        kallsyms_filename = os.path.join(self.resultsdir, 'kallsyms')
        address_count = {}

        count = 0
        while True:
            kallsyms = self._read_kallsyms(kallsyms_filename)
            symbols = self._parse_kallsyms(kallsyms)

            assert symbols.has_key(self.target_symbol), \
                   "The '%s' symbol is missing!?" % (self.target_symbol)

            addr = symbols[self.target_symbol]
            logging.debug("Reboot %d: Symbol %s @ %s", \
                          count, self.target_symbol, addr)

            address_count.setdefault(addr, 0)
            address_count[addr] += 1

            count += 1
            if count == self.reboot_count:
                break
            self._reboot_machine()

        unique = len(address_count)
        logging.info("Unique kernel offsets: %d", unique)
        highest = 0
        for addr in address_count:
            logging.debug("Address %s: %d", addr, address_count[addr])
            if address_count[addr] > highest:
                highest = address_count[addr]
        if unique < 2:
            raise error.TestFail("kASLR not functioning")
        if unique < (self.reboot_count / 3):
            raise error.TestFail("kASLR entropy seems very low")
        if highest > (unique / 10):
            raise error.TestFail("kASLR entropy seems to clump")
