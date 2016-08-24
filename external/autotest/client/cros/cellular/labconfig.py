# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import optparse
import pickle
import re
import subprocess

import common
from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.cros.cellular import cellular_logging
from autotest_lib.client.cros.cellular import labconfig_data

log = cellular_logging.SetupCellularLogging('labconfig')


class LabConfigError(Exception):
    """Exception thrown on bad lab configuration"""
    pass


def get_interface_ip(interface='eth0'):
    """Returns the IP address for an interface, or None if not found.
    @param interface: the interface to request IP address for.
    """

    # We'd like to use
    #  utils.system_output('ifconfig eth0 2>&1', retain_output=True)
    # but that gives us a dependency on the rest of autotest, which
    # means that running the unit test requires pythonpath manipulation
    stdout = subprocess.Popen(['ip', '-4', 'addr', 'show', 'dev', interface],
                              stdout=subprocess.PIPE).communicate()[0]

    match = re.search(r'inet ([0-9.]+)[/ ]', stdout)
    if not match:
        return None
    return match.group(1)


class Configuration(object):
    """Configuration for a cellular test.

    This includes things like the address of the cell emulator device
    and details of the RF switching between the emulated basestation
    and the DUT."""

    def __init__(self, args):
        # For server tests, this constructor runs as part of the
        # server control file, on whatever machine the test was
        # started on.
        parser = optparse.OptionParser()

        # Record our args so we can serialize ourself.
        self.args = args

        self.ip = None

        parser.add_option('--cell', dest='cell', default=None,
                          help='Cellular test cell to use')
        parser.add_option(
            '--technology', dest='technology', default='all',
            help='Radio access technologies to use (e.g. "WCDMA")')
        (self.options, _) = parser.parse_args(args)

        self.cell = self._get_cell(self.options.cell)

    def _get_cell(self, name):
        """Extracts the named cell from labconfig_data.CELLS."""
        if not name:
            raise LabConfigError(
                'Could not find --cell argument.  ' +
                'To specify a cell, pass --args=--cell=foo to test_that')

        if name not in labconfig_data.CELLS:
            raise LabConfigError(
                'Could not find cell %s, valid cells are %s' % (
                    name, labconfig_data.CELLS.keys()))

        return labconfig_data.CELLS[name]

    def _get_dut(self, machine=None):
        """Returns the DUT record for machine from cell["duts"]
        Args:
            machine:  name or IP of machine.  None: for "the current machine".

        Right now, we use the interface of eth0 to figure out which
        machine we're running on.  The important thing is that this
        matches the IP address in the cell duts configuration.  We'll
        have to come up with a better way if this proves brittle."""

        # TODO(byronk) : crosbug.com/235911:
        # autotest: Getting IP address from eth0 by name is brittle
        if self.ip and not machine:
            machine = self.ip
        ifconfig = ''
        if not machine:
            log.debug('self.ip is : %s ' % self.ip)
            # TODO(byronk): use sysfs to find network interface
            possible_interfaces = ['eth0', 'eth1', 'eth_test']
            log.debug('Looking for an up network interface in : %s' %
                      possible_interfaces)
            for interface in possible_interfaces:
                machine = get_interface_ip(interface)
                if machine:
                    log.debug('Got an IP address: %s Stopping the search.. ' %
                              machine)
                    self.ip = machine
                    break
            else:
                ifconfig = subprocess.Popen(['ip', 'addr', 'show'],
                        stdout=subprocess.PIPE).communicate()[0]
        if not machine:
            raise LabConfigError(
                'Could not determine which machine we are.\n'
                '  Cell =  %s \n' % self.options.cell +
                'Tried these interface names: %s \n' % possible_interfaces +
                '`ip addr show` output:\n%s' % ifconfig
            )

        for dut in self.cell["duts"]:
            if machine == dut["address"] or machine == dut["name"]:
                return dut

        raise LabConfigError(
            'This machine %s not matching: (%s,%s) in config. Cell = %s: %s' %
            (machine, dut['address'],
             dut['name'], self.options.cell, self.cell['duts']))

    def get_technologies(self, machine=None):
        """Gets technologies to use for machine; defaults to all available.
        @param machine: Machine to get technologies for.
        """
        technologies_list = self.options.technology.split(',')

        if 'all' in technologies_list:
            m = self._get_dut(machine)
            technologies_list = m["technologies"]

        enums = [getattr(cellular.Technology, t, None)
                 for t in technologies_list]

        if None in enums:
            raise LabConfigError(
                'Could not understand a technology in %s' % technologies_list)

        return enums

    def get_rf_switch_port(self, machine=None):
        """Get the RF Switch Port for the specified machine.
        @param machine: machine to get rf switch port for
        """
        dut = self._get_dut(machine)
        print dut
        return dut['rf_switch_port']

    def get_pickle(self):
        """Get pickled object."""
        return pickle.dumps(self)
