# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, sys

from config import rpm_config
import rpm_controller

LOGGING_FORMAT = rpm_config.get('GENERAL','logging_format')
oyster_rpm_name_format = 'chromeos1-rack%d-rpm1'
atlantis_rpm_name_format = 'chromeos2-row%d-rack%d-rpm1'
DEFAULT_OYSTERBAY_OUTLET_MAP = {
    1 : 'host1',
    2 : 'host2',
    4 : 'host3',
    5 : 'host4',
    7 : 'host5',
    8 : 'host6',
    9 : 'host7',
    10 : 'host8',
    12 : 'host9',
    13 : 'host10',
    15 : 'host11',
    16 : 'host12'
}
DEFAULT_ATLANTIS_OUTLET_MAP = {
    1 : 'host1',
    2 : 'host7',
    4 : 'host2',
    5 : 'host8',
    7 : 'host3',
    8 : 'host9',
    9 : 'host4',
    10 : 'host10',
    12 : 'host5',
    13 : 'host11',
    15 : 'host6',
    16 : 'host12'
}


def setup_rpm(rpm_name):
    logging.debug('Setting up %s.', rpm_name)
    rpm = rpm_controller.SentryRPMController(rpm_name)
    if rpm_name.startswith('chromeos1'):
        outlet_mapping = DEFAULT_OYSTERBAY_OUTLET_MAP
    else:
        outlet_mapping = DEFAULT_ATLANTIS_OUTLET_MAP
    if not rpm.setup(outlet_mapping):
        logging.error('Failed to set up %s.', rpm_name)


def main():
    if len(sys.argv) != 2:
        print 'USAGE: python %s [rpm|atlantis|oyster]' % sys.argv[0]
        print 'atlantis|oyster: implies all RPMs inside that lab.'
        return
    if sys.argv[1] != 'atlantis' and sys.argv[1] != 'oyster':
        setup_rpm(sys.argv[1])
        return
    if sys.argv[1] == 'oyster':
        logging.debug('Setting up All RPM devices in lab: Oyster Bay.')
        for rack in range(3,8):
            setup_rpm(oyster_rpm_name_format % rack)
        return
    logging.debug('Setting up All RPM devices in lab: Atlantis.')
    for row in range(1,6):
        for rack in range(1,8):
            if ((row == 1 and rack == 1) or (row == 1 and rack == 2) or
                (row == 5 and rack == 6) or (row ==5 and rack == 7)):
                # These RPM's do not follow the normal layout.
                continue
            setup_rpm(atlantis_rpm_name_format % (row, rack))


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG, format=LOGGING_FORMAT)
    main()
