# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the system-related functionality."""

import os

from autotest_lib.client.bin import utils


class SystemFacadeNativeError(Exception):
    """Error in SystemFacadeNative."""
    pass


class SystemFacadeNative(object):
    """Facede to access the system-related functionality.

    The methods inside this class only accept Python native types.

    """
    SCALING_GOVERNOR_MODES = [
            'interactive',
            'performance',
            'ondemand',
            'powersave',
            ]

    def set_scaling_governor_mode(self, index, mode):
        """Set mode of CPU scaling governor on one CPU.

        @param index: CPU index starting from 0.

        @param mode: Mode of scaling governor, accept 'interactive' or
                     'performance'.

        @returns: The original mode.

        """
        if mode not in self.SCALING_GOVERNOR_MODES:
            raise SystemFacadeNativeError('mode %s is invalid' % mode)

        governor_path = os.path.join(
                '/sys/devices/system/cpu/cpu%d' % index,
                'cpufreq/scaling_governor')
        if not os.path.exists(governor_path):
            raise SystemFacadeNativeError(
                    'scaling governor of CPU %d is not available' % index)

        original_mode = utils.read_one_line(governor_path)
        utils.open_write_close(governor_path, mode)

        return original_mode
