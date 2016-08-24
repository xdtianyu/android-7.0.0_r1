# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.networking import shill_proxy


class WiMaxProxy(shill_proxy.ShillProxy):
    """Wrapper around shill dbus interface used by WiMAX tests."""

    def set_logging_for_wimax_test(self):
        """Set the logging in shill for a test of WiMAX technology.

        Set the log level to |ShillProxy.LOG_LEVEL_FOR_TEST| and the log scopes
        to the ones defined in |ShillProxy.LOG_SCOPES_FOR_TEST| for
        |ShillProxy.TECHNOLOGY_WIMAX|.

        """
        self.set_logging_for_test(self.TECHNOLOGY_WIMAX)


    def find_wimax_service_object(self):
        """Returns the first dbus object found that is a WiMAX service.

        @return DBus object for the first WiMAX service found. None if no
                service found.

        """
        return self.find_object('Service', {'Type': self.TECHNOLOGY_WIMAX})


    def find_wimax_device_object(self):
        """Returns the first dbus object found that is a WiMAX device.

        @return DBus object for the first WiMAX device found. None if no
                device found.

        """
        return self.find_object('Device', {'Type': self.TECHNOLOGY_WIMAX})
