#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manage vms through vagrant.

The intent of this interface is to provde a layer of abstraction
between the box providers and the creation of a lab cluster. To switch to a
different provider:

* Create a VagrantFile template and specify _template in the subclass
  Eg: GCE VagrantFiles need a :google section
* Override vagrant_cmd to massage parameters
  Eg: vagrant up => vagrant up --provider=google

Note that the second is optional because most providers honor
`VAGRANT_DEFAULT_PROVIDER` directly in the template.
"""


import logging
import subprocess
import sys
import os

import common
from autotest_lib.site_utils.lib import infra


class VagrantCmdError(Exception):
    """Raised when a vagrant command fails."""


# TODO: We don't really need to setup everythig in the same VAGRANT_DIR.
# However managing vms becomes a headache once the VagrantFile and its
# related dot files are removed, as one has to resort to directly
# querying the box provider. Always running the cluster from the same
# directory simplifies vm lifecycle management.
VAGRANT_DIR = os.path.abspath(os.path.join(__file__, os.pardir))
VAGRANT_VERSION = '1.6.0'


def format_msg(msg):
    """Format the give message.

    @param msg: A message to format out to stdout.
    """
    print '\n{:^20s}%s'.format('') % msg


class VagrantProvisioner(object):
    """Provisiong vms with vagrant."""

    # A path to a Vagrantfile template specific to the vm provider, specified
    # in the child class.
    _template = None
    _box_name = 'base'


    @classmethod
    def vagrant_cmd(cls, cmd, stream_output=False):
        """Execute a vagrant command in VAGRANT_DIR.

        @param cmd: The command to execute.
        @param stream_output: If True, stream the output of `cmd`.
                Waits for `cmd` to finish and returns a string with the
                output if false.
        """
        with infra.chdir(VAGRANT_DIR):
            try:
                return infra.execute_command(
                        'localhost',
                        'vagrant %s' % cmd, stream_output=stream_output)
            except subprocess.CalledProcessError as e:
                raise VagrantCmdError(
                        'Command "vagrant %s" failed with %s' % (cmd, e))


    def _check_vagrant(self):
        """Check Vagrant."""

        # TODO: Automate the installation of vagrant.
        try:
            version = int(self.vagrant_cmd('--version').rstrip('\n').rsplit(
                    ' ')[-1].replace('.', ''))
        except VagrantCmdError:
            logging.error(
                    'Looks like you don\'t have vagrant. Please run: \n'
                    '`apt-get install virtualbox vagrant`. This assumes you '
                    'are on Trusty; There is a TODO to automate installation.')
            sys.exit(1)
        except TypeError as e:
            logging.warning('The format of the vagrant version string seems to '
                            'have changed, assuming you have a version > %s.',
                            VAGRANT_VERSION)
            return
        if version < int(VAGRANT_VERSION.replace('.', '')):
            logging.error('Please upgrade vagrant to a version > %s by '
                          'downloading a deb file from '
                          'https://www.vagrantup.com/downloads and installing '
                          'it with dpkg -i file.deb', VAGRANT_VERSION)
            sys.exit(1)


    def __init__(self, puppet_path):
        """Initialize a vagrant provisioner.

        @param puppet_path: Since vagrant uses puppet to provision machines,
                this is the location of puppet modules for various server roles.
        """
        self._check_vagrant()
        self.puppet_path = puppet_path


    def register_box(self, source, name=_box_name):
        """Register a box with vagrant.

        Eg: vagrant box add core_cluster chromeos_lab_core_cluster.box

        @param source: A path to the box, typically a file path on localhost.
        @param name: A name to register the box under.
        """
        if name in self.vagrant_cmd('box list'):
            logging.warning("Name %s already in registry, will reuse.", name)
            return
        logging.info('Adding a new box from %s under name: %s', source, name)
        self.vagrant_cmd('box add %s %s' % (name, source))


    def unregister_box(self, name):
        """Unregister a box.

        Eg: vagrant box remove core_cluster.

        @param name: The name of the box as it appears in `vagrant box list`
        """
        if name not in self.vagrant_cmd('box list'):
            logging.warning("Name %s not in registry.", name)
            return
        logging.info('Removing box %s', name)
        self.vagrant_cmd('box remove %s' % name)


    def create_vagrant_file(self, **kwargs):
        """Create a vagrant file.

        Read the template, apply kwargs and the puppet_path so vagrant can find
        server provisioning rules, and write it back out as the VagrantFile.

        @param kwargs: Extra args needed to convert a template
                to a real VagrantFile.
        """
        vagrant_file = os.path.join(VAGRANT_DIR, 'Vagrantfile')
        kwargs.update({
            'manifest_path': os.path.join(self.puppet_path, 'manifests'),
            'module_path': os.path.join(self.puppet_path, 'modules'),
        })
        vagrant_template = ''
        with open(self._template, 'r') as template:
            vagrant_template = template.read()
        with open(vagrant_file, 'w') as vagrantfile:
            vagrantfile.write(vagrant_template % kwargs)


    # TODO: This is a leaky abstraction, since it isn't really clear
    # what the kwargs are. It's the best we can do, because the kwargs
    # really need to match the VagrantFile. We leave parsing the VagrantFile
    # for the right args upto the caller.
    def initialize_vagrant(self, **kwargs):
        """Initialize vagrant.

        @param kwargs: The kwargs to pass to the VagrantFile.
            Eg: {
                'shard1': 'stumpyshard',
                'shard1_port': 8002,
                'shard1_shadow_config_hostname': 'localhost:8002',
            }
        @return: True if vagrant was initialized, False if the cwd already
                 contains a vagrant environment.
        """
        # TODO: Split this out. There are cases where we will need to
        # reinitialize (by destroying all vms and recreating the VagrantFile)
        # that we cannot do without manual intervention right now.
        try:
            self.vagrant_cmd('status')
            logging.info('Vagrant already initialized in %s', VAGRANT_DIR)
            return False
        except VagrantCmdError:
            logging.info('Initializing vagrant in %s', VAGRANT_DIR)
            self.create_vagrant_file(**kwargs)
            return True


    def provision(self, force=False):
        """Provision vms according to the vagrant file.

        @param force: If True, vms in the VAGRANT_DIR will be destroyed and
                reprovisioned.
        """
        if force:
            logging.info('Destroying vagrant setup.')
            try:
                self.vagrant_cmd('destroy --force', stream_output=True)
            except VagrantCmdError:
                pass
        format_msg('Starting vms. This should take no longer than 5 minutes')
        self.vagrant_cmd('up', stream_output=True)


class VirtualBox(VagrantProvisioner):
    """A VirtualBoxProvisioner."""

    _template = os.path.join(VAGRANT_DIR, 'ClusterTemplate')
