# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import re
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


def has_ectool():
    """Determine if ectool shell command is present.

    Returns:
        boolean true if avail, false otherwise.
    """
    cmd = 'which ectool'
    return (utils.system(cmd, ignore_status=True) == 0)


class EC_Common(object):
    """Class for EC common.

    This incredibly brief base class is intended to encapsulate common elements
    across various CrOS MCUs (ec proper, USB-PD, Sensor Hub).  At the moment
    that includes only the use of ectool.
    """

    def __init__(self, target='cros_ec'):
        """Constructor.

        @param target: target name of ec to communicate with.
        """
        if not has_ectool():
            ec_info = utils.system_output("mosys ec info",
                                          ignore_status=True)
            logging.warning("Ectool absent on this platform ( %s )",
                         ec_info)
            raise error.TestNAError("Platform doesn't support ectool")
        self._target = target

    def ec_command(self, cmd, **kwargs):
        """Executes ec command and returns results.

        @param cmd: string of command to execute.
        @param kwargs: optional params passed to utils.system_output

        @returns: string of results from ec command.
        """
        full_cmd = 'ectool --name=%s %s' % (self._target, cmd)
        result = utils.system_output(full_cmd, **kwargs)
        logging.debug('Command: %s', full_cmd)
        logging.debug('Result: %s', result)
        return result


class EC(EC_Common):
    """Class for CrOS embedded controller (EC)."""
    HELLO_RE = "EC says hello"
    GET_FANSPEED_RE = "Current fan RPM: ([0-9]*)"
    SET_FANSPEED_RE = "Fan target RPM set."
    TEMP_SENSOR_RE = "Reading temperature...([0-9]*)"
    TOGGLE_AUTO_FAN_RE = "Automatic fan control is now on"
    # For battery, check we can see a non-zero capacity value.
    BATTERY_RE = "Design capacity:\s+[1-9]\d*\s+mAh"
    LIGHTBAR_RE = "^ 05\s+3f\s+3f$"


    def hello(self):
        """Test EC hello command.

        @returns True if success False otherwise.
        """
        response = self.ec_command('hello')
        return (re.search(self.HELLO_RE, response) is not None)

    def auto_fan_ctrl(self):
        """Turns auto fan ctrl on.

        @returns True if success False otherwise.
        """
        response = self.ec_command('autofanctrl')
        logging.info('Turned on auto fan control.')
        return (re.search(self.TOGGLE_AUTO_FAN_RE, response) is not None)

    def get_fanspeed(self):
        """Gets fanspeed.

        @raises error.TestError if regexp fails to match.

        @returns integer of fan speed RPM.
        """
        response = self.ec_command('pwmgetfanrpm')
        match = re.search(self.GET_FANSPEED_RE, response)
        if not match:
            raise error.TestError('Unable to read fan speed')

        rpm = int(match.group(1))
        logging.info('Fan speed: %d', rpm)
        return rpm

    def set_fanspeed(self, rpm):
        """Sets fan speed.

        @param rpm: integer of fan speed RPM to set

        @returns True if success False otherwise.
        """
        response = self.ec_command('pwmsetfanrpm %d' % rpm)
        logging.info('Set fan speed: %d', rpm)
        return (re.search(self.SET_FANSPEED_RE, response) is not None)

    def get_temperature(self, idx):
        """Gets temperature from idx sensor.

        @param idx: integer of temp sensor to read.

        @raises error.TestError if fails to read sensor.

        @returns integer of temperature reading in degrees Kelvin.
        """
        response = self.ec_command('temps %d' % idx)
        match = re.search(self.TEMP_SENSOR_RE, response)
        if not match:
            raise error.TestError('Unable to read temperature sensor %d' % idx)

        return int(match.group(1))

    def get_battery(self):
        """Get battery presence (design capacity found).

        @returns True if success False otherwise.
        """
        response = self.ec_command('battery')
        return (re.search(self.BATTERY_RE, response) is not None)

    def get_lightbar(self):
        """Test lightbar.

        @returns True if success False otherwise.
        """
        self.ec_command('lightbar on')
        self.ec_command('lightbar init')
        self.ec_command('lightbar 4 255 255 255')
        response = self.ec_command('lightbar')
        self.ec_command('lightbar off')
        return (re.search(self.LIGHTBAR_RE, response, re.MULTILINE) is not None)


class EC_USBPD_Port(EC_Common):
    """Class for CrOS embedded controller for USB-PD Port.

    Public attributes:
        index: integer of USB type-C port index.

    Public Methods:
        is_dfp: Determine if data role is Downstream Facing Port (DFP).
        is_amode_supported: Check if alternate mode is supported by port.
        is_amode_entered: Check if alternate mode is entered.
        set_amode: Set an alternate mode.

    Private attributes:
        _port: integer of USB type-C port id.
        _port_info: holds usbpd protocol info.
        _amodes: holds alternate mode info.

    Private methods:
        _invalidate_port_data: Remove port data to force re-eval.
        _get_port_info: Get USB-PD port info.
        _get_amodes: parse and return port's svid info.
    """
    def __init__(self, index):
        """Constructor.

        @param index: integer of USB type-C port index.
        """
        self.index = index
        # TODO(crosbug.com/p/38133) target= only works for samus
        super(EC_USBPD_Port, self).__init__(target='cros_pd')

        # Interrogate port at instantiation.  Use invalidate to force re-eval.
        self._port_info = self._get_port_info()
        self._amodes = self._get_amodes()

    def _invalidate_port_data(self):
        """Remove port data to force re-eval."""
        self._port_info = None
        self._amodes = None

    def _get_port_info(self):
        """Get USB-PD port info.

        ectool command usbpd provides the following information about the port:
          - Enabled/Disabled
          - Power & Data Role
          - Polarity
          - Protocol State

        At time of authoring it looks like:
          Port C0 is enabled, Role:SNK UFP Polarity:CC2 State:SNK_READY

        @raises error.TestError if ...
          port info not parseable.

        @returns dictionary for <port> with keyval pairs:
          enabled: True | False | None
          power_role: sink | source | None
          data_role: UFP | DFP | None
          is_reversed: True | False | None
          state: various strings | None
        """
        PORT_INFO_RE = 'Port\s+C(\d+)\s+is\s+(\w+),\s+Role:(\w+)\s+(\w+)\s+' + \
                       'Polarity:CC(\d+)\s+State:(\w+)'

        match = re.search(PORT_INFO_RE,
                          self.ec_command("usbpd %s" % (self.index)))
        if not match or int(match.group(1)) != self.index:
            error.TestError('Unable to determine port %d info' % self.index)

        pinfo = dict(enabled=None, power_role=None, data_role=None,
                    is_reversed=None, state=None)
        pinfo['enabled'] = match.group(2) == 'enabled'
        pinfo['power_role'] = 'sink' if match.group(3) == 'SNK' else 'source'
        pinfo['data_role'] = match.group(4)
        pinfo['is_reversed'] = True if match.group(5) == '2' else False
        pinfo['state'] = match.group(6)
        logging.debug('port_info = %s', pinfo)
        return pinfo

    def _get_amodes(self):
        """Parse alternate modes from pdgetmode.

        Looks like ...
          *SVID:0xff01 *0x00000485  0x00000000 ...
          SVID:0x18d1   0x00000001  0x00000000 ...

        @returns dictionary of format:
          <svid>: {active: True|False, configs: <config_list>, opos:<opos>}
            where:
              <svid>        : USB-IF Standard or vendor id as
                              hex string (i.e. 0xff01)
              <config_list> : list of uint32_t configs
              <opos>        : integer of active object position.
                              Note, this is the config list index + 1
        """
        SVID_RE = r'(\*?)SVID:(\S+)\s+(.*)'
        svids = dict()
        cmd = 'pdgetmode %d' % self.index
        for line in self.ec_command(cmd, ignore_status=True).split('\n'):
            if line.strip() == '':
                continue
            logging.debug('pdgetmode line: %s', line)
            match = re.search(SVID_RE, line)
            if not match:
                logging.warning("Unable to parse SVID line %s", line)
                continue
            active = match.group(1) == '*'
            svid = match.group(2)
            configs_str = match.group(3)
            configs = list()
            opos = None
            for i,config in enumerate(configs_str.split(), 1):
                if config.startswith('*'):
                    opos = i
                    config = config[1:]
                config = int(config, 16)
                # ignore unpopulated configs
                if config == 0:
                    continue
                configs.append(config)
            svids[svid] = dict(active=active, configs=configs, opos=opos)

        logging.debug("Port %d svids = %s", self.index, svids)
        return svids

    def is_dfp(self):
        """Determine if data role is Downstream Facing Port (DFP).

        @returns True if DFP False otherwise.
        """
        if self._port_info is None:
            self._port_info = self._get_port_info()

        return self._port_info['data_role'] == 'DFP'

    def is_amode_supported(self, svid):
        """Check if alternate mode is supported by port partner.

        @param svid: alternate mode SVID hexstring (i.e. 0xff01)
        """
        if self._amodes is None:
            self._amodes = self._get_amodes()

        if svid in self._amodes.keys():
            return True
        return False

    def is_amode_entered(self, svid, opos):
        """Check if alternate mode is entered.

        @param svid: alternate mode SVID hexstring (i.e. 0xff01).
        @param opos: object position of config to act on.

        @returns True if entered False otherwise
        """
        if self._amodes is None:
            self._amodes = self._get_amodes()

        if not self.is_amode_supported(svid):
            return False

        if self._amodes[svid]['active'] and self._amodes[svid]['opos'] == opos:
            return True

        return False

    def set_amode(self, svid, opos, enter, delay_secs=2):
        """Set alternate mode.

        @param svid: alternate mode SVID hexstring (i.e. 0xff01).
        @param opos: object position of config to act on.
        @param enter: Boolean of whether to enter mode.

        @raises error.TestError if ...
           mode not supported.
           opos is > number of configs.

        @returns True if successful False otherwise
        """
        if self._amodes is None:
            self._amodes = self._get_amodes()

        if svid not in self._amodes.keys():
            raise error.TestError("SVID %s not supported", svid)

        if opos > len(self._amodes[svid]['configs']):
            raise error.TestError("opos > available configs")

        cmd = "pdsetmode %d %s %d %d" % (self.index, svid, opos,
                                         1 if enter else 0)
        self.ec_command(cmd, ignore_status=True)
        self._invalidate_port_data()

        # allow some time for mode entry/exit
        time.sleep(delay_secs)
        return self.is_amode_entered(svid, opos) == enter

    def get_flash_info(self):
        mat0_re = r'has no discovered device'
        mat1_re = r'.*ptype:(\d+)\s+vid:(\w+)\s+pid:(\w+).*'
        mat2_re = r'.*DevId:(\d+)\.(\d+)\s+Hash:\s*(\w+.*)\s*CurImg:(\w+).*'
        flash_dict = dict.fromkeys(['ptype', 'vid', 'pid', 'dev_major',
                                    'dev_minor', 'rw_hash', 'image_status'])

        cmd = 'infopddev %d' % self.index

        tries = 3
        while (tries):
            res = self.ec_command(cmd, ignore_status=True)
            if not 'has no discovered device' in res:
                break

            tries -= 1
            time.sleep(1)

        for ln in res.split('\n'):
            mat1 = re.match(mat1_re, ln)
            if mat1:
                flash_dict['ptype'] = int(mat1.group(1))
                flash_dict['vid'] = mat1.group(2)
                flash_dict['pid'] = mat1.group(3)
                continue

            mat2 = re.match(mat2_re, ln)
            if mat2:
                flash_dict['dev_major'] = int(mat2.group(1))
                flash_dict['dev_minor'] = int(mat2.group(2))
                flash_dict['rw_hash'] = mat2.group(3)
                flash_dict['image_status'] = mat2.group(4)
                break

        return flash_dict


class EC_USBPD(EC_Common):
    """Class for CrOS embedded controller for USB-PD.

    Public attributes:
        ports: list EC_USBPD_Port instances

    Public Methods:
        get_num_ports: get number of USB-PD ports device has.

    Private attributes:
        _num_ports: integer number of USB-PD ports device has.
    """
    def __init__(self, num_ports=None):
        """Constructor.

        @param num_ports: total number of USB-PD ports on device.  This is an
          override.  If left 'None' will try to determine.
        """
        self._num_ports = num_ports
        self.ports = list()

        # TODO(crosbug.com/p/38133) target= only works for samus
        super(EC_USBPD, self).__init__(target='cros_pd')

        if (self.get_num_ports() == 0):
            raise error.TestNAError("Device has no USB-PD ports")

        for i in xrange(self._num_ports):
            self.ports.append(EC_USBPD_Port(i))

    def get_num_ports(self):
        """Determine the number of ports for device.

        Uses ectool's usbpdpower command which in turn makes host command call
        to EC_CMD_USB_PD_PORTS to determine the number of ports.

        TODO(tbroch) May want to consider adding separate ectool command to
        surface the number of ports directly instead of via usbpdpower

        @returns number of ports.
        """
        if (self._num_ports is not None):
            return self._num_ports

        self._num_ports = len(self.ec_command("usbpdpower").split(b'\n'))
        return self._num_ports
