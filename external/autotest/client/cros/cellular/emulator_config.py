# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Configure cellular data emulation setup."""
import time

import common
from autotest_lib.client.cros.cellular import base_station_8960
from autotest_lib.client.cros.cellular import base_station_pxt
from autotest_lib.client.cros.cellular import cellular_logging
from autotest_lib.client.cros.cellular import ether_io_rf_switch
from autotest_lib.client.cros.cellular import prologix_scpi_driver, scpi

log = cellular_logging.SetupCellularLogging('emulator_config')


class Error(Exception):
    pass

# TODO(byronk): Move this to the base_station_interface file or factory file
def _BaseStationFactory(c, technology):
    """Create a base station from a base station labconfig dictionary."""

    adapter = c['gpib_adapter']
    #TODO(byronk): get rid of the legacy single letter variable names
    s = scpi.Scpi(
        prologix_scpi_driver.PrologixScpiDriver(
            hostname=adapter['address'],
            port=adapter['ip_port'],
            gpib_address=adapter['gpib_address']),
        opc_on_stanza=True)
    if technology == 'Technology:LTE':
        return base_station_pxt.BaseStationPxt(s)
    else:
        return base_station_8960.BaseStation8960(s)


# TODO(byronk): Make this a factory class, move to a better file
def _CreateRfSwitch(config):
    if 'rf_switch' not in config.cell:
        return None
    switch_config = config.cell['rf_switch']
    if switch_config['type'] != 'ether_io':
        raise KeyError('Could not configure switch of type %s' %
                       switch_config['type'])
    return ether_io_rf_switch.RfSwitch(switch_config['address'])


def StartDefault(config, technology):
    """Set up a base station and turn it on.  Return BS and verifier."""
    # TODO(byronk): Stop using strings here. Config class? enum?
    call_box_name_part = '8960'
    if 'LTE' in technology:
        call_box_name_part = 'pxt'

    bs = None
    # Find the first matching base station. Only a problem when we go to 3.
    # TODO(byronk):This should be in the factory part
    for cfg in config.cell['basestations']:
        tp = cfg['type']
        if call_box_name_part in tp:
            bs_config = cfg
            log.info('Using this call box: %s ' % cfg)
            break
    if bs_config is None:
        raise Error(
            'None of these base stations support %s:  %s' %
            (technology, config.cell['basestations']))

    # Match up to the legacy names. TODO(byronk) :fix this mess
    #TODO(byronk): get rid of the legacy single letter variable names
    c = cfg
    bs = _BaseStationFactory(bs_config, technology)

    rf_switch = _CreateRfSwitch(config)
    if rf_switch:
        port = config.get_rf_switch_port()
        log.info(
            'Changing switch port from %s to %s' % (rf_switch.Query(), port))
        rf_switch.SelectPort(port)

    with bs.checker_context:
        bs.SetBsNetmaskV4(c['bs_netmask'])
        bs.SetBsIpV4(*c['bs_addresses'])

        bs.SetUeIpV4(*c['ue_rf_addresses'])
        bs.SetUeDnsV4(*c['ue_dns_addresses'])

        bs.SetTechnology(technology)
        bs.SetPower(-40)
        verifier = bs.GetAirStateVerifier()
        bs.Start()

    # TODO(rochberg):  Why does this seem to be necessary?
    time.sleep(5)

    return bs, verifier
