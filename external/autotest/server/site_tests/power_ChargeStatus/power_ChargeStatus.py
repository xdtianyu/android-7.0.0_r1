# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class power_ChargeStatus(test.test):
    """
    Test power_supply_info AC and BAT "state" on power OFF and ON.

    If DUT is connected to RPM(default) - No need to pass any command line args.
    If DUT is connected to USB powerstrip(via servo), Need to pass cmdlineargs
    as --args=power_control="servoj10".
    If DUT is not connected to servo and RPM. i.e to handle manually, Need to
    pass cmdlineargs as --args=power_control="manual".
    """
    version = 1

    def initialize(self, host, cmdline_args):
        args = {}
        for arg in cmdline_args:
            match = re.search("^(\w+)=(.+)", arg)
            if match:
                args[match.group(1)] = match.group(2)
        self.power_control = args.get('power_control', host.POWER_CONTROL_RPM)
        if self.power_control not in host.POWER_CONTROL_VALID_ARGS:
            raise error.TestError('Valid values for --args=power_control '
                                  'are %s. But you entered wrong argument '
                                  'as "%s".'
                                   % (host.POWER_CONTROL_VALID_ARGS,
                                   self.power_control))


    def run_once(self, host):
        ac_state = self.get_ac_status(host)
        bat_state = self.get_bat_status(host)
        self.test_charge_state(ac_state, bat_state)
        host.power_off(self.power_control)
        ac_state = self.get_ac_status(host)
        bat_state = self.get_bat_status(host)
        self.test_discharge_state(ac_state, bat_state)
        host.power_on(self.power_control)


    def test_charge_state(self, ac_state, bat_state):
        """Tests when on AC- the Main line is "ON" and "Charging/Charged".

        @param ac_state Specifies the power_supply_info "Line Power"
                        online value.
        @param bat_state Specifies the power_supply_info "Battery"
                         charge state value.
        """
        if not (ac_state == "yes" and (bat_state == "Charging"
            or bat_state == "Fully charged")):
            raise error.TestFail("AC is not online and BAT state is %s."
                                  % bat_state)


    def test_discharge_state(self, ac_state, bat_state):
        """Tests when on DC - the Main line is "No" and "Discharging".

        @param ac_state Specifies the power_supply_info "Line Power"
                        online value.
        @param bat_state Specifies the power_supply_info "Battery"
                         charge state value.
        """
        if not (ac_state == "no" and bat_state == "Discharging"):
            raise error.TestFail("Not Discharging, on AC and BAT state is %s."
                                  % bat_state)


    def get_ac_status(self, host):
        """Get the AC state info from "power_supply_info"."""
        ac_state_info = host.run(
            "power_supply_info | egrep 'online'").stdout.strip()
        return self.split_info(ac_state_info)


    def get_bat_status(self, host):
        """ Get the DC state info from "power_supply_info"."""
        bat_state_info = host.run(
            "power_supply_info | egrep 'state'").stdout.strip()
        return self.split_info(bat_state_info)


    def split_info(self, info_list):
        """Splits & trims stdout and returns the AC and BAT state Values.

        @param info_list Specifies the stdout value.
        """
        split_list = info_list.split(":")
        return split_list[-1].strip()
