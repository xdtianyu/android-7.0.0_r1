# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Expects to be run in an environment with sudo and no interactive password
# prompt, such as within the Chromium OS development chroot.


"""This file provides core logic for servo verify/repair process."""

from autotest_lib.server.hosts import servo_host


# Names of the host attributes in the database that represent the values for
# the servo_host and servo_port for a servo connected to the DUT.
PLANKTON_HOST_ATTR = 'plankton_host'
PLANKTON_PORT_ATTR = 'plnakton_port'


def make_plankton_hostname(dut_hostname):
    """Given a DUT's hostname, return the hostname of its servo.

    @param dut_hostname: hostname of a DUT.

    @return hostname of the DUT's servo.

    """
    host_parts = dut_hostname.split('.')
    host_parts[0] = host_parts[0] + '-plankton'
    return '.'.join(host_parts)


class PlanktonHost(servo_host.ServoHost):
    """Host class for a host that controls a servo, e.g. beaglebone."""


    def _initialize(self, plankton_host='localhost', plankton_port=9998,
                    required_by_test=True, is_in_lab=None, *args, **dargs):
        """Initialize a ServoHost instance.

        A ServoHost instance represents a host that controls a servo.

        @param plankton_host: Name of the host where the servod process
                           is running.
        @param plankton_port: Port the servod process is listening on.

        """
        super(PlanktonHost, self)._initialize(plankton_host, plankton_port,
                                              False, None, *args, **dargs)


def create_plankton_host(plankton_args):
    """Create a PlanktonHost object used to access plankton servo

    The `plankton_args` parameter is a dictionary specifying optional
    Servo client parameter overrides (i.e. a specific host or port).
    When specified, the caller requires that an exception be raised
    unless both the PlanktonHost and the Servo are successfully
    created.

    @param plankton_args: A dictionary that contains args for creating
                       a PlanktonHost object,
                       e.g. {'planton_host': '172.11.11.111',
                             'plankton_port': 9999}.
                       See comments above.

    @returns: A PlanktonHost object or None. See comments above.

    """
    # TODO Make this work in the lab chromium:564836
    if plankton_args is None:
        return None
    return PlanktonHost(Required_by_test=True, is_in_lab=False, **plankton_args)
