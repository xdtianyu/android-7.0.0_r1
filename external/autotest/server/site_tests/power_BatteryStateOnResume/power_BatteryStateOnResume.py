# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json, re, time

from autotest_lib.client.common_lib import error
from autotest_lib.server import hosts
from autotest_lib.server import test


_TIME_TO_SUSPEND = 10
_EXTRA_DELAY = 10


class power_BatteryStateOnResume(test.test):
    """
    Test to verify the battery charge state of the DUT on resume after the AC
    charger gets unplugged and plugged in its suspend state.

    If DUT is connected to RPM(default) - No need to pass any command line args.
    If DUT is connected to USB powerstrip(via servo), Need to pass cmdlineargs
    as --args=power_control="servoj10".
    If DUT is not connected to servo and RPM. i.e to handle manually, Need to
    pass cmdlineargs as --args=power_control="manual".
    """
    version = 1

    def initialize(self, host, client_ip, cmdline_args):
        self._client = hosts.create_host(client_ip)
        self.ensure_battery_present()

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
        if not self.loggedin(host):
            raise error.TestError("Not logged in!")
        self.unplug_ac_on_suspend(host)
        self.test_discharge_state(host)
        self.plug_ac_on_suspend(host)
        self.test_charge_state(host)


    def unplug_ac_on_suspend(self, host):
        """Unplugs AC when device in Suspend state."""
        host.servo.lid_close()
        time.sleep(_TIME_TO_SUSPEND + _EXTRA_DELAY)
        host.power_off(self.power_control)
        host.servo.lid_open()
        time.sleep(_EXTRA_DELAY)


    def plug_ac_on_suspend(self, host):
        """Plugs AC when device in Suspend state."""
        host.servo.lid_close()
        time.sleep(_TIME_TO_SUSPEND + _EXTRA_DELAY)
        host.power_on(self.power_control)
        host.servo.lid_open()
        time.sleep(_EXTRA_DELAY)


    def ensure_battery_present(self):
        """Ensure we have battery exists in DUT."""
        result = self._client.run('power_supply_info | egrep present')
        if 'yes' not in result.stdout:
            raise error.TestError('Find no batteries')


    def loggedin(self, host):
        """
        Checks if the host has a logged in user.

        @return True if a user is logged in on the device.
        """
        try:
            cmd_out = host.run('cryptohome --action=status').stdout.strip()
        except:
            return False
        status = json.loads(cmd_out)
        return any((mount['mounted'] for mount in status['mounts']))


    def test_charge_state(self, host):
        """Tests whether battery is in 'Charging/Charged' state."""
        bat_state = self.get_bat_status(host)
        if not (bat_state == 'Charging' or bat_state == 'Fully charged'):
            raise error.TestFail('Not Charging.  BAT state is %s.' % bat_state)


    def test_discharge_state(self, host):
        """Tests whether battery is in 'Discharging' state."""
        bat_state = self.get_bat_status(host)
        if not bat_state == 'Discharging':
            raise error.TestFail(
                'Not Discharging.  BAT state is %s.' % bat_state)


    def get_bat_status(self, host):
        """Returns the battery state per the 'power_supply_info' tool.

        @return battery power 'state' value. (i.e, Charging/Discharging ..)
        """
        bat_state_info = host.run(
            'power_supply_info | egrep state').stdout.strip()
        split_list = bat_state_info.split(":")
        return split_list[-1].strip()
