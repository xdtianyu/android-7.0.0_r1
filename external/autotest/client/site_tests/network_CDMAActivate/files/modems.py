# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.types
import gobject

from autotest_lib.client.cros.cellular.pseudomodem import modem_cdma
from autotest_lib.client.cros.cellular.pseudomodem import pm_errors
from autotest_lib.client.cros.cellular.pseudomodem import utils as pm_utils

I_ACTIVATION_TEST = 'Interface.CDMAActivationTest'

class UnactivatedCdmaModem(modem_cdma.ModemCdma):
    """ A |ModemCDMA| subclass that starts off unactivated. """
    def __init__(self):
        super(UnactivatedCdmaModem, self).__init__(
                home_network=modem_cdma.ModemCdma.CdmaNetwork(activated=False))


class ActivationRetryModem(modem_cdma.ModemCdma):
    """
    TestModem to test that shill retries OTASP activation until it succeeds.

    """
    def __init__(self, num_activate_retries):
        # This assignment is needed before the call to super.__init__(...)
        self.activate_count = 0
        super(ActivationRetryModem, self).__init__(
                home_network=modem_cdma.ModemCdma.CdmaNetwork(activated=False))
        self._num_activate_retries = num_activate_retries


    def _InitializeProperties(self):
        props = super(ActivationRetryModem, self)._InitializeProperties()

        # For the purposes of this test, introduce a property that
        # stores how many times "Activate" has been called on this
        # modem.
        props[I_ACTIVATION_TEST] = {
            'ActivateCount' : dbus.types.UInt32(self.activate_count)
        }
        return props


    def _IncrementActivateCount(self):
        self.activate_count += 1
        self.Set(I_ACTIVATION_TEST,
                 'ActivateCount',
                 self.activate_count)


    @pm_utils.log_dbus_method(return_cb_arg='return_cb',
                              raise_cb_arg='raise_cb')
    def Activate(self, carrier, return_cb, raise_cb):
        """
        Activation will only succeed on the NUM_ACTIVATE_RETRIESth try.

        """
        self._IncrementActivateCount()
        if (self.activate_count == self._num_activate_retries):
            super(ActivationRetryModem, self).Activate(
                    carrier, return_cb, raise_cb)
        else:
            def _raise_activation_error():
                raise_cb(pm_errors.MMCdmaActivationError(
                         pm_errors.MMCdmaActivationError.START_FAILED))
            gobject.idle_add(_raise_activation_error)
