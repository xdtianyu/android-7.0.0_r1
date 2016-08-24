#!/usr/bin/env python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Force a DUT through the standard servo repair cycle.

This command is meant primarily for use in the following use case:
  * A DUT with servo attached has failed repair.
  * The servo has been fixed, and we now want to confirm the
    fix by using servo to repair the DUT.

The command will force selected DUTs through the standard servo
repair cycle, reinstalling the stable test image on the DUTs
from USB.

"""

import sys

import common
from autotest_lib.site_utils.deployment import install


def main(argv):
    """Standard main routine.

    @param argv  Command line arguments including `sys.argv[0]`.
    """
    install.install_duts(argv, full_deploy=False)


if __name__ == '__main__':
    try:
        main(sys.argv)
    except KeyboardInterrupt:
        pass
    except EnvironmentError as e:
        sys.stderr.write('Unexpected OS error:\n    %s\n' % e)
    except Exception as e:
        sys.stderr.write('Unexpected exception:\n    %s\n' % e)
