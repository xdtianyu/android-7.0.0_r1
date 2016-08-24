# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module containing TPM handler class used by SAFT."""

FW_NV_ADDRESS = 0x1007
KERNEL_NV_ADDRESS = 0x1008

class TpmError(Exception):
    pass

class TpmNvRam(object):
    """An object representing TPM NvRam.

    Attributes:
    addr: a number, NvRAm address in TPM.
    size: a number, count of bites in this NvRam section.
    os_if: an instance of the OS interface (os_interface or a mock object).
    version_offset: - a number, offset into the NvRam contents where the the
        versions are stored. The total version field size is 4 bytes, the
        first two bytes are the body version, the second two bytes are the key
        version. Numbers are stored in little endian format.
    pattern: optional, a tuple of two elements, the first element is the
       offset of the pattern expected to be present in the NvRam, and the
       second element is an array of bytes the pattern must match.
    contents: an array of bytes, the contents of the NvRam.
    """

    def __init__(self, addr, size, version_offset, data_pattern=None):
        self.addr = addr
        self.size = size
        self.os_if = None
        self.version_offset = version_offset
        self.pattern = data_pattern
        self.contents = []

    def init(self, os_if):
        self.os_if = os_if
        cmd = 'tpmc read 0x%x 0x%x' % (self.addr, self.size)
        nvram_data = self.os_if.run_shell_command_get_output(cmd)[0].split()
        self.contents = [int(x, 16) for x in nvram_data]
        if self.pattern:
            pattern_offset = self.pattern[0]
            pattern_data = self.pattern[1]
            contents_pattern = self.contents[pattern_offset:pattern_offset +
                                             len(pattern_data)]
            if contents_pattern != pattern_data:
                raise TpmError('Nvram pattern does not match')

    def get_body_version(self):
        return self.contents[
            self.version_offset + 1] * 256 + self.contents[self.version_offset]

    def get_key_version(self):
        return self.contents[
            self.version_offset + 3] * 256 + self.contents[
            self.version_offset + 2]

class TpmHandler(object):
    """An object to control TPM device's NVRAM.

    Attributes:
      os_if: an instance of the OS interface (os_interface or a mock object).
      nvrams: A dictionary where the keys are the nvram names, and the values
          are instances of TpmNvRam objects, providing access to the
          appropriate TPM NvRam sections.
    """

    def __init__(self):
        self.os_if = None
        self.nvrams = {
            'kernel': TpmNvRam(KERNEL_NV_ADDRESS, 13, 5, (
                    1, [0x4c, 0x57, 0x52, 0x47])),
            'bios': TpmNvRam(FW_NV_ADDRESS, 10, 2)
            }

    def init(self, os_if):
        self.os_if = os_if
        status = self.os_if.run_shell_command_get_output(
            'initctl status tcsd')[0]
        if status.startswith('tcsd start/running'):
            self.os_if.run_shell_command('stop tcsd')

        for nvram in self.nvrams.itervalues():
            nvram.init(self.os_if)

    def get_fw_version(self):
        return self.nvrams['bios'].get_body_version()

    def get_fw_body_version(self):
        return self.nvrams['bios'].get_key_version()

    def get_kernel_version(self):
        return self.nvrams['kernel'].get_body_version()
