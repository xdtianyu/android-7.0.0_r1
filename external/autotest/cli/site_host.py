# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import inspect, new, socket, sys

from autotest_lib.client.bin import utils
from autotest_lib.cli import host, rpc
from autotest_lib.server import hosts
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.client.common_lib import error, host_protections


# In order for hosts to work correctly, some of its variables must be setup.
hosts.factory.ssh_user = 'root'
hosts.factory.ssh_pass = ''
hosts.factory.ssh_port = 22
hosts.factory.ssh_verbosity_flag = ''
hosts.factory.ssh_options = ''


# pylint: disable=missing-docstring
class site_host(host.host):
    pass


class site_host_create(site_host, host.host_create):
    """
    site_host_create subclasses host_create in host.py.
    """

    @classmethod
    def construct_without_parse(
            cls, web_server, hosts, platform=None,
            locked=False, lock_reason='', labels=[], acls=[],
            protection=host_protections.Protection.NO_PROTECTION):
        """Construct an site_host_create object and fill in data from args.

        Do not need to call parse after the construction.

        Return an object of site_host_create ready to execute.

        @param web_server: A string specifies the autotest webserver url.
            It is needed to setup comm to make rpc.
        @param hosts: A list of hostnames as strings.
        @param platform: A string or None.
        @param locked: A boolean.
        @param lock_reason: A string.
        @param labels: A list of labels as strings.
        @param acls: A list of acls as strings.
        @param protection: An enum defined in host_protections.
        """
        obj = cls()
        obj.web_server = web_server
        try:
            # Setup stuff needed for afe comm.
            obj.afe = rpc.afe_comm(web_server)
        except rpc.AuthError, s:
            obj.failure(str(s), fatal=True)
        obj.hosts = hosts
        obj.platform = platform
        obj.locked = locked
        if locked and lock_reason.strip():
            obj.data['lock_reason'] = lock_reason.strip()
        obj.labels = labels
        obj.acls = acls
        if protection:
            obj.data['protection'] = protection
        # TODO(kevcheng): Update the admin page to take in serials?
        obj.serials = None
        return obj


    def _execute_add_one_host(self, host):
        # Always add the hosts as locked to avoid the host
        # being picked up by the scheduler before it's ACL'ed.
        self.data['locked'] = True
        if not self.locked:
            self.data['lock_reason'] = 'Forced lock on device creation'
        self.execute_rpc('add_host', hostname=host,
                         status="Ready", **self.data)
        # If there are labels avaliable for host, use them.
        host_info = self.host_info_map[host]
        labels = set(self.labels)
        if host_info.labels:
            labels.update(host_info.labels)
        # Now add the platform label.
        # If a platform was not provided and we were able to retrieve it
        # from the host, use the retrieved platform.
        platform = self.platform if self.platform else host_info.platform
        if platform:
            labels.add(platform)

        if len(labels):
            self.execute_rpc('host_add_labels', id=host, labels=list(labels))

        if self.serials:
            afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
            afe.set_host_attribute('serials', ','.join(self.serials),
                                   hostname=host)


    def execute(self):
        # Check to see if the platform or any other labels can be grabbed from
        # the hosts.
        self.host_info_map = {}
        for host in self.hosts:
            try:
                if utils.ping(host, tries=1, deadline=1) == 0:
                    if self.serials and len(self.serials) > 1:
                        host_dut = hosts.create_testbed(
                                host, adb_serials=self.serials)
                    else:
                        adb_serial = None
                        if self.serials:
                            adb_serial = self.serials[0]
                        host_dut = hosts.create_host(host,
                                                     adb_serial=adb_serial)
                    host_info = host_information(host,
                                                 host_dut.get_platform(),
                                                 host_dut.get_labels())
                else:
                    # Can't ping the host, use default information.
                    host_info = host_information(host, None, [])
            except (socket.gaierror, error.AutoservRunError,
                    error.AutoservSSHTimeout):
                # We may be adding a host that does not exist yet or we can't
                # reach due to hostname/address issues or if the host is down.
                host_info = host_information(host, None, [])
            self.host_info_map[host] = host_info
        # We need to check if these labels & ACLs exist,
        # and create them if not.
        if self.platform:
            self.check_and_create_items('get_labels', 'add_label',
                                        [self.platform],
                                        platform=True)
        else:
            # No platform was provided so check and create the platform label
            # for each host.
            platforms = []
            for host_info in self.host_info_map.values():
                if host_info.platform and host_info.platform not in platforms:
                    platforms.append(host_info.platform)
            if platforms:
                self.check_and_create_items('get_labels', 'add_label',
                                            platforms,
                                            platform=True)
        labels_to_check_and_create = self.labels[:]
        for host_info in self.host_info_map.values():
            labels_to_check_and_create = (host_info.labels +
                                          labels_to_check_and_create)
        if labels_to_check_and_create:
            self.check_and_create_items('get_labels', 'add_label',
                                        labels_to_check_and_create,
                                        platform=False)

        if self.acls:
            self.check_and_create_items('get_acl_groups',
                                        'add_acl_group',
                                        self.acls)

        return self._execute_add_hosts()


class host_information(object):
    """Store host information so we don't have to keep looking it up."""


    def __init__(self, hostname, platform, labels):
        self.hostname = hostname
        self.platform = platform
        self.labels = labels


# Any classes we don't override in host should be copied automatically
for cls in [getattr(host, n) for n in dir(host) if not n.startswith("_")]:
    if not inspect.isclass(cls):
        continue
    cls_name = cls.__name__
    site_cls_name = 'site_' + cls_name
    if hasattr(sys.modules[__name__], site_cls_name):
        continue
    bases = (site_host, cls)
    members = {'__doc__': cls.__doc__}
    site_cls = new.classobj(site_cls_name, bases, members)
    setattr(sys.modules[__name__], site_cls_name, site_cls)
