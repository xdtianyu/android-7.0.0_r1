# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import os
import shutil
import time

from autotest_lib.client.bin import utils

class NetworkChroot(object):
    """Implements a chroot environment that runs in a separate network
    namespace from the caller.  This is useful for network tests that
    involve creating a server on the other end of a virtual ethernet
    pair.  This object is initialized with an interface name to pass
    to the chroot, as well as the IP address to assign to this
    interface, since in passing the interface into the chroot, any
    pre-configured address is removed.

    The startup of the chroot is an orchestrated process where a
    small startup script is run to perform the following tasks:
      - Write out pid file which will be a handle to the
        network namespace that that |interface| should be passed to.
      - Wait for the network namespace to be passed in, by performing
        a "sleep" and writing the pid of this process as well.  Our
        parent will kill this process to resume the startup process.
      - We can now configure the network interface with an address.
      - At this point, we can now start any user-requested server
        processes.
    """
    BIND_ROOT_DIRECTORIES = ('bin', 'dev', 'dev/pts', 'lib', 'lib32', 'lib64',
                             'proc', 'sbin', 'sys', 'usr', 'usr/local')
    # Subset of BIND_ROOT_DIRECTORIES that should be mounted writable.
    BIND_ROOT_WRITABLE_DIRECTORIES = frozenset(('dev/pts',))
    # Directories we'll bind mount when we want to bridge DBus namespaces.
    # Includes directories containing the system bus socket and machine ID.
    DBUS_BRIDGE_DIRECTORIES = ('var/run/dbus/', 'var/lib/dbus/')

    ROOT_DIRECTORIES = ('etc',  'tmp', 'var', 'var/log', 'var/run')
    STARTUP = 'etc/chroot_startup.sh'
    STARTUP_DELAY_SECONDS = 5
    STARTUP_PID_FILE = 'var/run/vpn_startup.pid'
    STARTUP_SLEEPER_PID_FILE = 'var/run/vpn_sleeper.pid'
    COPIED_CONFIG_FILES = [
        'etc/ld.so.cache'
    ]
    CONFIG_FILE_TEMPLATES = {
        STARTUP:
            '#!/bin/sh\n'
            'exec > /var/log/startup.log 2>&1\n'
            'set -x\n'
            'echo $$ > /%(startup-pidfile)s\n'
            'sleep %(startup-delay-seconds)d &\n'
            'echo $! > /%(sleeper-pidfile)s &\n'
            'wait\n'
            'ip addr add %(local-ip-and-prefix)s dev %(local-interface-name)s\n'
            'ip link set %(local-interface-name)s up\n'
    }
    CONFIG_FILE_VALUES = {
        'sleeper-pidfile': STARTUP_SLEEPER_PID_FILE,
        'startup-delay-seconds': STARTUP_DELAY_SECONDS,
        'startup-pidfile': STARTUP_PID_FILE
    }

    def __init__(self, interface, address, prefix):
        self._interface = interface

        # Copy these values from the class-static since specific instances
        # of this class are allowed to modify their contents.
        self._bind_root_directories = list(self.BIND_ROOT_DIRECTORIES)
        self._root_directories = list(self.ROOT_DIRECTORIES)
        self._copied_config_files = list(self.COPIED_CONFIG_FILES)
        self._config_file_templates = self.CONFIG_FILE_TEMPLATES.copy()
        self._config_file_values = self.CONFIG_FILE_VALUES.copy()

        self._config_file_values.update({
            'local-interface-name': interface,
            'local-ip': address,
            'local-ip-and-prefix': '%s/%d' % (address, prefix)
        })


    def startup(self):
        """Create the chroot and start user processes."""
        self.make_chroot()
        self.write_configs()
        self.run(['/bin/bash', os.path.join('/', self.STARTUP), '&'])
        self.move_interface_to_chroot_namespace()
        self.kill_pid_file(self.STARTUP_SLEEPER_PID_FILE)


    def shutdown(self):
        """Remove the chroot filesystem in which the VPN server was running"""
        # TODO(pstew): Some processes take a while to exit, which will cause
        # the cleanup below to fail to complete successfully...
        time.sleep(10)
        utils.system_output('rm -rf --one-file-system %s' % self._temp_dir,
                            ignore_status=True)


    def add_config_templates(self, template_dict):
        """Add a filename-content dict to the set of templates for the chroot

        @param template_dict dict containing filename-content pairs for
            templates to be applied to the chroot.  The keys to this dict
            should not contain a leading '/'.

        """
        self._config_file_templates.update(template_dict)


    def add_config_values(self, value_dict):
        """Add a name-value dict to the set of values for the config template

        @param value_dict dict containing key-value pairs of values that will
            be applied to the config file templates.

        """
        self._config_file_values.update(value_dict)


    def add_copied_config_files(self, files):
        """Add |files| to the set to be copied to the chroot.

        @param files iterable object containing a list of files to
            be copied into the chroot.  These elements should not contain a
            leading '/'.

        """
        self._copied_config_files += files


    def add_root_directories(self, directories):
        """Add |directories| to the set created within the chroot.

        @param directories list/tuple containing a list of directories to
            be created in the chroot.  These elements should not contain a
            leading '/'.

        """
        self._root_directories += directories


    def add_startup_command(self, command):
        """Add a command to the script run when the chroot starts up.

        @param command string containing the command line to run.

        """
        self._config_file_templates[self.STARTUP] += '%s\n' % command


    def get_log_contents(self):
        """Return the logfiles from the chroot."""
        return utils.system_output("head -10000 %s" %
                                   self.chroot_path("var/log/*"))


    def bridge_dbus_namespaces(self):
        """Make the system DBus daemon visible inside the chroot."""
        # Need the system socket and the machine-id.
        self._bind_root_directories += self.DBUS_BRIDGE_DIRECTORIES


    def chroot_path(self, path):
        """Returns the the path within the chroot for |path|.

        @param path string filename within the choot.  This should not
            contain a leading '/'.

        """
        return os.path.join(self._temp_dir, path.lstrip('/'))


    def get_pid_file(self, pid_file, missing_ok=False):
        """Returns the integer contents of |pid_file| in the chroot.

        @param pid_file string containing the filename within the choot
            to read and convert to an integer.  This should not contain a
            leading '/'.
        @param missing_ok bool indicating whether exceptions due to failure
            to open the pid file should be caught.  If true a missing pid
            file will cause this method to return 0.  If false, a missing
            pid file will cause an exception.

        """
        chroot_pid_file = self.chroot_path(pid_file)
        try:
            with open(chroot_pid_file) as f:
                return int(f.read())
        except IOError, e:
            if not missing_ok or e.errno != errno.ENOENT:
                raise e

            return 0


    def kill_pid_file(self, pid_file, missing_ok=False):
        """Kills the process belonging to |pid_file| in the chroot.

        @param pid_file string filename within the chroot to gain the process ID
            which this method will kill.
        @param missing_ok bool indicating whether a missing pid file is okay,
            and should be ignored.

        """
        pid = self.get_pid_file(pid_file, missing_ok=missing_ok)
        if missing_ok and pid == 0:
            return
        utils.system('kill %d' % pid, ignore_status=True)


    def make_chroot(self):
        """Make a chroot filesystem."""
        self._temp_dir = utils.system_output('mktemp -d /tmp/chroot.XXXXXXXXX')
        utils.system('chmod go+rX %s' % self._temp_dir)
        for rootdir in self._root_directories:
            os.mkdir(self.chroot_path(rootdir))

        self._jail_args = []
        for rootdir in self._bind_root_directories:
            src_path = os.path.join('/', rootdir)
            dst_path = self.chroot_path(rootdir)
            if not os.path.exists(src_path):
                continue
            elif os.path.islink(src_path):
                link_path = os.readlink(src_path)
                os.symlink(link_path, dst_path)
            else:
                os.makedirs(dst_path)  # Recursively create directories.
                mount_arg = '%s,%s' % (src_path, src_path)
                if rootdir in self.BIND_ROOT_WRITABLE_DIRECTORIES:
                    mount_arg += ',1'
                self._jail_args += [ '-b', mount_arg ]

        for config_file in self._copied_config_files:
            src_path = os.path.join('/', config_file)
            dst_path = self.chroot_path(config_file)
            if os.path.exists(src_path):
                shutil.copyfile(src_path, dst_path)


    def move_interface_to_chroot_namespace(self):
        """Move network interface to the network namespace of the server."""
        utils.system('ip link set %s netns %d' %
                     (self._interface,
                      self.get_pid_file(self.STARTUP_PID_FILE)))


    def run(self, args, ignore_status=False):
        """Run a command in a chroot, within a separate network namespace.

        @param args list containing the command line arguments to run.
        @param ignore_status bool set to true if a failure should be ignored.

        """
        utils.system('minijail0 -e -C %s %s' %
                     (self._temp_dir, ' '.join(self._jail_args + args)),
                     ignore_status=ignore_status)


    def write_configs(self):
        """Write out config files"""
        for config_file, template in self._config_file_templates.iteritems():
            with open(self.chroot_path(config_file), 'w') as f:
                f.write(template % self._config_file_values)
