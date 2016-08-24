# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections, ctypes, fcntl, glob, logging, math, numpy, os, re, struct
import threading, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error, enum
from autotest_lib.client.cros import kernel_trace

BatteryDataReportType = enum.Enum('CHARGE', 'ENERGY')

# battery data reported at 1e6 scale
BATTERY_DATA_SCALE = 1e6
# number of times to retry reading the battery in the case of bad data
BATTERY_RETRY_COUNT = 3

class DevStat(object):
    """
    Device power status. This class implements generic status initialization
    and parsing routines.
    """

    def __init__(self, fields, path=None):
        self.fields = fields
        self.path = path


    def reset_fields(self):
        """
        Reset all class fields to None to mark their status as unknown.
        """
        for field in self.fields.iterkeys():
            setattr(self, field, None)


    def read_val(self,  file_name, field_type):
        try:
            path = file_name
            if not file_name.startswith('/'):
                path = os.path.join(self.path, file_name)
            f = open(path, 'r')
            out = f.readline()
            val = field_type(out)
            return val

        except:
            return field_type(0)


    def read_all_vals(self):
        for field, prop in self.fields.iteritems():
            if prop[0]:
                val = self.read_val(prop[0], prop[1])
                setattr(self, field, val)


class ThermalStatACPI(DevStat):
    """
    ACPI-based thermal status.

    Fields:
    (All temperatures are in millidegrees Celsius.)

    str   enabled:            Whether thermal zone is enabled
    int   temp:               Current temperature
    str   type:               Thermal zone type
    int   num_trip_points:    Number of thermal trip points that activate
                                cooling devices
    int   num_points_tripped: Temperature is above this many trip points
    str   trip_point_N_type:  Trip point #N's type
    int   trip_point_N_temp:  Trip point #N's temperature value
    int   cdevX_trip_point:   Trip point o cooling device #X (index)
    """

    MAX_TRIP_POINTS = 20

    thermal_fields = {
        'enabled':              ['enabled', str],
        'temp':                 ['temp', int],
        'type':                 ['type', str],
        'num_points_tripped':   ['', '']
        }
    path = '/sys/class/thermal/thermal_zone*'

    def __init__(self, path=None):
        # Browse the thermal folder for trip point fields.
        self.num_trip_points = 0

        thermal_fields = glob.glob(path + '/*')
        for file in thermal_fields:
            field = file[len(path + '/'):]
            if field.find('trip_point') != -1:
                if field.find('temp'):
                    field_type = int
                else:
                    field_type = str
                self.thermal_fields[field] = [field, field_type]

                # Count the number of trip points.
                if field.find('_type') != -1:
                    self.num_trip_points += 1

        super(ThermalStatACPI, self).__init__(self.thermal_fields, path)
        self.update()

    def update(self):
        if not os.path.exists(self.path):
            return

        self.read_all_vals()
        self.num_points_tripped = 0

        for field in self.thermal_fields:
            if field.find('trip_point_') != -1 and field.find('_temp') != -1 \
                    and self.temp > self.read_val(field, int):
                self.num_points_tripped += 1
                logging.info('Temperature trip point #' + \
                            field[len('trip_point_'):field.rfind('_temp')] + \
                            ' tripped.')


class ThermalStatHwmon(DevStat):
    """
    hwmon-based thermal status.

    Fields:
    int   <tname>_temp<num>_input: Current temperature in millidegrees Celsius
      where:
          <tname> : name of hwmon device in sysfs
          <num>   : number of temp as some hwmon devices have multiple

    """
    path = '/sys/class/hwmon'

    thermal_fields = {}
    def __init__(self, rootpath=None):
        if not rootpath:
            rootpath = self.path
        for subpath1 in glob.glob('%s/hwmon*' % rootpath):
            for subpath2 in ['','device/']:
                gpaths = glob.glob("%s/%stemp*_input" % (subpath1, subpath2))
                for gpath in gpaths:
                    bname = os.path.basename(gpath)
                    field_path = os.path.join(subpath1, subpath2, bname)

                    tname_path = os.path.join(os.path.dirname(gpath), "name")
                    tname = utils.read_one_line(tname_path)

                    field_key = "%s_%s" % (tname, bname)
                    self.thermal_fields[field_key] = [field_path, int]

        super(ThermalStatHwmon, self).__init__(self.thermal_fields, rootpath)
        self.update()

    def update(self):
        if not os.path.exists(self.path):
            return

        self.read_all_vals()

    def read_val(self,  file_name, field_type):
        try:
            path = os.path.join(self.path, file_name)
            f = open(path, 'r')
            out = f.readline()
            return field_type(out)
        except:
            return field_type(0)


class ThermalStat(object):
    """helper class to instantiate various thermal devices."""
    def __init__(self):
        self._thermals = []
        self.min_temp = 999999999
        self.max_temp = -999999999

        thermal_stat_types = [(ThermalStatHwmon.path, ThermalStatHwmon),
                              (ThermalStatACPI.path, ThermalStatACPI)]
        for thermal_glob_path, thermal_type in thermal_stat_types:
            try:
                thermal_path = glob.glob(thermal_glob_path)[0]
                logging.debug('Using %s for thermal info.' % thermal_path)
                self._thermals.append(thermal_type(thermal_path))
            except:
                logging.debug('Could not find thermal path %s, skipping.' %
                              thermal_glob_path)


    def get_temps(self):
        """Get temperature readings.

        Returns:
            string of temperature readings.
        """
        temp_str = ''
        for thermal in self._thermals:
            thermal.update()
            for kname in thermal.fields:
                if kname is 'temp' or kname.endswith('_input'):
                    val = getattr(thermal, kname)
                    temp_str += '%s:%d ' % (kname, val)
                    if val > self.max_temp:
                        self.max_temp = val
                    if val < self.min_temp:
                        self.min_temp = val


        return temp_str


class BatteryStat(DevStat):
    """
    Battery status.

    Fields:

    float charge_full:        Last full capacity reached [Ah]
    float charge_full_design: Full capacity by design [Ah]
    float charge_now:         Remaining charge [Ah]
    float current_now:        Battery discharge rate [A]
    float energy:             Current battery charge [Wh]
    float energy_full:        Last full capacity reached [Wh]
    float energy_full_design: Full capacity by design [Wh]
    float energy_rate:        Battery discharge rate [W]
    float power_now:          Battery discharge rate [W]
    float remaining_time:     Remaining discharging time [h]
    float voltage_min_design: Minimum voltage by design [V]
    float voltage_max_design: Maximum voltage by design [V]
    float voltage_now:        Voltage now [V]
    """

    battery_fields = {
        'status':               ['status', str],
        'charge_full':          ['charge_full', float],
        'charge_full_design':   ['charge_full_design', float],
        'charge_now':           ['charge_now', float],
        'current_now':          ['current_now', float],
        'voltage_min_design':   ['voltage_min_design', float],
        'voltage_max_design':   ['voltage_max_design', float],
        'voltage_now':          ['voltage_now', float],
        'energy':               ['energy_now', float],
        'energy_full':          ['energy_full', float],
        'energy_full_design':   ['energy_full_design', float],
        'power_now':            ['power_now', float],
        'energy_rate':          ['', ''],
        'remaining_time':       ['', '']
        }

    def __init__(self, path=None):
        super(BatteryStat, self).__init__(self.battery_fields, path)
        self.update()


    def update(self):
        for _ in xrange(BATTERY_RETRY_COUNT):
            try:
                self._read_battery()
                return
            except error.TestError as e:
                logging.warn(e)
                continue
        raise error.TestError('Failed to read battery state')


    def _read_battery(self):
        self.read_all_vals()

        if self.charge_full == 0 and self.energy_full != 0:
            battery_type = BatteryDataReportType.ENERGY
        else:
            battery_type = BatteryDataReportType.CHARGE

        if self.voltage_min_design != 0:
            voltage_nominal = self.voltage_min_design
        else:
            voltage_nominal = self.voltage_now

        if voltage_nominal == 0:
            raise error.TestError('Failed to determine battery voltage')

        # Since charge data is present, calculate parameters based upon
        # reported charge data.
        if battery_type == BatteryDataReportType.CHARGE:
            self.charge_full = self.charge_full / BATTERY_DATA_SCALE
            self.charge_full_design = self.charge_full_design / \
                                      BATTERY_DATA_SCALE
            self.charge_now = self.charge_now / BATTERY_DATA_SCALE

            self.current_now = math.fabs(self.current_now) / \
                               BATTERY_DATA_SCALE

            self.energy =  voltage_nominal * \
                           self.charge_now / \
                           BATTERY_DATA_SCALE
            self.energy_full = voltage_nominal * \
                               self.charge_full / \
                               BATTERY_DATA_SCALE
            self.energy_full_design = voltage_nominal * \
                                      self.charge_full_design / \
                                      BATTERY_DATA_SCALE

        # Charge data not present, so calculate parameters based upon
        # reported energy data.
        elif battery_type == BatteryDataReportType.ENERGY:
            self.charge_full = self.energy_full / voltage_nominal
            self.charge_full_design = self.energy_full_design / \
                                      voltage_nominal
            self.charge_now = self.energy / voltage_nominal

            # TODO(shawnn): check if power_now can really be reported
            # as negative, in the same way current_now can
            self.current_now = math.fabs(self.power_now) / \
                               voltage_nominal

            self.energy = self.energy / BATTERY_DATA_SCALE
            self.energy_full = self.energy_full / BATTERY_DATA_SCALE
            self.energy_full_design = self.energy_full_design / \
                                      BATTERY_DATA_SCALE

        self.voltage_min_design = self.voltage_min_design / \
                                  BATTERY_DATA_SCALE
        self.voltage_max_design = self.voltage_max_design / \
                                  BATTERY_DATA_SCALE
        self.voltage_now = self.voltage_now / \
                           BATTERY_DATA_SCALE
        voltage_nominal = voltage_nominal / \
                          BATTERY_DATA_SCALE

        if self.charge_full > (self.charge_full_design * 1.5):
            raise error.TestError('Unreasonable charge_full value')
        if self.charge_now > (self.charge_full_design * 1.5):
            raise error.TestError('Unreasonable charge_now value')

        self.energy_rate =  self.voltage_now * self.current_now

        self.remaining_time = 0
        if self.current_now and self.energy_rate:
            self.remaining_time =  self.energy / self.energy_rate


class LineStatDummy(object):
    """
    Dummy line stat for devices which don't provide power_supply related sysfs
    interface.
    """
    def __init__(self):
        self.online = True


    def update(self):
        pass

class LineStat(DevStat):
    """
    Power line status.

    Fields:

    bool online:              Line power online
    """

    linepower_fields = {
        'is_online':             ['online', int]
        }


    def __init__(self, path=None):
        super(LineStat, self).__init__(self.linepower_fields, path)
        logging.debug("line path: %s", path)
        self.update()


    def update(self):
        self.read_all_vals()
        self.online = self.is_online == 1


class SysStat(object):
    """
    System power status for a given host.

    Fields:

    battery:   A list of BatteryStat objects.
    linepower: A list of LineStat objects.
    """
    psu_types = ['Mains', 'USB', 'USB_ACA', 'USB_C', 'USB_CDP', 'USB_DCP',
                 'USB_PD', 'USB_PD_DRP', 'Unknown']

    def __init__(self):
        power_supply_path = '/sys/class/power_supply/*'
        self.battery = None
        self.linepower = []
        self.thermal = None
        self.battery_path = None
        self.linepower_path = []

        power_supplies = glob.glob(power_supply_path)
        for path in power_supplies:
            type_path = os.path.join(path,'type')
            if not os.path.exists(type_path):
                continue
            power_type = utils.read_one_line(type_path)
            if power_type == 'Battery':
                self.battery_path = path
            elif power_type in self.psu_types:
                self.linepower_path.append(path)

        if not self.battery_path or not self.linepower_path:
            logging.warning("System does not provide power sysfs interface")

        self.thermal = ThermalStat()


    def refresh(self):
        """
        Initialize device power status objects.
        """
        self.linepower = []

        if self.battery_path:
            self.battery = [ BatteryStat(self.battery_path) ]

        for path in self.linepower_path:
            self.linepower.append(LineStat(path))
        if not self.linepower:
            self.linepower = [ LineStatDummy() ]

        temp_str = self.thermal.get_temps()
        if temp_str:
            logging.info('Temperature reading: ' + temp_str)
        else:
            logging.error('Could not read temperature, skipping.')


    def on_ac(self):
        """
        Returns true if device is currently running from AC power.
        """
        on_ac = False
        for linepower in self.linepower:
            on_ac |= linepower.online

        # Butterfly can incorrectly report AC online for some time after
        # unplug. Check battery discharge state to confirm.
        if utils.get_board() == 'butterfly':
            on_ac &= (not self.battery_discharging())
        return on_ac

    def battery_discharging(self):
        """
        Returns true if battery is currently discharging.
        """
        return(self.battery[0].status.rstrip() == 'Discharging')

    def percent_current_charge(self):
        return self.battery[0].charge_now * 100 / \
               self.battery[0].charge_full_design


    def assert_battery_state(self, percent_initial_charge_min):
        """Check initial power configuration state is battery.

        Args:
          percent_initial_charge_min: float between 0 -> 1.00 of
            percentage of battery that must be remaining.
            None|0|False means check not performed.

        Raises:
          TestError: if one of battery assertions fails
        """
        if self.on_ac():
            # TODO(shawnn): This is debug code. Need to remove it later.
            if utils.get_board() == 'butterfly':
                logging.debug('Butterfly on_ac, delay and re-check')
                tries = 0
                while self.on_ac():
                    logging.debug('Butterfly {on_ac, pcc, tries}: %d %d %d' %
                      (self.on_ac(), self.percent_current_charge(), tries))
                    tries += 1
                    if tries > 300:
                        logging.debug('on_ac never deasserted')
                        break
                    time.sleep(5)
                    self.refresh()

            raise error.TestError(
                'Running on AC power. Please remove AC power cable.')

        percent_initial_charge = self.percent_current_charge()

        if percent_initial_charge_min and percent_initial_charge < \
                                          percent_initial_charge_min:
            raise error.TestError('Initial charge (%f) less than min (%f)'
                      % (percent_initial_charge, percent_initial_charge_min))


def get_status():
    """
    Return a new power status object (SysStat). A new power status snapshot
    for a given host can be obtained by either calling this routine again and
    constructing a new SysStat object, or by using the refresh method of the
    SysStat object.
    """
    status = SysStat()
    status.refresh()
    return status


class AbstractStats(object):
    """
    Common superclass for measurements of percentages per state over time.

    Public Attributes:
        incremental:  If False, stats returned are from a single
        _read_stats.  Otherwise, stats are from the difference between
        the current and last refresh.
    """

    @staticmethod
    def to_percent(stats):
        """
        Turns a dict with absolute time values into a dict with percentages.
        """
        total = sum(stats.itervalues())
        if total == 0:
            return {}
        return dict((k, v * 100.0 / total) for (k, v) in stats.iteritems())


    @staticmethod
    def do_diff(new, old):
        """
        Returns a dict with value deltas from two dicts with matching keys.
        """
        return dict((k, new[k] - old.get(k, 0)) for k in new.iterkeys())


    @staticmethod
    def format_results_percent(results, name, percent_stats):
        """
        Formats autotest result keys to format:
          percent_<name>_<key>_time
        """
        for key in percent_stats:
            results['percent_%s_%s_time' % (name, key)] = percent_stats[key]


    @staticmethod
    def format_results_wavg(results, name, wavg):
        """
        Add an autotest result keys to format: wavg_<name>
        """
        if wavg is not None:
            results['wavg_%s' % (name)] = wavg


    def __init__(self, name=None, incremental=True):
        if not name:
            error.TestFail("Need to name AbstractStats instance please.")
        self.name = name
        self.incremental = incremental
        self._stats = self._read_stats()


    def refresh(self):
        """
        Returns dict mapping state names to percentage of time spent in them.
        """
        raw_stats = result = self._read_stats()
        if self.incremental:
            result = self.do_diff(result, self._stats)
        self._stats = raw_stats
        return self.to_percent(result)


    def _automatic_weighted_average(self):
        """
        Turns a dict with absolute times (or percentages) into a weighted
        average value.
        """
        total = sum(self._stats.itervalues())
        if total == 0:
            return None

        return sum((float(k)*v) / total for (k, v) in self._stats.iteritems())


    def _supports_automatic_weighted_average(self):
        """
        Override!

        Returns True if stats collected can be automatically converted from
        percent distribution to weighted average. False otherwise.
        """
        return False


    def weighted_average(self):
        """
        Return weighted average calculated using the automated average method
        (if supported) or using a custom method defined by the stat.
        """
        if self._supports_automatic_weighted_average():
            return self._automatic_weighted_average()

        return self._weighted_avg_fn()


    def _weighted_avg_fn(self):
        """
        Override! Custom weighted average function.

        Returns weighted average as a single floating point value.
        """
        return None


    def _read_stats(self):
        """
        Override! Reads the raw data values that shall be measured into a dict.
        """
        raise NotImplementedError('Override _read_stats in the subclass!')


class CPUFreqStats(AbstractStats):
    """
    CPU Frequency statistics
    """

    def __init__(self):
        cpufreq_stats_path = '/sys/devices/system/cpu/cpu*/cpufreq/stats/' + \
                             'time_in_state'
        intel_pstate_stats_path = '/sys/devices/system/cpu/intel_pstate/' + \
                             'aperf_mperf'
        self._file_paths = glob.glob(cpufreq_stats_path)
        self._intel_pstate_file_paths = glob.glob(intel_pstate_stats_path)
        self._running_intel_pstate = False
        self._initial_perf = None
        self._current_perf = None
        self._max_freq = 0

        if not self._file_paths:
            logging.debug('time_in_state file not found')
            if self._intel_pstate_file_paths:
                logging.debug('intel_pstate frequency stats file found')
                self._running_intel_pstate = True

        super(CPUFreqStats, self).__init__(name='cpufreq')


    def _read_stats(self):
        if self._running_intel_pstate:
            aperf = 0
            mperf = 0

            for path in self._intel_pstate_file_paths:
                if not os.path.exists(path):
                    logging.debug('%s is not found', path)
                    continue
                data = utils.read_file(path)
                for line in data.splitlines():
                    pair = line.split()
                    # max_freq is supposed to be the same for all CPUs
                    # and remain constant throughout.
                    # So, we set the entry only once
                    if not self._max_freq:
                        self._max_freq = int(pair[0])
                    aperf += int(pair[1])
                    mperf += int(pair[2])

            if not self._initial_perf:
                self._initial_perf = (aperf, mperf)

            self._current_perf = (aperf, mperf)

        stats = {}
        for path in self._file_paths:
            if not os.path.exists(path):
                logging.debug('%s is not found', path)
                continue

            data = utils.read_file(path)
            for line in data.splitlines():
                pair = line.split()
                freq = int(pair[0])
                timeunits = int(pair[1])
                if freq in stats:
                    stats[freq] += timeunits
                else:
                    stats[freq] = timeunits
        return stats


    def _supports_automatic_weighted_average(self):
        return not self._running_intel_pstate


    def _weighted_avg_fn(self):
        if not self._running_intel_pstate:
            return None

        if self._current_perf[1] != self._initial_perf[1]:
            # Avg freq = max_freq * aperf_delta / mperf_delta
            return self._max_freq * \
                float(self._current_perf[0] - self._initial_perf[0]) / \
                (self._current_perf[1] - self._initial_perf[1])
        return 1.0


class CPUIdleStats(AbstractStats):
    """
    CPU Idle statistics (refresh() will not work with incremental=False!)
    """
    # TODO (snanda): Handle changes in number of c-states due to events such
    # as ac <-> battery transitions.
    # TODO (snanda): Handle non-S0 states. Time spent in suspend states is
    # currently not factored out.
    def __init__(self):
        super(CPUIdleStats, self).__init__(name='cpuidle')


    def _read_stats(self):
        cpuidle_stats = collections.defaultdict(int)
        cpuidle_path = '/sys/devices/system/cpu/cpu*/cpuidle'
        epoch_usecs = int(time.time() * 1000 * 1000)
        cpus = glob.glob(cpuidle_path)

        for cpu in cpus:
            state_path = os.path.join(cpu, 'state*')
            states = glob.glob(state_path)
            cpuidle_stats['C0'] += epoch_usecs

            for state in states:
                name = utils.read_one_line(os.path.join(state, 'name'))
                latency = utils.read_one_line(os.path.join(state, 'latency'))

                if not int(latency) and name == 'POLL':
                    # C0 state. Kernel stats aren't right, so calculate by
                    # subtracting all other states from total time (using epoch
                    # timer since we calculate differences in the end anyway).
                    # NOTE: Only x86 lists C0 under cpuidle, ARM does not.
                    continue

                usecs = int(utils.read_one_line(os.path.join(state, 'time')))
                cpuidle_stats['C0'] -= usecs

                if name == '<null>':
                    # Kernel race condition that can happen while a new C-state
                    # gets added (e.g. AC->battery). Don't know the 'name' of
                    # the state yet, but its 'time' would be 0 anyway.
                    logging.warning('Read name: <null>, time: %d from %s'
                        % (usecs, state) + '... skipping.')
                    continue

                cpuidle_stats[name] += usecs

        return cpuidle_stats


class CPUPackageStats(AbstractStats):
    """
    Package C-state residency statistics for modern Intel CPUs.
    """

    ATOM         =              {'C2': 0x3F8, 'C4': 0x3F9, 'C6': 0x3FA}
    NEHALEM      =              {'C3': 0x3F8, 'C6': 0x3F9, 'C7': 0x3FA}
    SANDY_BRIDGE = {'C2': 0x60D, 'C3': 0x3F8, 'C6': 0x3F9, 'C7': 0x3FA}
    HASWELL      = {'C2': 0x60D, 'C3': 0x3F8, 'C6': 0x3F9, 'C7': 0x3FA,
                                 'C8': 0x630, 'C9': 0x631,'C10': 0x632}

    def __init__(self):
        def _get_platform_states():
            """
            Helper to decide what set of microarchitecture-specific MSRs to use.

            Returns: dict that maps C-state name to MSR address, or None.
            """
            modalias = '/sys/devices/system/cpu/modalias'
            if not os.path.exists(modalias):
                return None

            values = utils.read_one_line(modalias).split(':')
            # values[2]: vendor, values[4]: family, values[6]: model (CPUID)
            if values[2] != '0000' or values[4] != '0006':
                return None

            return {
                # model groups pulled from Intel manual, volume 3 chapter 35
                '0027': self.ATOM,         # unreleased? (Next Generation Atom)
                '001A': self.NEHALEM,      # Bloomfield, Nehalem-EP (i7/Xeon)
                '001E': self.NEHALEM,      # Clarks-/Lynnfield, Jasper (i5/i7/X)
                '001F': self.NEHALEM,      # unreleased? (abandoned?)
                '0025': self.NEHALEM,      # Arran-/Clarksdale (i3/i5/i7/C/X)
                '002C': self.NEHALEM,      # Gulftown, Westmere-EP (i7/Xeon)
                '002E': self.NEHALEM,      # Nehalem-EX (Xeon)
                '002F': self.NEHALEM,      # Westmere-EX (Xeon)
                '002A': self.SANDY_BRIDGE, # SandyBridge (i3/i5/i7/C/X)
                '002D': self.SANDY_BRIDGE, # SandyBridge-E (i7)
                '003A': self.SANDY_BRIDGE, # IvyBridge (i3/i5/i7/X)
                '003C': self.HASWELL,      # Haswell (Core/Xeon)
                '003D': self.HASWELL,      # Broadwell (Core)
                '003E': self.SANDY_BRIDGE, # IvyBridge (Xeon)
                '003F': self.HASWELL,      # Haswell-E (Core/Xeon)
                '004F': self.HASWELL,      # Broadwell (Xeon)
                '0056': self.HASWELL,      # Broadwell (Xeon D)
                }.get(values[6], None)

        self._platform_states = _get_platform_states()
        super(CPUPackageStats, self).__init__(name='cpupkg')


    def _read_stats(self):
        packages = set()
        template = '/sys/devices/system/cpu/cpu%s/topology/physical_package_id'
        if not self._platform_states:
            return {}
        stats = dict((state, 0) for state in self._platform_states)
        stats['C0_C1'] = 0

        for cpu in os.listdir('/dev/cpu'):
            if not os.path.exists(template % cpu):
                continue
            package = utils.read_one_line(template % cpu)
            if package in packages:
                continue
            packages.add(package)

            stats['C0_C1'] += utils.rdmsr(0x10, cpu) # TSC
            for (state, msr) in self._platform_states.iteritems():
                ticks = utils.rdmsr(msr, cpu)
                stats[state] += ticks
                stats['C0_C1'] -= ticks

        return stats


class GPUFreqStats(AbstractStats):
    """GPU Frequency statistics class.

    TODO(tbroch): add stats for other GPUs
    """

    _MALI_DEV = '/sys/class/misc/mali0/device'
    _MALI_EVENTS = ['mali_dvfs:mali_dvfs_set_clock']
    _MALI_34_TRACE_CLK_RE = r'(\d+.\d+): mali_dvfs_set_clock: frequency=(\d+)'
    _MALI_TRACE_CLK_RE = r'(\d+.\d+): mali_dvfs_set_clock: frequency=(\d+)\d{6}'

    _I915_ROOT = '/sys/kernel/debug/dri/0'
    _I915_EVENTS = ['i915:intel_gpu_freq_change']
    _I915_CLK = os.path.join(_I915_ROOT, 'i915_cur_delayinfo')
    _I915_TRACE_CLK_RE = r'(\d+.\d+): intel_gpu_freq_change: new_freq=(\d+)'
    _I915_CUR_FREQ_RE = r'CAGF:\s+(\d+)MHz'
    _I915_MIN_FREQ_RE = r'Lowest \(RPN\) frequency:\s+(\d+)MHz'
    _I915_MAX_FREQ_RE = r'Max non-overclocked \(RP0\) frequency:\s+(\d+)MHz'
    # TODO(dbasehore) parse this from debugfs if/when this value is added
    _I915_FREQ_STEP = 50

    _gpu_type = None


    def _get_mali_freqs(self):
        """Get mali clocks based on kernel version.

        For 3.4:
            # cat /sys/class/misc/mali0/device/clock
            Current sclk_g3d[G3D_BLK] = 100Mhz
            Possible settings : 533, 450, 400, 350, 266, 160, 100Mhz

        For 3.8 (and beyond):
            # cat /sys/class/misc/mali0/device/clock
            100000000
            # cat /sys/class/misc/mali0/device/available_frequencies
            100000000
            160000000
            266000000
            350000000
            400000000
            450000000
            533000000
            533000000

        Returns:
          cur_mhz: string of current GPU clock in mhz
        """
        cur_mhz = None
        fqs = []

        fname = os.path.join(self._MALI_DEV, 'clock')
        if os.uname()[2].startswith('3.4'):
            with open(fname) as fd:
                for ln in fd.readlines():
                    result = re.findall(r'Current.* = (\d+)Mhz', ln)
                    if result:
                        cur_mhz = result[0]
                        continue
                    result = re.findall(r'(\d+)[,M]', ln)
                    if result:
                        fqs = result
                        fd.close()
        else:
            cur_mhz = str(int(int(utils.read_one_line(fname).strip()) / 1e6))
            fname = os.path.join(self._MALI_DEV, 'available_frequencies')
            with open(fname) as fd:
                for ln in fd.readlines():
                    freq = int(int(ln.strip()) / 1e6)
                    fqs.append(str(freq))
                fqs.sort()

        self._freqs = fqs
        return cur_mhz


    def __init__(self, incremental=False):


        min_mhz = None
        max_mhz = None
        cur_mhz = None
        events = None
        self._freqs = []
        self._prev_sample = None
        self._trace = None

        if os.path.exists(self._MALI_DEV):
            self._set_gpu_type('mali')
        elif os.path.exists(self._I915_CLK):
            self._set_gpu_type('i915')

        logging.debug("gpu_type is %s", self._gpu_type)

        if self._gpu_type is 'mali':
            events = self._MALI_EVENTS
            cur_mhz = self._get_mali_freqs()
            if self._freqs:
                min_mhz = self._freqs[0]
                max_mhz = self._freqs[-1]

        elif self._gpu_type is 'i915':
            events = self._I915_EVENTS
            with open(self._I915_CLK) as fd:
                for ln in fd.readlines():
                    logging.debug("ln = %s", ln)
                    result = re.findall(self._I915_CUR_FREQ_RE, ln)
                    if result:
                        cur_mhz = result[0]
                        continue
                    result = re.findall(self._I915_MIN_FREQ_RE, ln)
                    if result:
                        min_mhz = result[0]
                        continue
                    result = re.findall(self._I915_MAX_FREQ_RE, ln)
                    if result:
                        max_mhz = result[0]
                        continue
                if min_mhz and max_mhz:
                    for i in xrange(int(min_mhz), int(max_mhz) +
                                    self._I915_FREQ_STEP, self._I915_FREQ_STEP):
                        self._freqs.append(str(i))

        logging.debug("cur_mhz = %s, min_mhz = %s, max_mhz = %s", cur_mhz,
                      min_mhz, max_mhz)

        if cur_mhz and min_mhz and max_mhz:
            self._trace = kernel_trace.KernelTrace(events=events)

        # Not all platforms or kernel versions support tracing.
        if not self._trace or not self._trace.is_tracing():
            logging.warning("GPU frequency tracing not enabled.")
        else:
            self._prev_sample = (cur_mhz, self._trace.uptime_secs())
            logging.debug("Current GPU freq: %s", cur_mhz)
            logging.debug("All GPU freqs: %s", self._freqs)

        super(GPUFreqStats, self).__init__(name='gpu', incremental=incremental)


    @classmethod
    def _set_gpu_type(cls, gpu_type):
        cls._gpu_type = gpu_type


    def _read_stats(self):
        if self._gpu_type:
            return getattr(self, "_%s_read_stats" % self._gpu_type)()
        return {}


    def _trace_read_stats(self, regexp):
        """Read GPU stats from kernel trace outputs.

        Args:
            regexp: regular expression to match trace output for frequency

        Returns:
            Dict with key string in mhz and val float in seconds.
        """
        if not self._prev_sample:
            return {}

        stats = dict((k, 0.0) for k in self._freqs)
        results = self._trace.read(regexp=regexp)
        for (tstamp_str, freq) in results:
            tstamp = float(tstamp_str)

            # do not reparse lines in trace buffer
            if tstamp <= self._prev_sample[1]:
                continue
            delta = tstamp - self._prev_sample[1]
            logging.debug("freq:%s tstamp:%f - %f delta:%f",
                          self._prev_sample[0],
                          tstamp, self._prev_sample[1],
                          delta)
            stats[self._prev_sample[0]] += delta
            self._prev_sample = (freq, tstamp)

        # Do last record
        delta = self._trace.uptime_secs() - self._prev_sample[1]
        logging.debug("freq:%s tstamp:uptime - %f delta:%f",
                      self._prev_sample[0],
                      self._prev_sample[1], delta)
        stats[self._prev_sample[0]] += delta

        logging.debug("GPU freq percents:%s", stats)
        return stats


    def _mali_read_stats(self):
        """Read Mali GPU stats

        For 3.4:
            Frequencies are reported in MHz.

        For 3.8+:
            Frequencies are reported in Hz, so use a regex that drops the last 6
            digits.

        Output in trace looks like this:

            kworker/u:24-5220  [000] .... 81060.329232: mali_dvfs_set_clock: frequency=400
            kworker/u:24-5220  [000] .... 81061.830128: mali_dvfs_set_clock: frequency=350

        Returns:
            Dict with frequency in mhz as key and float in seconds for time
              spent at that frequency.
        """
        regexp = None
        if os.uname()[2].startswith('3.4'):
            regexp = self._MALI_34_TRACE_CLK_RE
        else:
            regexp = self._MALI_TRACE_CLK_RE

        return self._trace_read_stats(regexp)


    def _i915_read_stats(self):
        """Read i915 GPU stats.

        Output looks like this (kernel >= 3.8):

          kworker/u:0-28247 [000] .... 259391.579610: intel_gpu_freq_change: new_freq=400
          kworker/u:0-28247 [000] .... 259391.581797: intel_gpu_freq_change: new_freq=350

        Returns:
            Dict with frequency in mhz as key and float in seconds for time
              spent at that frequency.
        """
        return self._trace_read_stats(self._I915_TRACE_CLK_RE)


class USBSuspendStats(AbstractStats):
    """
    USB active/suspend statistics (over all devices)
    """
    # TODO (snanda): handle hot (un)plugging of USB devices
    # TODO (snanda): handle duration counters wraparound

    def __init__(self):
        usb_stats_path = '/sys/bus/usb/devices/*/power'
        self._file_paths = glob.glob(usb_stats_path)
        if not self._file_paths:
            logging.debug('USB stats path not found')
        super(USBSuspendStats, self).__init__(name='usb')


    def _read_stats(self):
        usb_stats = {'active': 0, 'suspended': 0}

        for path in self._file_paths:
            active_duration_path = os.path.join(path, 'active_duration')
            total_duration_path = os.path.join(path, 'connected_duration')

            if not os.path.exists(active_duration_path) or \
               not os.path.exists(total_duration_path):
                logging.debug('duration paths do not exist for: %s', path)
                continue

            active = int(utils.read_file(active_duration_path))
            total = int(utils.read_file(total_duration_path))
            logging.debug('device %s active for %.2f%%',
                          path, active * 100.0 / total)

            usb_stats['active'] += active
            usb_stats['suspended'] += total - active

        return usb_stats


class StatoMatic(object):
    """Class to aggregate and monitor a bunch of power related statistics."""
    def __init__(self):
        self._start_uptime_secs = kernel_trace.KernelTrace.uptime_secs()
        self._astats = [USBSuspendStats(),
                        CPUFreqStats(),
                        GPUFreqStats(incremental=False),
                        CPUIdleStats(),
                        CPUPackageStats()]
        self._disk = DiskStateLogger()
        self._disk.start()


    def publish(self):
        """Publishes results of various statistics gathered.

        Returns:
            dict with
              key = string 'percent_<name>_<key>_time'
              value = float in percent
        """
        results = {}
        tot_secs = kernel_trace.KernelTrace.uptime_secs() - \
            self._start_uptime_secs
        for stat_obj in self._astats:
            percent_stats = stat_obj.refresh()
            logging.debug("pstats = %s", percent_stats)
            if stat_obj.name is 'gpu':
                # TODO(tbroch) remove this once GPU freq stats have proved
                # reliable
                stats_secs = sum(stat_obj._stats.itervalues())
                if stats_secs < (tot_secs * 0.9) or \
                        stats_secs > (tot_secs * 1.1):
                    logging.warning('%s stats dont look right.  Not publishing.',
                                 stat_obj.name)
                    continue
            new_res = {}
            AbstractStats.format_results_percent(new_res, stat_obj.name,
                                                 percent_stats)
            wavg = stat_obj.weighted_average()
            if wavg:
                AbstractStats.format_results_wavg(new_res, stat_obj.name, wavg)

            results.update(new_res)

        new_res = {}
        if self._disk.get_error():
            new_res['disk_logging_error'] = str(self._disk.get_error())
        else:
            AbstractStats.format_results_percent(new_res, 'disk',
                                                 self._disk.result())
        results.update(new_res)

        return results


class PowerMeasurement(object):
    """Class to measure power.

    Public attributes:
        domain: String name of the power domain being measured.  Example is
          'system' for total system power

    Public methods:
        refresh: Performs any power/energy sampling and calculation and returns
            power as float in watts.  This method MUST be implemented in
            subclass.
    """

    def __init__(self, domain):
        """Constructor."""
        self.domain = domain


    def refresh(self):
        """Performs any power/energy sampling and calculation.

        MUST be implemented in subclass

        Returns:
            float, power in watts.
        """
        raise NotImplementedError("'refresh' method should be implemented in "
                                  "subclass.")


def parse_power_supply_info():
    """Parses power_supply_info command output.

    Command output from power_manager ( tools/power_supply_info.cc ) looks like
    this:

        Device: Line Power
          path:               /sys/class/power_supply/cros_ec-charger
          ...
        Device: Battery
          path:               /sys/class/power_supply/sbs-9-000b
          ...

    """
    rv = collections.defaultdict(dict)
    dev = None
    for ln in utils.system_output('power_supply_info').splitlines():
        logging.debug("psu: %s", ln)
        result = re.findall(r'^Device:\s+(.*)', ln)
        if result:
            dev = result[0]
            continue
        result = re.findall(r'\s+(.+):\s+(.+)', ln)
        if result and dev:
            kname = re.findall(r'(.*)\s+\(\w+\)', result[0][0])
            if kname:
                rv[dev][kname[0]] = result[0][1]
            else:
                rv[dev][result[0][0]] = result[0][1]

    return rv


class SystemPower(PowerMeasurement):
    """Class to measure system power.

    TODO(tbroch): This class provides a subset of functionality in BatteryStat
    in hopes of minimizing power draw.  Investigate whether its really
    significant and if not, deprecate.

    Private Attributes:
      _voltage_file: path to retrieve voltage in uvolts
      _current_file: path to retrieve current in uamps
    """

    def __init__(self, battery_dir):
        """Constructor.

        Args:
            battery_dir: path to dir containing the files to probe and log.
                usually something like /sys/class/power_supply/BAT0/
        """
        super(SystemPower, self).__init__('system')
        # Files to log voltage and current from
        self._voltage_file = os.path.join(battery_dir, 'voltage_now')
        self._current_file = os.path.join(battery_dir, 'current_now')


    def refresh(self):
        """refresh method.

        See superclass PowerMeasurement for details.
        """
        keyvals = parse_power_supply_info()
        return float(keyvals['Battery']['energy rate'])


class MeasurementLogger(threading.Thread):
    """A thread that logs measurement readings.

    Example code snippet:
         mylogger = MeasurementLogger([Measurent1, Measurent2])
         mylogger.run()
         for testname in tests:
             start_time = time.time()
             #run the test method for testname
             mlogger.checkpoint(testname, start_time)
         keyvals = mylogger.calc()

    Public attributes:
        seconds_period: float, probing interval in seconds.
        readings: list of lists of floats of measurements.
        times: list of floats of time (since Epoch) of when measurements
            occurred.  len(time) == len(readings).
        done: flag to stop the logger.
        domains: list of  domain strings being measured

    Public methods:
        run: launches the thread to gather measuremnts
        calc: calculates
        save_results:

    Private attributes:
       _measurements: list of Measurement objects to be sampled.
       _checkpoint_data: list of tuples.  Tuple contains:
           tname: String of testname associated with this time interval
           tstart: Float of time when subtest started
           tend: Float of time when subtest ended
       _results: list of results tuples.  Tuple contains:
           prefix: String of subtest
           mean: Float of mean  in watts
           std: Float of standard deviation of measurements
           tstart: Float of time when subtest started
           tend: Float of time when subtest ended
    """
    def __init__(self, measurements, seconds_period=1.0):
        """Initialize a logger.

        Args:
            _measurements: list of Measurement objects to be sampled.
            seconds_period: float, probing interval in seconds.  Default 1.0
        """
        threading.Thread.__init__(self)

        self.seconds_period = seconds_period

        self.readings = []
        self.times = []
        self._checkpoint_data = []

        self.domains = []
        self._measurements = measurements
        for meas in self._measurements:
            self.domains.append(meas.domain)

        self.done = False


    def run(self):
        """Threads run method."""
        while(not self.done):
            readings = []
            for meas in self._measurements:
                readings.append(meas.refresh())
            # TODO (dbasehore): We probably need proper locking in this file
            # since there have been race conditions with modifying and accessing
            # data.
            self.readings.append(readings)
            self.times.append(time.time())
            time.sleep(self.seconds_period)


    def checkpoint(self, tname='', tstart=None, tend=None):
        """Check point the times in seconds associated with test tname.

        Args:
           tname: String of testname associated with this time interval
           tstart: Float in seconds of when tname test started.  Should be based
                off time.time()
           tend: Float in seconds of when tname test ended.  Should be based
                off time.time().  If None, then value computed in the method.
        """
        if not tstart and self.times:
            tstart = self.times[0]
        if not tend:
            tend = time.time()
        self._checkpoint_data.append((tname, tstart, tend))
        logging.info('Finished test "%s" between timestamps [%s, %s]',
                     tname, tstart, tend)


    def calc(self, mtype=None):
        """Calculate average measurement during each of the sub-tests.

        Method performs the following steps:
            1. Signals the thread to stop running.
            2. Calculates mean, max, min, count on the samples for each of the
               measurements.
            3. Stores results to be written later.
            4. Creates keyvals for autotest publishing.

        Args:
            mtype: string of measurement type.  For example:
                   pwr == power
                   temp == temperature

        Returns:
            dict of keyvals suitable for autotest results.
        """
        if not mtype:
            mtype = 'meas'

        t = numpy.array(self.times)
        keyvals = {}
        results  = []

        if not self.done:
            self.done = True
        # times 2 the sleep time in order to allow for readings as well.
        self.join(timeout=self.seconds_period * 2)

        if not self._checkpoint_data:
            self.checkpoint()

        for i, domain_readings in enumerate(zip(*self.readings)):
            meas = numpy.array(domain_readings)
            domain = self.domains[i]

            for tname, tstart, tend in self._checkpoint_data:
                if tname:
                    prefix = '%s_%s' % (tname, domain)
                else:
                    prefix = domain
                keyvals[prefix+'_duration'] = tend - tstart
                # Select all readings taken between tstart and tend timestamps.
                # Try block just in case
                # code.google.com/p/chromium/issues/detail?id=318892
                # is not fixed.
                try:
                    meas_array = meas[numpy.bitwise_and(tstart < t, t < tend)]
                except ValueError, e:
                    logging.debug('Error logging measurements: %s', str(e))
                    logging.debug('timestamps %d %s' % (t.len, t))
                    logging.debug('timestamp start, end %f %f' % (tstart, tend))
                    logging.debug('measurements %d %s' % (meas.len, meas))

                # If sub-test terminated early, avoid calculating avg, std and
                # min
                if not meas_array.size:
                    continue
                meas_mean = meas_array.mean()
                meas_std = meas_array.std()

                # Results list can be used for pretty printing and saving as csv
                results.append((prefix, meas_mean, meas_std,
                                tend - tstart, tstart, tend))

                keyvals[prefix + '_' + mtype] = meas_mean
                keyvals[prefix + '_' + mtype + '_cnt'] = meas_array.size
                keyvals[prefix + '_' + mtype + '_max'] = meas_array.max()
                keyvals[prefix + '_' + mtype + '_min'] = meas_array.min()
                keyvals[prefix + '_' + mtype + '_std'] = meas_std

        self._results = results
        return keyvals


    def save_results(self, resultsdir, fname=None):
        """Save computed results in a nice tab-separated format.
        This is useful for long manual runs.

        Args:
            resultsdir: String, directory to write results to
            fname: String name of file to write results to
        """
        if not fname:
            fname = 'meas_results_%.0f.txt' % time.time()
        fname = os.path.join(resultsdir, fname)
        with file(fname, 'wt') as f:
            for row in self._results:
                # First column is name, the rest are numbers. See _calc_power()
                fmt_row = [row[0]] + ['%.2f' % x for x in row[1:]]
                line = '\t'.join(fmt_row)
                f.write(line + '\n')


class PowerLogger(MeasurementLogger):
    def save_results(self, resultsdir, fname=None):
        if not fname:
            fname = 'power_results_%.0f.txt' % time.time()
        super(PowerLogger, self).save_results(resultsdir, fname)


    def calc(self, mtype='pwr'):
        return super(PowerLogger, self).calc(mtype)


class TempMeasurement(object):
    """Class to measure temperature.

    Public attributes:
        domain: String name of the temperature domain being measured.  Example is
          'cpu' for cpu temperature

    Private attributes:
        _path: Path to temperature file to read ( in millidegrees Celsius )

    Public methods:
        refresh: Performs any temperature sampling and calculation and returns
            temperature as float in degrees Celsius.
    """
    def __init__(self, domain, path):
        """Constructor."""
        self.domain = domain
        self._path = path


    def refresh(self):
        """Performs temperature

        Returns:
            float, temperature in degrees Celsius
        """
        return int(utils.read_one_line(self._path)) / 1000.


class TempLogger(MeasurementLogger):
    """A thread that logs temperature readings in millidegrees Celsius."""
    def __init__(self, measurements, seconds_period=30.0):
        if not measurements:
            measurements = []
            tstats = ThermalStatHwmon()
            for kname in tstats.fields:
                match = re.match(r'(\S+)_temp(\d+)_input', kname)
                if not match:
                    continue
                domain = match.group(1) + '-t' + match.group(2)
                fpath = tstats.fields[kname][0]
                new_meas = TempMeasurement(domain, fpath)
                measurements.append(new_meas)
        super(TempLogger, self).__init__(measurements, seconds_period)


    def save_results(self, resultsdir, fname=None):
        if not fname:
            fname = 'temp_results_%.0f.txt' % time.time()
        super(TempLogger, self).save_results(resultsdir, fname)


    def calc(self, mtype='temp'):
        return super(TempLogger, self).calc(mtype)


class DiskStateLogger(threading.Thread):
    """Records the time percentages the disk stays in its different power modes.

    Example code snippet:
        mylogger = power_status.DiskStateLogger()
        mylogger.start()
        result = mylogger.result()

    Public methods:
        start: Launches the thread and starts measurements.
        result: Stops the thread if it's still running and returns measurements.
        get_error: Returns the exception in _error if it exists.

    Private functions:
        _get_disk_state: Returns the disk's current ATA power mode as a string.

    Private attributes:
        _seconds_period: Disk polling interval in seconds.
        _stats: Dict that maps disk states to seconds spent in them.
        _running: Flag that is True as long as the logger should keep running.
        _time: Timestamp of last disk state reading.
        _device_path: The file system path of the disk's device node.
        _error: Contains a TestError exception if an unexpected error occured
    """
    def __init__(self, seconds_period = 5.0, device_path = None):
        """Initializes a logger.

        Args:
            seconds_period: Disk polling interval in seconds. Default 5.0
            device_path: The path of the disk's device node. Default '/dev/sda'
        """
        threading.Thread.__init__(self)
        self._seconds_period = seconds_period
        self._device_path = device_path
        self._stats = {}
        self._running = False
        self._error = None

        result = utils.system_output('rootdev -s')
        # TODO(tbroch) Won't work for emmc storage and will throw this error in
        # keyvals : 'ioctl(SG_IO) error: [Errno 22] Invalid argument'
        # Lets implement something complimentary for emmc
        if not device_path:
            self._device_path = \
                re.sub('(sd[a-z]|mmcblk[0-9]+)p?[0-9]+', '\\1', result)
        logging.debug("device_path = %s", self._device_path)


    def start(self):
        logging.debug("inside DiskStateLogger.start")
        if os.path.exists(self._device_path):
            logging.debug("DiskStateLogger started")
            super(DiskStateLogger, self).start()


    def _get_disk_state(self):
        """Checks the disk's power mode and returns it as a string.

        This uses the SG_IO ioctl to issue a raw SCSI command data block with
        the ATA-PASS-THROUGH command that allows SCSI-to-ATA translation (see
        T10 document 04-262r8). The ATA command issued is CHECKPOWERMODE1,
        which returns the device's current power mode.
        """

        def _addressof(obj):
            """Shortcut to return the memory address of an object as integer."""
            return ctypes.cast(obj, ctypes.c_void_p).value

        scsi_cdb = struct.pack("12B", # SCSI command data block (uint8[12])
                               0xa1, # SCSI opcode: ATA-PASS-THROUGH
                               3 << 1, # protocol: Non-data
                               1 << 5, # flags: CK_COND
                               0, # features
                               0, # sector count
                               0, 0, 0, # LBA
                               1 << 6, # flags: ATA-USING-LBA
                               0xe5, # ATA opcode: CHECKPOWERMODE1
                               0, # reserved
                               0, # control (no idea what this is...)
                              )
        scsi_sense = (ctypes.c_ubyte * 32)() # SCSI sense buffer (uint8[32])
        sgio_header = struct.pack("iiBBHIPPPIIiPBBBBHHiII", # see <scsi/sg.h>
                                  83, # Interface ID magic number (int32)
                                  -1, # data transfer direction: none (int32)
                                  12, # SCSI command data block length (uint8)
                                  32, # SCSI sense data block length (uint8)
                                  0, # iovec_count (not applicable?) (uint16)
                                  0, # data transfer length (uint32)
                                  0, # data block pointer
                                  _addressof(scsi_cdb), # SCSI CDB pointer
                                  _addressof(scsi_sense), # sense buffer pointer
                                  500, # timeout in milliseconds (uint32)
                                  0, # flags (uint32)
                                  0, # pack ID (unused) (int32)
                                  0, # user data pointer (unused)
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, # output params
                                 )
        try:
            with open(self._device_path, 'r') as dev:
                result = fcntl.ioctl(dev, 0x2285, sgio_header)
        except IOError, e:
            raise error.TestError('ioctl(SG_IO) error: %s' % str(e))
        _, _, _, _, status, host_status, driver_status = \
            struct.unpack("4x4xxx2x4xPPP4x4x4xPBxxxHH4x4x4x", result)
        if status != 0x2: # status: CHECK_CONDITION
            raise error.TestError('SG_IO status: %d' % status)
        if host_status != 0:
            raise error.TestError('SG_IO host status: %d' % host_status)
        if driver_status != 0x8: # driver status: SENSE
            raise error.TestError('SG_IO driver status: %d' % driver_status)

        if scsi_sense[0] != 0x72: # resp. code: current error, descriptor format
            raise error.TestError('SENSE response code: %d' % scsi_sense[0])
        if scsi_sense[1] != 0: # sense key: No Sense
            raise error.TestError('SENSE key: %d' % scsi_sense[1])
        if scsi_sense[7] < 14: # additional length (ATA status is 14 - 1 bytes)
            raise error.TestError('ADD. SENSE too short: %d' % scsi_sense[7])
        if scsi_sense[8] != 0x9: # additional descriptor type: ATA Return Status
            raise error.TestError('SENSE descriptor type: %d' % scsi_sense[8])
        if scsi_sense[11] != 0: # errors: none
            raise error.TestError('ATA error code: %d' % scsi_sense[11])

        if scsi_sense[13] == 0x00:
            return 'standby'
        if scsi_sense[13] == 0x80:
            return 'idle'
        if scsi_sense[13] == 0xff:
            return 'active'
        return 'unknown(%d)' % scsi_sense[13]


    def run(self):
        """The Thread's run method."""
        try:
            self._time = time.time()
            self._running = True
            while(self._running):
                time.sleep(self._seconds_period)
                state = self._get_disk_state()
                new_time = time.time()
                if state in self._stats:
                    self._stats[state] += new_time - self._time
                else:
                    self._stats[state] = new_time - self._time
                self._time = new_time
        except error.TestError, e:
            self._error = e
            self._running = False


    def result(self):
        """Stop the logger and return dict with result percentages."""
        if (self._running):
            self._running = False
            self.join(self._seconds_period * 2)
        return AbstractStats.to_percent(self._stats)


    def get_error(self):
        """Returns the _error exception... please only call after result()."""
        return self._error
