# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections, logging, numpy, os, tempfile, time
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.client.cros import backchannel, httpd
from autotest_lib.client.cros import memory_bandwidth_logger
from autotest_lib.client.cros import power_rapl, power_status, power_utils
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.networking import wifi_proxy

params_dict = {
    'test_time_ms': '_mseconds',
    'should_scroll': '_should_scroll',
    'should_scroll_up': '_should_scroll_up',
    'scroll_loop': '_scroll_loop',
    'scroll_interval_ms': '_scroll_interval_ms',
    'scroll_by_pixels': '_scroll_by_pixels',
    'tasks': '_tasks',
}


class power_LoadTest(test.test):
    """test class"""
    version = 2
    _username = 'powerloadtest@gmail.com'
    _pltp_url = 'https://sites.google.com/a/chromium.org/dev/chromium-os' \
                '/testing/power-testing/pltp/pltp'


    def initialize(self, percent_initial_charge_min=None,
                 check_network=True, loop_time=3600, loop_count=1,
                 should_scroll='true', should_scroll_up='true',
                 scroll_loop='false', scroll_interval_ms='10000',
                 scroll_by_pixels='600', test_low_batt_p=3,
                 verbose=True, force_wifi=False, wifi_ap='', wifi_sec='none',
                 wifi_pw='', wifi_timeout=60, tasks='', kblight_percent=10,
                 volume_level=10, mic_gain=10, low_batt_margin_p=2,
                 ac_ok=False, log_mem_bandwidth=False):
        """
        percent_initial_charge_min: min battery charge at start of test
        check_network: check that Ethernet interface is not running
        loop_time: length of time to run the test for in each loop
        loop_count: number of times to loop the test for
        should_scroll: should the extension scroll pages
        should_scroll_up: should scroll in up direction
        scroll_loop: continue scrolling indefinitely
        scroll_interval_ms: how often to scoll
        scroll_by_pixels: number of pixels to scroll each time
        test_low_batt_p: percent battery at which test should stop
        verbose: add more logging information
        force_wifi: should we force to test to run on wifi
        wifi_ap: the name (ssid) of the wifi access point
        wifi_sec: the type of security for the wifi ap
        wifi_pw: password for the wifi ap
        wifi_timeout: The timeout for wifi configuration
        kblight_percent: percent brightness of keyboard backlight
        volume_level: percent audio volume level
        mic_gain: percent audio microphone gain level
        low_batt_margin_p: percent low battery margin to be added to
            sys_low_batt_p to guarantee test completes prior to powerd shutdown
        ac_ok: boolean to allow running on AC
        log_mem_bandwidth: boolean to log memory bandwidth during the test
        """
        self._backlight = None
        self._services = None
        self._browser = None
        self._loop_time = loop_time
        self._loop_count = loop_count
        self._mseconds = self._loop_time * 1000
        self._verbose = verbose

        self._sys_low_batt_p = 0.
        self._sys_low_batt_s = 0.
        self._test_low_batt_p = test_low_batt_p
        self._should_scroll = should_scroll
        self._should_scroll_up = should_scroll_up
        self._scroll_loop = scroll_loop
        self._scroll_interval_ms = scroll_interval_ms
        self._scroll_by_pixels = scroll_by_pixels
        self._tmp_keyvals = {}
        self._power_status = power_status.get_status()
        self._tmp_keyvals['b_on_ac'] = self._power_status.on_ac()
        self._force_wifi = force_wifi
        self._testServer = None
        self._tasks = tasks.replace(' ','')
        self._backchannel = None
        self._shill_proxy = None
        self._kblight_percent = kblight_percent
        self._volume_level = volume_level
        self._mic_gain = mic_gain
        self._ac_ok = ac_ok
        self._log_mem_bandwidth = log_mem_bandwidth
        self._wait_time = 60
        self._stats = collections.defaultdict(list)

        with tempfile.NamedTemporaryFile() as pltp:
            file_utils.download_file(self._pltp_url, pltp.name)
            self._password = pltp.read().rstrip()

        if not ac_ok:
            self._power_status.assert_battery_state(percent_initial_charge_min)
        # If force wifi enabled, convert eth0 to backchannel and connect to the
        # specified WiFi AP.
        if self._force_wifi:
            sec_config = None
            # TODO(dbasehore): Fix this when we get a better way of figuring out
            # the wifi security configuration.
            if wifi_sec == 'rsn' or wifi_sec == 'wpa':
                sec_config = xmlrpc_security_types.WPAConfig(
                        psk=wifi_pw,
                        wpa_mode=xmlrpc_security_types.WPAConfig.MODE_PURE_WPA2,
                        wpa2_ciphers=
                                [xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
            wifi_config = xmlrpc_datatypes.AssociationParameters(
                    ssid=wifi_ap, security_config=sec_config,
                    configuration_timeout=wifi_timeout)
            # If backchannel is already running, don't run it again.
            self._backchannel = backchannel.Backchannel()
            if not self._backchannel.setup():
                raise error.TestError('Could not setup Backchannel network.')

            self._shill_proxy = wifi_proxy.WifiProxy()
            self._shill_proxy.remove_all_wifi_entries()
            for i in xrange(1,4):
                raw_output = self._shill_proxy.connect_to_wifi_network(
                        wifi_config.ssid,
                        wifi_config.security,
                        wifi_config.security_parameters,
                        wifi_config.save_credentials,
                        station_type=wifi_config.station_type,
                        hidden_network=wifi_config.is_hidden,
                        discovery_timeout_seconds=
                                wifi_config.discovery_timeout,
                        association_timeout_seconds=
                                wifi_config.association_timeout,
                        configuration_timeout_seconds=
                                wifi_config.configuration_timeout * i)
                result = xmlrpc_datatypes.AssociationResult. \
                        from_dbus_proxy_output(raw_output)
                if result.success:
                    break
                logging.warn('wifi connect: disc:%d assoc:%d config:%d fail:%s',
                             result.discovery_time, result.association_time,
                             result.configuration_time, result.failure_reason)
            else:
                raise error.TestError('Could not connect to WiFi network.')

        else:
            # Find all wired ethernet interfaces.
            # TODO: combine this with code in network_DisableInterface, in a
            # common library somewhere.
            ifaces = [ nic.strip() for nic in os.listdir('/sys/class/net/')
                if ((not os.path.exists('/sys/class/net/' + nic + '/phy80211'))
                    and nic.find('eth') != -1) ]
            logging.debug(str(ifaces))
            for iface in ifaces:
                if check_network and self._is_network_iface_running(iface):
                    raise error.TestError('Ethernet interface is active. ' +
                                          'Please remove Ethernet cable')

        # record the max backlight level
        self._backlight = power_utils.Backlight()
        self._tmp_keyvals['level_backlight_max'] = \
            self._backlight.get_max_level()

        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services.stop_services()

        # fix up file perms for the power test extension so that chrome
        # can access it
        os.system('chmod -R 755 %s' % self.bindir)

        # setup a HTTP Server to listen for status updates from the power
        # test extension
        self._testServer = httpd.HTTPListener(8001, docroot=self.bindir)
        self._testServer.run()

        # initialize various interesting power related stats
        self._statomatic = power_status.StatoMatic()

        self._power_status.refresh()
        (self._sys_low_batt_p, self._sys_low_batt_s) = \
            self._get_sys_low_batt_values()
        min_low_batt_p = min(self._sys_low_batt_p + low_batt_margin_p, 100)
        if self._sys_low_batt_p and (min_low_batt_p > self._test_low_batt_p):
            logging.warning("test low battery threshold is below system " +
                         "low battery requirement.  Setting to %f",
                         min_low_batt_p)
            self._test_low_batt_p = min_low_batt_p

        self._ah_charge_start = self._power_status.battery[0].charge_now
        self._wh_energy_start = self._power_status.battery[0].energy

    def run_once(self):
        t0 = time.time()

        # record the PSR counter
        psr_t0 = self._get_psr_counter()

        try:
            kblight = power_utils.KbdBacklight()
            kblight.set(self._kblight_percent)
            self._tmp_keyvals['percent_kbd_backlight'] = kblight.get()
        except power_utils.KbdBacklightException as e:
            logging.info("Assuming no keyboard backlight due to :: %s", str(e))
            kblight = None

        measurements = \
            [power_status.SystemPower(self._power_status.battery_path)]
        if power_utils.has_rapl_support():
            measurements += power_rapl.create_rapl()
        self._plog = power_status.PowerLogger(measurements, seconds_period=20)
        self._tlog = power_status.TempLogger([], seconds_period=20)
        self._plog.start()
        self._tlog.start()
        if self._log_mem_bandwidth:
            self._mlog = memory_bandwidth_logger.MemoryBandwidthLogger(
                raw=False, seconds_period=2)
            self._mlog.start()

        ext_path = os.path.join(os.path.dirname(__file__), 'extension')
        self._browser = chrome.Chrome(extension_paths=[ext_path],
                                gaia_login=True,
                                username=self._username,
                                password=self._password)
        extension = self._browser.get_extension(ext_path)
        for k in params_dict:
            if getattr(self, params_dict[k]) is not '':
                extension.ExecuteJavaScript('var %s = %s;' %
                                            (k, getattr(self, params_dict[k])))

        # This opens a trap start page to capture tabs opened for first login.
        # It will be closed when startTest is run.
        extension.ExecuteJavaScript('chrome.windows.create(null, null);')

        for i in range(self._loop_count):
            start_time = time.time()
            extension.ExecuteJavaScript('startTest();')
            # the power test extension will report its status here
            latch = self._testServer.add_wait_url('/status')

            # reset backlight level since powerd might've modified it
            # based on ambient light
            self._set_backlight_level()
            self._set_lightbar_level()
            if kblight:
                kblight.set(self._kblight_percent)
            audio_helper.set_volume_levels(self._volume_level,
                                           self._mic_gain)

            low_battery = self._do_wait(self._verbose, self._loop_time,
                                        latch)

            self._plog.checkpoint('loop%d' % (i), start_time)
            self._tlog.checkpoint('loop%d' % (i), start_time)
            if self._verbose:
                logging.debug('loop %d completed', i)

            if low_battery:
                logging.info('Exiting due to low battery')
                break

        t1 = time.time()
        self._tmp_keyvals['minutes_battery_life_tested'] = (t1 - t0) / 60
        if psr_t0:
            self._tmp_keyvals['psr_residency'] = \
                (self._get_psr_counter() - psr_t0) / (10 * (t1 - t0))


    def postprocess_iteration(self):
        def _log_stats(prefix, stats):
            if not len(stats):
                return
            np = numpy.array(stats)
            logging.debug("%s samples: %d", prefix, len(np))
            logging.debug("%s mean:    %.2f", prefix, np.mean())
            logging.debug("%s stdev:   %.2f", prefix, np.std())
            logging.debug("%s max:     %.2f", prefix, np.max())
            logging.debug("%s min:     %.2f", prefix, np.min())


        def _log_per_loop_stats():
            samples_per_loop = self._loop_time / self._wait_time + 1
            for kname in self._stats:
                start_idx = 0
                loop = 1
                for end_idx in xrange(samples_per_loop, len(self._stats[kname]),
                                      samples_per_loop):
                    _log_stats("%s loop %d" % (kname, loop),
                               self._stats[kname][start_idx:end_idx])
                    loop += 1
                    start_idx = end_idx


        def _log_all_stats():
            for kname in self._stats:
                _log_stats(kname, self._stats[kname])


        keyvals = self._plog.calc()
        keyvals.update(self._tlog.calc())
        keyvals.update(self._statomatic.publish())

        if self._log_mem_bandwidth:
            self._mlog.stop()
            self._mlog.join()

        _log_all_stats()
        _log_per_loop_stats()

        # record battery stats
        keyvals['a_current_now'] = self._power_status.battery[0].current_now
        keyvals['ah_charge_full'] = self._power_status.battery[0].charge_full
        keyvals['ah_charge_full_design'] = \
                             self._power_status.battery[0].charge_full_design
        keyvals['ah_charge_start'] = self._ah_charge_start
        keyvals['ah_charge_now'] = self._power_status.battery[0].charge_now
        keyvals['ah_charge_used'] = keyvals['ah_charge_start'] - \
                                    keyvals['ah_charge_now']
        keyvals['wh_energy_start'] = self._wh_energy_start
        keyvals['wh_energy_now'] = self._power_status.battery[0].energy
        keyvals['wh_energy_used'] = keyvals['wh_energy_start'] - \
                                    keyvals['wh_energy_now']
        keyvals['v_voltage_min_design'] = \
                             self._power_status.battery[0].voltage_min_design
        keyvals['wh_energy_full_design'] = \
                             self._power_status.battery[0].energy_full_design
        keyvals['v_voltage_now'] = self._power_status.battery[0].voltage_now

        keyvals.update(self._tmp_keyvals)

        keyvals['percent_sys_low_battery'] = self._sys_low_batt_p
        keyvals['seconds_sys_low_battery'] = self._sys_low_batt_s
        voltage_np = numpy.array(self._stats['v_voltage_now'])
        voltage_mean = voltage_np.mean()
        keyvals['v_voltage_mean'] = voltage_mean

        keyvals['wh_energy_powerlogger'] = \
                             self._energy_use_from_powerlogger(keyvals)

        if keyvals['ah_charge_used'] > 0:
            # For full runs, we should use charge to scale for battery life,
            # since the voltage swing is accounted for.
            # For short runs, energy will be a better estimate.
            if self._loop_count > 1:
                estimated_reps = (keyvals['ah_charge_full_design'] /
                                  keyvals['ah_charge_used'])
            else:
                estimated_reps = (keyvals['wh_energy_full_design'] /
                                  keyvals['wh_energy_powerlogger'])

            bat_life_scale =  estimated_reps * \
                              ((100 - keyvals['percent_sys_low_battery']) / 100)

            keyvals['minutes_battery_life'] = bat_life_scale * \
                keyvals['minutes_battery_life_tested']
            # In the case where sys_low_batt_s is non-zero subtract those
            # minutes from the final extrapolation.
            if self._sys_low_batt_s:
                keyvals['minutes_battery_life'] -= self._sys_low_batt_s / 60

            keyvals['a_current_rate'] = keyvals['ah_charge_used'] * 60 / \
                                        keyvals['minutes_battery_life_tested']
            keyvals['w_energy_rate'] = keyvals['wh_energy_used'] * 60 / \
                                       keyvals['minutes_battery_life_tested']
            self.output_perf_value(description='minutes_battery_life',
                                   value=keyvals['minutes_battery_life'],
                                   units='minutes')

        self.write_perf_keyval(keyvals)
        self._plog.save_results(self.resultsdir)
        self._tlog.save_results(self.resultsdir)


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        if self._services:
            self._services.restore_services()

        # cleanup backchannel interface
        # Prevent wifi congestion in test lab by forcing machines to forget the
        # wifi AP we connected to at the start of the test.
        if self._shill_proxy:
            self._shill_proxy.remove_all_wifi_entries()
        if self._backchannel:
            self._backchannel.teardown()
        if self._browser:
            self._browser.close()
        if self._testServer:
            self._testServer.stop()


    def _do_wait(self, verbose, seconds, latch):
        latched = False
        low_battery = False
        total_time = seconds + self._wait_time
        elapsed_time = 0

        while elapsed_time < total_time:
            time.sleep(self._wait_time)
            elapsed_time += self._wait_time

            self._power_status.refresh()
            charge_now = self._power_status.battery[0].charge_now
            energy_rate = self._power_status.battery[0].energy_rate
            voltage_now = self._power_status.battery[0].voltage_now
            self._stats['w_energy_rate'].append(energy_rate)
            self._stats['v_voltage_now'].append(voltage_now)
            if verbose:
                logging.debug('ah_charge_now %f', charge_now)
                logging.debug('w_energy_rate %f', energy_rate)
                logging.debug('v_voltage_now %f', voltage_now)

            low_battery = (self._power_status.percent_current_charge() <
                           self._test_low_batt_p)

            latched = latch.is_set()

            if latched or low_battery:
                break

        if latched:
            # record chrome power extension stats
            form_data = self._testServer.get_form_entries()
            logging.debug(form_data)
            for e in form_data:
                key = 'ext_' + e
                if key in self._tmp_keyvals:
                    self._tmp_keyvals[key] += "_%s" % form_data[e]
                else:
                    self._tmp_keyvals[key] = form_data[e]
        else:
            logging.debug("Didn't get status back from power extension")

        return low_battery


    def _set_backlight_level(self):
        self._backlight.set_default()
        # record brightness level
        self._tmp_keyvals['level_backlight_current'] = \
            self._backlight.get_level()


    def _set_lightbar_level(self, level='off'):
        """Set lightbar level.

        Args:
          level: string value to set lightbar to.  See ectool for more details.
        """
        rv = utils.system('which ectool', ignore_status=True)
        if rv:
            return
        rv = utils.system('ectool lightbar %s' % level, ignore_status=True)
        if rv:
            logging.info('Assuming no lightbar due to non-zero exit status')
        else:
            logging.info('Setting lightbar to %s', level)
            self._tmp_keyvals['level_lightbar_current'] = level


    def _get_sys_low_batt_values(self):
        """Determine the low battery values for device and return.

        2012/11/01: power manager (powerd.cc) parses parameters in filesystem
          and outputs a log message like:

           [1101/173837:INFO:powerd.cc(258)] Using low battery time threshold
                     of 0 secs and using low battery percent threshold of 3.5

           It currently checks to make sure that only one of these values is
           defined.

        Returns:
          Tuple of (percent, seconds)
            percent: float of low battery percentage
            seconds: float of low battery seconds

        """
        split_re = 'threshold of'

        powerd_log = '/var/log/power_manager/powerd.LATEST'
        cmd = 'grep "low battery time" %s' % powerd_log
        line = utils.system_output(cmd)
        secs = float(line.split(split_re)[1].split()[0])
        percent = float(line.split(split_re)[2].split()[0])
        if secs and percent:
            raise error.TestError("Low battery percent and seconds " +
                                  "are non-zero.")
        return (percent, secs)


    def _get_psr_counter(self):
        """Get the current value of the system PSR counter.
        This counts the number of milliseconds the system has resided in PSR.

        Returns:
          count: amount of time PSR has been active since boot in ms, or
              None if the performance counter can't be read

        """
        psr_status_file = '/sys/kernel/debug/dri/0/i915_edp_psr_status'
        try:
            count = utils.get_field(utils.read_file(psr_status_file),
                                    0,
                                    linestart='Performance_Counter:')
        except IOError:
            logging.info("Can't find or read PSR status file")
            return None

        logging.debug("PSR performance counter: %s", count)
        return int(count) if count else None


    def _is_network_iface_running(self, name):
        """
        Checks to see if the interface is running.

        Args:
          name: name of the interface to check.

        Returns:
          True if the interface is running.

        """
        try:
            # TODO: Switch to 'ip' (crbug.com/410601).
            out = utils.system_output('ifconfig %s' % name)
        except error.CmdError, e:
            logging.info(e)
            return False

        return out.find('RUNNING') >= 0


    def _energy_use_from_powerlogger(self, keyval):
        """
        Calculates the energy use, in Wh, used over the course of the run as
        reported by the PowerLogger.

        Args:
          keyval: the dictionary of keyvals containing PowerLogger output

        Returns:
          energy_wh: total energy used over the course of this run

        """
        energy_wh = 0
        loop = 0
        while True:
            duration_key = 'loop%d_system_duration' % loop
            avg_power_key = 'loop%d_system_pwr' % loop
            if duration_key not in keyval or avg_power_key not in keyval:
                break
            energy_wh += keyval[duration_key] * keyval[avg_power_key] / 3600
            loop += 1
        return energy_wh
