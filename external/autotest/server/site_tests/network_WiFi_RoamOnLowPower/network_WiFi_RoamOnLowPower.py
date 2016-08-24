# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import multiprocessing
import re
import select
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_attenuator
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import rvr_test_base

class Reporter(object):
    """Object that forwards stdout from Host.run to a pipe.

    The |stdout_tee| parameter for Host.run() requires an object that looks
    like a Python built-in file.  In particular, it needs 'flush', which a
    multiprocessing.Connection (the object returned by multiprocessing.Pipe)
    doesn't have.  This wrapper provides that functionaly in order to allow a
    pipe to be the target of a stdout_tee.

    """

    def __init__(self, write_pipe):
        """Initializes reporter.

        @param write_pipe: the place to send output.

        """
        self._write_pipe = write_pipe


    def flush(self):
        """Flushes the output - not used by the pipe."""
        pass


    def close(self):
        """Closes the pipe."""
        return self._write_pipe.close()


    def fileno(self):
        """Returns the file number of the pipe."""
        return self._write_pipe.fileno()


    def write(self, string):
        """Write to the pipe.

        @param string: the string to write to the pipe.

        """
        self._write_pipe.send(string)


    def writelines(self, sequence):
        """Write a number of lines to the pipe.

        @param sequence: the array of lines to be written.

        """
        for string in sequence:
            self._write_pipe.send(string)


class LaunchIwEvent(object):
    """Calls 'iw event' and searches for a list of events in its output.

    This class provides a framework for launching 'iw event' in its own
    process and searching its output for an ordered list of events expressed
    as regular expressions.

    Expected to be called as follows:
        launch_iw_event = LaunchIwEvent('iw',
                                        self.context.client.host,
                                        timeout_seconds=60.0)
        # Do things that cause nl80211 traffic

        # Now, wait for the results you want.
        if not launch_iw_event.wait_for_events(['RSSI went below threshold',
                                                'scan started',
                                                # ...
                                                'connected to']):
            raise error.TestFail('Did not find all expected events')

    """
    # A timeout from Host.run(timeout) kills the process and that takes a
    # few seconds.  Therefore, we need to add some margin to the select
    # timeout (which will kill the process if Host.run(timeout) fails for some
    # reason).
    TIMEOUT_MARGIN_SECONDS = 5

    def __init__(self, iw_command, dut, timeout_seconds):
        """Launches 'iw event' process with communication channel for output

        @param dut: Host object for the dut
        @param timeout_seconds: timeout for 'iw event' (since it never
        returns)

        """
        self._iw_command = iw_command
        self._dut = dut
        self._timeout_seconds = timeout_seconds
        self._pipe_reader, pipe_writer = multiprocessing.Pipe()
        self._iw_event = multiprocessing.Process(target=self.do_iw,
                                                 args=(pipe_writer,
                                                       self._timeout_seconds,))
        self._iw_event.start()


    def do_iw(self, connection, timeout_seconds):
        """Runs 'iw event'

        iw results are passed back, on the fly, through a supplied connection
        object.  The process terminates itself after a specified timeout.

        @param connection: a Connection object to which results are written.
        @param timeout_seconds: number of seconds before 'iw event' is killed.

        """
        reporter = Reporter(connection)
        # ignore_timeout just ignores the _exception_; the timeout is still
        # valid.
        self._dut.run('%s event' % self._iw_command,
                      timeout=timeout_seconds,
                      stdout_tee=reporter,
                      ignore_timeout=True)


    def wait_for_events(self, expected_events):
        """Waits for 'expected_events' (in order) from iw.

        @param expected_events: a list of strings that are regular expressions.
            This method searches for the each expression, in the order that they
            appear in |expected_events|, in the stream of output from iw. x

        @returns: True if all events were found.  False, otherwise.

        """
        if not expected_events:
            logging.error('No events')
            return False

        expected_event = expected_events.pop(0)
        done_time = (time.time() + self._timeout_seconds +
                     LaunchIwEvent.TIMEOUT_MARGIN_SECONDS)
        received_event_log = []
        while expected_event:
            timeout = done_time - time.time()
            if timeout <= 0:
                break
            (sread, swrite, sexec) = select.select(
                    [self._pipe_reader], [], [], timeout)
            if sread:
                received_event = sread[0].recv()
                received_event_log.append(received_event)
                if re.search(expected_event, received_event):
                    logging.info('Found expected event: "%s"',
                                 received_event.rstrip())
                    if expected_events:
                        expected_event = expected_events.pop(0)
                    else:
                        expected_event = None
                        logging.info('Found ALL expected events')
                        break
            else:  # Timeout.
                break

        if expected_event:
            logging.error('Never found expected event "%s". iw log:',
                          expected_event)
            for event in received_event_log:
                logging.error(event.rstrip())
            return False
        return True


class network_WiFi_RoamOnLowPower(rvr_test_base.RvRTestBase):
    """Tests roaming to an AP when the old one's signal is too weak.

    This test uses a dual-radio Stumpy as the AP and configures the radios to
    broadcast two BSS's with different frequencies on the same SSID.  The DUT
    connects to the first radio, the test attenuates that radio, and the DUT
    is supposed to roam to the second radio.

    This test requires a particular configuration of test equipment:

                                   +--------- StumpyCell/AP ----------+
                                   | chromeX.grover.hostY.router.cros |
                                   |                                  |
                                   |       [Radio 0]  [Radio 1]       |
                                   +--------A-----B----C-----D--------+
        +------ BeagleBone ------+          |     |    |     |
        | chromeX.grover.hostY.  |          |     X    |     X
        | attenuator.cros      [Port0]-[attenuator]    |
        |                      [Port1]----- | ----[attenuator]
        |                      [Port2]-X    |          |
        |                      [Port3]-X    +-----+    |
        |                        |                |    |
        +------------------------+                |    |
                                   +--------------E----F--------------+
                                   |             [Radio 0]            |
                                   |                                  |
                                   |    chromeX.grover.hostY.cros     |
                                   +-------------- DUT ---------------+

    Where antennas A, C, and E are the primary antennas for AP/radio0,
    AP/radio1, and DUT/radio0, respectively; and antennas B, D, and F are the
    auxilliary antennas for AP/radio0, AP/radio1, and DUT/radio0,
    respectively.  The BeagleBone controls 2 attenuators that are connected
    to the primary antennas of AP/radio0 and 1 which are fed into the primary
    and auxilliary antenna ports of DUT/radio 0.  Ports 2 and 3 of the
    BeagleBone as well as the auxillary antennae of AP/radio0 and 1 are
    terminated.

    This arrangement ensures that the attenuator port numbers are assigned to
    the primary radio, first, and the secondary radio, second.  If this happens,
    the ports will be numbered in the order in which the AP's channels are
    configured (port 0 is first, port 1 is second, etc.).

    This test is a de facto test that the ports are configured in that
    arrangement since swapping Port0 and Port1 would cause us to attenuate the
    secondary radio, providing no impetus for the DUT to switch radios and
    causing the test to fail to connect at radio 1's frequency.

    """

    version = 1

    FREQUENCY_0 = 2412
    FREQUENCY_1 = 2462
    PORT_0 = 0  # Port created first (on FREQUENCY_0)
    PORT_1 = 1  # Port created second (on FREQUENCY_1)

    # Supplicant's signal to noise threshold for roaming.  When noise is
    # measurable and S/N is less than the threshold, supplicant will attempt
    # to roam.  We're setting the roam threshold (and setting it so high --
    # it's usually 18) because some of the DUTs we're using have a hard time
    # measuring signals below -55 dBm.  A threshold of 40 roams when the
    # signal is about -50 dBm (since the noise tends to be around -89).
    ABSOLUTE_ROAM_THRESHOLD_DB = 40


    def run_once(self):
        """Test body."""
        self.context.client.clear_supplicant_blacklist()

        with self.context.client.roam_threshold(
                self.ABSOLUTE_ROAM_THRESHOLD_DB):
            logging.info('- Configure first AP & connect')
            self.context.configure(hostap_config.HostapConfig(
                    frequency=self.FREQUENCY_0,
                    mode=hostap_config.HostapConfig.MODE_11G))
            router_ssid = self.context.router.get_ssid()
            self.context.assert_connect_wifi(xmlrpc_datatypes.
                                             AssociationParameters(
                    ssid=router_ssid))
            self.context.assert_ping_from_dut()

            # Setup background scan configuration to set a signal level, below
            # which, supplicant will scan (3dB below the current level).  We
            # must reconnect for these parameters to take effect.
            logging.info('- Set background scan level')
            bgscan_config = xmlrpc_datatypes.BgscanConfiguration(
                    method='simple',
                    signal=self.context.client.wifi_signal_level - 3)
            self.context.client.shill.disconnect(router_ssid)
            self.context.assert_connect_wifi(
                    xmlrpc_datatypes.AssociationParameters(
                    ssid=router_ssid, bgscan_config=bgscan_config))

            logging.info('- Configure second AP')
            self.context.configure(hostap_config.HostapConfig(
                    ssid=router_ssid,
                    frequency=self.FREQUENCY_1,
                    mode=hostap_config.HostapConfig.MODE_11G),
                                   multi_interface=True)

            launch_iw_event = LaunchIwEvent('iw',
                                            self.context.client.host,
                                            timeout_seconds=60.0)

            logging.info('- Drop the power on the first AP')

            self.set_signal_to_force_roam(port=self.PORT_0,
                                          frequency=self.FREQUENCY_0)

            # Verify that the low signal event is generated, that supplicant
            # scans as a result (or, at least, that supplicant scans after the
            # threshold is passed), and that it connects to something.
            logging.info('- Wait for RSSI threshold drop, scan, and connect')
            if not launch_iw_event.wait_for_events(['RSSI went below threshold',
                                                    'scan started',
                                                    'connected to']):
                raise error.TestFail('Did not find all expected events')

            logging.info('- Wait for a connection on the second AP')
            # Instead of explicitly connecting, just wait to see if the DUT
            # connects to the second AP by itself
            self.context.wait_for_connection(ssid=router_ssid,
                                             freq=self.FREQUENCY_1, ap_num=1)

            # Clean up.
            self.context.router.deconfig()


    def set_signal_to_force_roam(self, port, frequency):
        """Adjust the AP attenuation to force the DUT to roam.

        wpa_supplicant (v2.0-devel) decides when to roam based on a number of
        factors even when we're only interested in the scenario when the roam
        is instigated by an RSSI drop.  The gates for roaming differ between
        systems that have drivers that measure noise and those that don't.  If
        the driver reports noise, the S/N of both the current BSS and the
        target BSS is capped at 30 and then the following conditions must be
        met:

            1) The S/N of the current AP must be below supplicant's roam
               threshold.
            2) The S/N of the roam target must be more than 3dB larger than
               that of the current BSS.

        If the driver does not report noise, the following condition must be
        met:

            3) The roam target's signal must be above the current BSS's signal
               by a signal-dependent value (that value doesn't currently go
               higher than 5).

        This would all be enough complication.  Unfortunately, the DUT's signal
        measurement hardware has typically not been optimized for accurate
        measurement throughout the signal range.  Based on some testing
        (crbug:295752), it was discovered that the DUT's measurements of signal
        levels somewhere below -50dBm show values greater than the actual signal
        and with quite a bit of variance.  Since wpa_supplicant uses this same
        mechanism to read its levels, this code must iterate to find values that
        will reliably trigger supplicant to roam to the second AP.

        It was also shown that some MIMO DUTs send different signal levels to
        their two radios (testing has shown this to be somewhere around 5dB to
        7dB).

        @param port: the beaglebone port that is desired to be attenuated.
        @param frequency: noise needs to be read for a frequency.

        """
        # wpa_supplicant calls an S/N of 30 dB "quite good signal" and caps the
        # S/N at this level for the purposes of roaming calculations.  We'll do
        # the same (since we're trying to instigate behavior in supplicant).
        GREAT_SNR = 30

        # The difference between the S/Ns of APs from 2), above.
        MIN_AP_SIGNAL_DIFF_FOR_ROAM_DB = 3

        # The maximum delta for a system that doesn't measure noise, from 3),
        # above.
        MIN_NOISELESS_SIGNAL_DIFF_FOR_ROAM_DB = 5

        # Adds a clear margin to attenuator levels to make sure that we
        # attenuate enough to do the job in light of signal and noise levels
        # that bounce around.  This value was reached empirically and further
        # tweaking may be necessary if this test gets flaky.
        SIGNAL_TO_NOISE_MARGIN_DB = 3

        # The measured difference between the radios on one of our APs.
        # TODO(wdg): dynamically measure the difference between the AP's radios
        # (crbug:307678).
        TEST_HW_SIGNAL_DELTA_DB = 7

        # wpa_supplicant's roaming algorithm differs between systems that can
        # measure noise and those that can't.  This code tracks those
        # differences.
        actual_signal_dbm = self.context.client.wifi_signal_level
        actual_noise_dbm = self.context.client.wifi_noise_level(frequency)
        logging.info('Radio 0 signal: %r, noise: %r', actual_signal_dbm,
                     actual_noise_dbm)
        if actual_noise_dbm is not None:
            system_measures_noise = True
            actual_snr_db = actual_signal_dbm - actual_noise_dbm
            radio1_snr_db = actual_snr_db - TEST_HW_SIGNAL_DELTA_DB

            # Supplicant will cap any S/N measurement used for roaming at
            # GREAT_SNR so we'll do the same.
            if radio1_snr_db > GREAT_SNR:
                radio1_snr_db = GREAT_SNR

            # In order to roam, the S/N of radio 0 must be both less than 3db
            # below radio1 and less than the roam threshold.
            logging.info('Radio 1 S/N = %d', radio1_snr_db)
            delta_snr_threshold_db = (radio1_snr_db -
                                      MIN_AP_SIGNAL_DIFF_FOR_ROAM_DB)
            if (delta_snr_threshold_db < self.ABSOLUTE_ROAM_THRESHOLD_DB):
                target_snr_db = delta_snr_threshold_db
                logging.info('Target S/N = %d (delta algorithm)',
                             target_snr_db)
            else:
                target_snr_db = self.ABSOLUTE_ROAM_THRESHOLD_DB
                logging.info('Target S/N = %d (threshold algorithm)',
                             target_snr_db)

            # Add some margin.
            target_snr_db -= SIGNAL_TO_NOISE_MARGIN_DB
            attenuation_db = actual_snr_db - target_snr_db
            logging.info('Noise: target S/N=%d attenuation=%r',
                         target_snr_db, attenuation_db)
        else:
            system_measures_noise = False
            # On a system that doesn't measure noise, supplicant needs the
            # signal from radio 0 to be less than that of radio 1 minus a fixed
            # delta value.  While we're here, subtract additional margin from
            # the target value.
            target_signal_dbm = (actual_signal_dbm - TEST_HW_SIGNAL_DELTA_DB -
                                 MIN_NOISELESS_SIGNAL_DIFF_FOR_ROAM_DB -
                                 SIGNAL_TO_NOISE_MARGIN_DB)
            attenuation_db = actual_signal_dbm - target_signal_dbm
            logging.info('No noise: target_signal=%r, attenuation=%r',
                         target_signal_dbm, attenuation_db)

        # Attenuate, measure S/N, repeat (due to flaky measurments) until S/N is
        # where we want it.
        keep_tweaking_snr = True
        while keep_tweaking_snr:
            # Keep attenuation values below the attenuator's maximum.
            if attenuation_db > (site_attenuator.Attenuator.
                                 MAX_VARIABLE_ATTENUATION):
                attenuation_db = (site_attenuator.Attenuator.
                                  MAX_VARIABLE_ATTENUATION)
            logging.info('Applying attenuation=%r', attenuation_db)
            self.context.attenuator.set_variable_attenuation_on_port(
                    port, attenuation_db)
            if attenuation_db >= (site_attenuator.Attenuator.
                                    MAX_VARIABLE_ATTENUATION):
                logging.warning('. NOTICE: Attenuation is at maximum value')
                keep_tweaking_snr = False
            elif system_measures_noise:
                actual_snr_db = self.get_signal_to_noise(frequency)
                if actual_snr_db > target_snr_db:
                    logging.info('. S/N (%d) > target value (%d)',
                                 actual_snr_db, target_snr_db)
                    attenuation_db += actual_snr_db - target_snr_db
                else:
                    logging.info('. GOOD S/N=%r', actual_snr_db)
                    keep_tweaking_snr = False
            else:
                actual_signal_dbm = self.context.client.wifi_signal_level
                logging.info('. signal=%r', actual_signal_dbm)
                if actual_signal_dbm > target_signal_dbm:
                    logging.info('. Signal > target value (%d)',
                                 target_signal_dbm)
                    attenuation_db += actual_signal_dbm - target_signal_dbm
                else:
                    keep_tweaking_snr = False

        logging.info('Done')


    def get_signal_to_noise(self, frequency):
        """Gets both the signal and the noise on the current connection.

        @param frequency: noise needs to be read for a frequency.
        @returns: signal and noise in dBm

        """
        ping_ip = self.context.get_wifi_addr(ap_num=0)
        ping_config = ping_runner.PingConfig(target_ip=ping_ip, count=1,
                                             ignore_status=True,
                                             ignore_result=True)
        self.context.client.ping(ping_config)  # Just to provide traffic.
        signal_dbm = self.context.client.wifi_signal_level
        noise_dbm = self.context.client.wifi_noise_level(frequency)
        print '. signal: %r, noise: %r' % (signal_dbm, noise_dbm)
        if noise_dbm is None:
            return None
        return signal_dbm - noise_dbm
