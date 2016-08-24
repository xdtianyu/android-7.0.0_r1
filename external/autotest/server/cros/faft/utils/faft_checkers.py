# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot


class FAFTCheckers(object):
    """Class that contains FAFT checkers."""
    version = 1

    def __init__(self, faft_framework):
        self.faft_framework = faft_framework
        self.faft_client = faft_framework.faft_client
        self.faft_config = faft_framework.faft_config
        self.fw_vboot2 = self.faft_client.system.get_fw_vboot2()

    def _parse_crossystem_output(self, lines):
        """Parse the crossystem output into a dict.

        @param lines: The list of crossystem output strings.
        @return: A dict which contains the crossystem keys/values.
        @raise TestError: If wrong format in crossystem output.

        >>> seq = FAFTSequence()
        >>> seq._parse_crossystem_output([ \
                "arch          = x86    # Platform architecture", \
                "cros_debug    = 1      # OS should allow debug", \
            ])
        {'cros_debug': '1', 'arch': 'x86'}
        >>> seq._parse_crossystem_output([ \
                "arch=x86", \
            ])
        Traceback (most recent call last):
            ...
        TestError: Failed to parse crossystem output: arch=x86
        >>> seq._parse_crossystem_output([ \
                "arch          = x86    # Platform architecture", \
                "arch          = arm    # Platform architecture", \
            ])
        Traceback (most recent call last):
            ...
        TestError: Duplicated crossystem key: arch
        """
        pattern = "^([^ =]*) *= *(.*[^ ]) *# [^#]*$"
        parsed_list = {}
        for line in lines:
            matched = re.match(pattern, line.strip())
            if not matched:
                raise error.TestError("Failed to parse crossystem output: %s"
                                      % line)
            (name, value) = (matched.group(1), matched.group(2))
            if name in parsed_list:
                raise error.TestError("Duplicated crossystem key: %s" % name)
            parsed_list[name] = value
        return parsed_list

    def crossystem_checker(self, expected_dict, suppress_logging=False):
        """Check the crossystem values matched.

        Given an expect_dict which describes the expected crossystem values,
        this function check the current crossystem values are matched or not.

        @param expected_dict: A dict which contains the expected values.
        @param suppress_logging: True to suppress any logging messages.
        @return: True if the crossystem value matched; otherwise, False.
        """
        succeed = True
        lines = self.faft_client.system.run_shell_command_get_output(
                'crossystem')
        got_dict = self._parse_crossystem_output(lines)
        for key in expected_dict:
            if key not in got_dict:
                logging.warn('Expected key %r not in crossystem result', key)
                succeed = False
                continue
            if isinstance(expected_dict[key], str):
                if got_dict[key] != expected_dict[key]:
                    message = ('Expected %r value %r but got %r' % (
                               key, expected_dict[key], got_dict[key]))
                    succeed = False
                else:
                    message = ('Expected %r value %r == real value %r' % (
                               key, expected_dict[key], got_dict[key]))

            elif isinstance(expected_dict[key], tuple):
                # Expected value is a tuple of possible actual values.
                if got_dict[key] not in expected_dict[key]:
                    message = ('Expected %r values %r but got %r' % (
                               key, expected_dict[key], got_dict[key]))
                    succeed = False
                else:
                    message = ('Expected %r values %r == real value %r' % (
                               key, expected_dict[key], got_dict[key]))
            else:
                logging.warn('The expected value of %r is neither a str nor a '
                             'dict: %r', key, expected_dict[key])
                succeed = False
                continue
            if not suppress_logging:
                logging.info(message)
        return succeed

    def mode_checker(self, mode):
        """Check the current system in the given mode.

        @param mode: A string of mode, one of 'normal', 'dev', or 'rec'.
        @return: True if the system in the given mode; otherwise, False.
        """
        is_devsw = (self.faft_config.mode_switcher_type ==
                    'physical_button_switcher')
        if mode == 'normal':
            if is_devsw:
                return self.crossystem_checker(
                        {'devsw_cur': '0'},
                        suppress_logging=True)
            else:
                return self.crossystem_checker(
                        {'devsw_boot': '0',
                         'mainfw_type': 'normal'},
                        suppress_logging=True)
        elif mode == 'dev':
            if is_devsw:
                return self.crossystem_checker(
                        {'devsw_cur': '1'},
                        suppress_logging=True)
            else:
                return self.crossystem_checker(
                        {'devsw_boot': '1',
                         'mainfw_type': 'developer'},
                        suppress_logging=True)
        elif mode == 'rec':
            return self.crossystem_checker(
                    {'mainfw_type': 'recovery'},
                    suppress_logging=True)
        else:
            raise NotImplementedError('The given mode %s not supported' % mode)

    def fw_tries_checker(self,
                         expected_mainfw_act,
                         expected_fw_tried=True,
                         expected_try_count=0):
        """Check the current FW booted and try_count

        Mainly for dealing with the vboot1-specific flags fwb_tries and
        tried_fwb fields in crossystem.  In vboot2, fwb_tries is meaningless and
        is ignored while tried_fwb is translated into fw_try_count.

        @param expected_mainfw_act: A string of expected firmware, 'A', 'B', or
                       None if don't care.
        @param expected_fw_tried: True if tried expected FW at last boot.
                       This means that mainfw_act=A,tried_fwb=0 or
                       mainfw_act=B,tried_fwb=1. Set to False if want to
                       check the opposite case for the mainfw_act.  This
                       check is only performed in vboot1 as tried_fwb is
                       never set in vboot2.
        @param expected_try_count: Number of times to try a FW slot.

        @return: True if the correct boot firmware fields matched.  Otherwise,
                       False.
        """
        crossystem_dict = {'mainfw_act': expected_mainfw_act.upper()}

        if not self.fw_vboot2:
            if expected_mainfw_act == 'B':
                tried_fwb_val = True
            else:
                tried_fwb_val = False
            if not expected_fw_tried:
                tried_fwb_val = not tried_fwb_val
            crossystem_dict['tried_fwb'] = '1' if tried_fwb_val else '0'

            crossystem_dict['fwb_tries'] = str(expected_try_count)
        else:
            crossystem_dict['fw_try_count'] = str(expected_try_count)
        return self.crossystem_checker(crossystem_dict)

    def vdat_flags_checker(self, mask, value):
        """Check the flags from VbSharedData matched.

        This function checks the masked flags from VbSharedData using crossystem
        are matched the given value.

        @param mask: A bitmask of flags to be matched.
        @param value: An expected value.
        @return: True if the flags matched; otherwise, False.
        """
        lines = self.faft_client.system.run_shell_command_get_output(
                    'crossystem vdat_flags')
        vdat_flags = int(lines[0], 16)
        if vdat_flags & mask != value:
            logging.info("Expected vdat_flags 0x%x mask 0x%x but got 0x%x",
                         value, mask, vdat_flags)
            return False
        return True

    def ro_normal_checker(self, expected_fw=None, twostop=False):
        """Check the current boot uses RO boot.

        @param expected_fw: A string of expected firmware, 'A', 'B', or
                            None if don't care.
        @param twostop: True to expect a TwoStop boot; False to expect a RO
                        boot.
        @return: True if the currect boot firmware matched and used RO boot;
                 otherwise, False.
        """
        crossystem_dict = {'tried_fwb': '0'}
        if expected_fw:
            crossystem_dict['mainfw_act'] = expected_fw.upper()
        succeed = True
        if not self.vdat_flags_checker(vboot.VDAT_FLAG_LF_USE_RO_NORMAL,
                0 if twostop else vboot.VDAT_FLAG_LF_USE_RO_NORMAL):
            succeed = False
        if not self.crossystem_checker(crossystem_dict):
            succeed = False
        if self.faft_framework.check_ec_capability(suppress_warning=True):
            expected_ec = ('RW' if twostop else 'RO')
            if not self.ec_act_copy_checker(expected_ec):
                succeed = False
        return succeed

    def dev_boot_usb_checker(self, dev_boot_usb=True):
        """Check the current boot is from a developer USB (Ctrl-U trigger).

        @param dev_boot_usb: True to expect an USB boot;
                             False to expect an internal device boot.
        @return: True if the currect boot device matched; otherwise, False.
        """
        return (self.crossystem_checker({'mainfw_type': 'developer'}) and
            self.faft_client.system.is_removable_device_boot() == dev_boot_usb)

    def root_part_checker(self, expected_part):
        """Check the partition number of the root device matched.

        @param expected_part: A string containing the number of the expected
                              root partition.
        @return: True if the currect root  partition number matched;
                 otherwise, False.
        """
        part = self.faft_client.system.get_root_part()[-1]
        if self.faft_framework.ROOTFS_MAP[expected_part] != part:
            logging.info("Expected root part %s but got %s",
                         self.faft_framework.ROOTFS_MAP[expected_part], part)
            return False
        return True

    def ec_act_copy_checker(self, expected_copy):
        """Check the EC running firmware copy matches.

        @param expected_copy: A string containing 'RO', 'A', or 'B' indicating
                              the expected copy of EC running firmware.
        @return: True if the current EC running copy matches; otherwise, False.
        """
        if self.faft_client.system.has_host():
            cmd = 'fwtool ec version'
        else:
            cmd = 'ectool version'
        lines = self.faft_client.system.run_shell_command_get_output(cmd)
        pattern = re.compile("Firmware copy: (.*)")
        for line in lines:
            matched = pattern.match(line)
            if matched:
                if matched.group(1) == expected_copy:
                    return True
                else:
                    logging.info("Expected EC in %s but now in %s",
                                 expected_copy, matched.group(1))
                    return False
        logging.info("Wrong output format of '%s':\n%s", cmd, '\n'.join(lines))
        return False
