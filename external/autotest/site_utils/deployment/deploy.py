#!/usr/bin/env python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Install an initial test image on a set of new DUTs.

This command is meant for deploying newly installed DUTs after
completing these steps:
  * Removing the write-protect screw.
  * Switching the DUT to dev mode.
  * Configuring the DUT to allow dev-mode boot from USB.
  * Installing the DUT on its shelf, fully cabled and ready to go.

The command will use servo to install dev-signed RO firmware on the
selected DUTs.  Then it forces the DUTs through the standard repair
flow, as in `repair.py`.

"""

import sys

import common
from autotest_lib.site_utils.deployment import install


def main(argv):
    """Standard main routine.

    @param argv  Command line arguments including `sys.argv[0]`.
    """
    install.install_duts(argv, full_deploy=True)


if __name__ == '__main__':
    try:
        main(sys.argv)
    except KeyboardInterrupt:
        pass
    except EnvironmentError as e:
        sys.stderr.write('Unexpected OS error:\n    %s\n' % e)
    except Exception as e:
        sys.stderr.write('Unexpected exception:\n    %s\n' % e)
