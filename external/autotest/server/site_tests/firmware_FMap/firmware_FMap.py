# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

TARGET_BIOS = 'host_firmware'
TARGET_EC = 'ec_firmware'

FMAP_AREA_NAMES = [
    'name',
    'offset',
    'size'
]

EXPECTED_FMAP_TREE_BIOS = {
  'WP_RO': {
    'RO_SECTION': {
      'FMAP': {},
      'GBB': {},
      'RO_FRID': {},
    },
    'RO_VPD': {},
  },
  'RW_SECTION_A': {
    'VBLOCK_A': {},
    'FW_MAIN_A': {},
    'RW_FWID_A': {},
  },
  'RW_SECTION_B': {
    'VBLOCK_B': {},
    'FW_MAIN_B': {},
    'RW_FWID_B': {},
  },
  'RW_VPD': {},
}

EXPECTED_FMAP_TREE_EC = {
  'WP_RO': {
    'EC_RO': {
      'FMAP': {},
      'RO_FRID': {},
    },
  },
  'EC_RW': {
    'RW_FWID': {},
  },
}

class firmware_FMap(FirmwareTest):
    """Provides access to firmware FMap"""

    _TARGET_AREA = {
        TARGET_BIOS: [],
        TARGET_EC: [],
    }

    _EXPECTED_FMAP_TREE = {
        TARGET_BIOS: EXPECTED_FMAP_TREE_BIOS,
        TARGET_EC: EXPECTED_FMAP_TREE_EC,
    }

    """Client-side FMap test.

    This test checks the active BIOS and EC firmware contains the required
    FMap areas and verifies their hierarchies. It relies on flashrom to dump
    the active BIOS and EC firmware and dump_fmap to decode them.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_FMap, self).initialize(host, cmdline_args)

    def run_cmd(self, command):
        """
        Log and execute command and return the output.

        @param command: Command to executeon device.
        @returns the output of command.

        """
        logging.info('Execute %s', command)
        output = self.faft_client.system.run_shell_command_get_output(command)
        logging.info('Output %s', output)
        return output

    def get_areas(self):
        """Get a list of dicts containing area names, offsets, and sizes
        per device.

        It fetches the FMap data from the active firmware via mosys.
        Stores the result in the appropriate _TARGET_AREA.
        """
        lines = self.run_cmd("mosys eeprom map")

        # The above output is formatted as:
        # name1 offset1 size1
        # name2 offset2 size2
        # ...
        # Convert it to a list of dicts like:
        # [{'name': name1, 'offset': offset1, 'size': size1},
        #  {'name': name2, 'offset': offset2, 'size': size2}, ...]
        for line in lines:
            row = line.split(' | ')
            self._TARGET_AREA[row[0]].append(
                dict(zip(FMAP_AREA_NAMES, [row[1], row[2], row[3]])))


    def _is_bounded(self, region, bounds):
        """Is the given region bounded by the given bounds?"""
        return ((bounds[0] <= region[0] < bounds[1]) and
                (bounds[0] < region[1] <= bounds[1]))


    def _is_overlapping(self, region1, region2):
        """Is the given region1 overlapping region2?"""
        return (min(region1[1], region2[1]) > max(region1[0], region2[0]))


    def check_section(self):
        """Check RW_SECTION_[AB] and RW_LEGACY.

        1- check RW_SECTION_[AB] exist, non-zero, same size
        2- RW_LEGACY exist and > 1MB in size
        """
        # Parse map into dictionary.
        bios = {}
        for e in self._TARGET_AREA[TARGET_BIOS]:
           bios[e['name']] = {'offset': e['offset'], 'size': e['size']}
        succeed = True
        # Check RW_SECTION_[AB] sections.
        if 'RW_SECTION_A' not in bios:
            succeed = False
            logging.error('Missing RW_SECTION_A section in FMAP')
        elif 'RW_SECTION_B' not in bios:
            succeed = False
            logging.error('Missing RW_SECTION_B section in FMAP')
        else:
            if bios['RW_SECTION_A']['size'] != bios['RW_SECTION_B']['size']:
                succeed = False
                logging.error('RW_SECTION_A size != RW_SECTION_B size')
            if (bios['RW_SECTION_A']['size'] == 0
                or bios['RW_SECTION_B']['size'] == 0):
                succeed = False
                logging.error('RW_SECTION_A size or RW_SECTION_B size == 0')
        # Check RW_LEGACY section.
        if 'RW_LEGACY' not in bios:
            succeed = False
            logging.error('Missing RW_LEGACY section in FMAP')
        else:
            if bios['RW_LEGACY']['size'] < 1024*1024:
                succeed = False
                logging.error('RW_LEGACY size is < 1M')
        if not succeed:
            raise error.TestFail('SECTION check failed.')


    def check_areas(self, areas, expected_tree, bounds=None):
        """Check the given area list met the hierarchy of the expected_tree.

        It checks all areas in the expected tree are existed and non-zero sized.
        It checks all areas in sub-trees are bounded by the region of the root
        node. It also checks all areas in child nodes are mutually exclusive.

        @param areas: A list of dicts containing area names, offsets, and sizes.
        @param expected_tree: A hierarchy dict of the expected FMap tree.
        @param bounds: The boards that all areas in the expect_tree are bounded.
                       If None, ignore the bounds check.

        >>> f = FMap()
        >>> a = [{'name': 'FOO', 'offset': 100, 'size': '200'},
        ...      {'name': 'BAR', 'offset': 100, 'size': '50'},
        ...      {'name': 'ZEROSIZED', 'offset': 150, 'size': '0'},
        ...      {'name': 'OUTSIDE', 'offset': 50, 'size': '50'}]
        ...      {'name': 'OVERLAP', 'offset': 120, 'size': '50'},
        >>> f.check_areas(a, {'FOO': {}})
        True
        >>> f.check_areas(a, {'NOTEXISTED': {}})
        False
        >>> f.check_areas(a, {'ZEROSIZED': {}})
        False
        >>> f.check_areas(a, {'BAR': {}, 'OVERLAP': {}})
        False
        >>> f.check_areas(a, {'FOO': {}, 'BAR': {}})
        False
        >>> f.check_areas(a, {'FOO': {}, 'OUTSIDE': {}})
        True
        >>> f.check_areas(a, {'FOO': {'BAR': {}}})
        True
        >>> f.check_areas(a, {'FOO': {'OUTSIDE': {}}})
        False
        >>> f.check_areas(a, {'FOO': {'NOTEXISTED': {}}})
        False
        >>> f.check_areas(a, {'FOO': {'ZEROSIZED': {}}})
        False
        """

        succeed = True
        checked_regions = []
        for branch in expected_tree:
            area = next((a for a in areas if a['name'] == branch), None)
            if not area:
                logging.error("The area %s is not existed.", branch)
                succeed = False
                continue
            region = [int(area['offset'], 16),
                      int(area['offset'], 16) + int(area['size'], 16)]
            if int(area['size'], 16) == 0:
                logging.error("The area %s is zero-sized.", branch)
                succeed = False
            elif bounds and not self._is_bounded(region, bounds):
                logging.error("The region %s [%d, %d) is out of the bounds "
                              "[%d, %d).", branch, region[0], region[1],
                              bounds[0], bounds[1])
                succeed = False
            elif any(r for r in checked_regions if self._is_overlapping(
                    region, r)):
                logging.error("The area %s is overlapping others.", branch)
                succeed = False
            elif not self.check_areas(areas, expected_tree[branch], region):
                succeed = False
            checked_regions.append(region)
        return succeed


    def run_once(self):
        self.get_areas()

        for key in self._TARGET_AREA.keys():
            if (self._TARGET_AREA[key] and
                    not self.check_areas(self._TARGET_AREA[key],
                                         self._EXPECTED_FMAP_TREE[key])):
                raise error.TestFail("%s FMap is not qualified.", key)
        self.check_section()
