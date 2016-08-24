# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import network

from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.cros.cellular import cell_tools
from autotest_lib.client.cros.cellular import environment
from autotest_lib.client.cros.cellular import mm

import time

import flimflam


_STILL_REGISTERED_ERROR = error.TestError('modem registered to base station')
_NOT_REGISTERED_ERROR = error.TestError('modem not registered to base station')

CELLULAR_TIMEOUT = 180


class _WrongTech(Exception):
    def __init__(self, technology):
        self.technology = technology


class cellular_Signal(test.test):
    version = 1

    def TimedPollForCondition(
        self, label, condition, exception=None, timeout=10, sleep_interval=0.5):
        """Poll until a condition becomes true and report timing stats

        Arguments:
          label: label for a performance statistics to be logged
          condition: function taking no args and returning bool
          exception: exception to throw if condition doesn't become true
          timeout: maximum number of seconds to wait
          sleep_interval: time to sleep between polls
          desc: description of default TimeoutError used if 'exception' is None

        Returns:
          The true value that caused the poll loop to terminate.

        Raises:
          'exception' arg
        """
        start_time = time.time();
        utils.poll_for_condition(condition,
                                 timeout=timeout,
                                 exception=exception,
                                 sleep_interval=sleep_interval);
        self.write_perf_keyval({label: time.time() - start_time })

    def run_once(self, config, technologies, wait_for_disc, verify_set_power):

        # This test only works if all the technologies are in the same
        # family. Check that before doing anything else.
        families = set(
            cellular.TechnologyToFamily[tech] for tech in technologies)
        if len(families) > 1:
            raise error.TestError('Specify only one family not: %s' % families)

        # choose a technology other than the one we plan to start with
        technology = technologies[-1]
        with environment.DefaultCellularTestContext(config) as c:
            env = c.env
            flim = flimflam.FlimFlam()
            flim.SetDebugTags('manager+device+modem')
            env.StartDefault(technology)
            network.ResetAllModems(flim)
            logging.info('Preparing for %s' % technology)
            cell_tools.PrepareModemForTechnology('', technology)

            # TODO(jglasgow) Need to figure out what isn't settling here.
            # Going to wait 'til after ResetAllModems changes land.
            time.sleep(10)

            # Clear all errors before we start.
            # Resetting the modem above may have caused some errors on the
            # 8960 (eg. lost connection, etc).
            env.emulator.ClearErrors()

            service = env.CheckedConnectToCellular(timeout=CELLULAR_TIMEOUT)

            # Step through all technologies, forcing a transition
            failed_technologies = []
            manager, modem_path = mm.PickOneModem('')
            cell_modem = manager.GetModem(modem_path)
            for tech in technologies:
                tname = str(tech).replace('Technology:', '')
                if verify_set_power:
                    logging.info('Powering off basestation')
                    env.emulator.SetPower(cellular.Power.OFF)
                    self.TimedPollForCondition(
                        'Power.OFF.%s.deregister_time' % tname,
                        lambda: not cell_modem.ModemIsRegistered(),
                        timeout=CELLULAR_TIMEOUT,
                        exception=_STILL_REGISTERED_ERROR)

                    logging.info('Powering on basestation')
                    env.emulator.SetPower(cellular.Power.DEFAULT)
                    self.TimedPollForCondition(
                        'Power.DEFAULT.%s.register_time' % tname,
                        lambda: cell_modem.ModemIsRegistered(),
                        timeout=CELLULAR_TIMEOUT,
                        exception=_NOT_REGISTERED_ERROR)

                logging.info('Stopping basestation')
                env.emulator.Stop()
                if wait_for_disc:
                    self.TimedPollForCondition(
                        'Stop.%s.deregister_time' % tname,
                        lambda: not cell_modem.ModemIsRegistered(),
                        timeout=CELLULAR_TIMEOUT,
                        exception=_STILL_REGISTERED_ERROR)

                logging.info('Reconfiguring for %s' % tech)
                env.emulator.SetTechnology(tech)
                env.emulator.Start()

                try:
                    self.TimedPollForCondition(
                        'Start.%s.register_time' % tname,
                        lambda: cell_modem.ModemIsRegisteredUsing(tech),
                        timeout=CELLULAR_TIMEOUT,
                        exception=_WrongTech(tech))
                except _WrongTech, wt:
                    failed_technologies.append(
                        (wt.technology, cell_modem.GetAccessTechnology()))

                # TODO(jglasgow): verify flimflam properties (signals?)

        if failed_technologies:
            msg = ('Failed to register using %s' %
                   ', '.join(str(x) for x in failed_technologies))
            raise error.TestError(msg)
