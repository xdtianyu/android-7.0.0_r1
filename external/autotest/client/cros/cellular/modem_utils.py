# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm


def ClearGobiModemFaultInjection():
    """If a Gobi modem is present, try to clear its fault-injection state."""
    try:
        modem_manager, modem_path = mm.PickOneModem('Gobi')
    except error.TestError:
        # Did not find a Gobi modem. Simply return.
        return

    modem = modem_manager.GetModem(modem_path).GobiModem()
    if modem:
        modem.InjectFault('ClearFaults', 1)
