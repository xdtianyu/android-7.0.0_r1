# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class platform_Firewall(test.test):
    """Ensure the firewall service is working correctly."""

    version = 1

    _PORT = 1234
    _IFACE = "eth0"

    _TCP_RULE = "-A INPUT -p tcp -m tcp --dport %d -j ACCEPT" % _PORT
    _UDP_RULE = "-A INPUT -p udp -m udp --dport %d -j ACCEPT" % _PORT
    _IFACE_RULE = "-A INPUT -i %s -p tcp -m tcp --dport %d -j ACCEPT" % (_IFACE,
                                                                         _PORT)

    _POLL_INTERVAL = 5

    _IPTABLES_DEL_CMD = "%s -D INPUT -p %s -m %s --dport %d -j ACCEPT"

    @staticmethod
    def _iptables_rules(executable):
        rule_output = utils.system_output("%s -S" % executable)
        logging.debug(rule_output)
        return [line.strip() for line in rule_output.splitlines()]


    @staticmethod
    def _check(expected_rule, actual_rules, error_msg, executable, check):
        # If check() returns false, fail the test.
        if not check(expected_rule, actual_rules):
            raise error.TestFail(error_msg % executable)


    @staticmethod
    def _check_included(expected_rule, actual_rules, error_msg, executable):
        # Test whether the rule is included, fail if it's not.
        platform_Firewall._check(
                expected_rule, actual_rules, error_msg, executable,
                lambda e, a: e in a)


    @staticmethod
    def _check_not_included(expected_rule, actual_rules, error_msg, executable):
        # Test whether the rule is not included, fail if it is.
        platform_Firewall._check(
                expected_rule, actual_rules, error_msg, executable,
                lambda e, a: e not in a)


    def run_once(self):
        # Create lifeline file descriptors.
        self.tcp_r, self.tcp_w = os.pipe()
        self.udp_r, self.udp_w = os.pipe()
        self.iface_r, self.iface_w = os.pipe()

        try:
            bus = dbus.SystemBus()
            pb_proxy = bus.get_object('org.chromium.PermissionBroker',
                                      '/org/chromium/PermissionBroker')
            pb = dbus.Interface(pb_proxy, 'org.chromium.PermissionBroker')

            tcp_lifeline = dbus.types.UnixFd(self.tcp_r)
            ret = pb.RequestTcpPortAccess(dbus.UInt16(self._PORT), "",
                                          tcp_lifeline)
            # |ret| is a dbus.Boolean, but compares as int.
            if ret == 0:
                raise error.TestFail("RequestTcpPortAccess returned false.")

            udp_lifeline = dbus.types.UnixFd(self.udp_r)
            ret = pb.RequestUdpPortAccess(dbus.UInt16(self._PORT), "",
                                          udp_lifeline)
            # |ret| is a dbus.Boolean, but compares as int.
            if ret == 0:
                raise error.TestFail("RequestUdpPortAccess returned false.")

            iface_lifeline = dbus.types.UnixFd(self.iface_r)
            ret = pb.RequestTcpPortAccess(dbus.UInt16(self._PORT),
                                          dbus.String(self._IFACE),
                                          iface_lifeline)
            # |ret| is a dbus.Boolean, but compares as int.
            if ret == 0:
                raise error.TestFail(
                        "RequestTcpPortAccess(port, interface) returned false.")

            # Test IPv4 and IPv6.
            for executable in ["iptables", "ip6tables"]:
                actual_rules = self._iptables_rules(executable)
                self._check_included(
                        self._TCP_RULE, actual_rules,
                        "RequestTcpPortAccess did not add %s rule.",
                        executable)
                self._check_included(
                        self._UDP_RULE, actual_rules,
                        "RequestUdpPortAccess did not add %s rule.",
                        executable)
                self._check_included(
                        self._IFACE_RULE, actual_rules,
                        "RequestTcpPortAccess(port, interface)"
                        " did not add %s rule.",
                        executable)

            ret = pb.ReleaseTcpPort(dbus.UInt16(self._PORT), "")
            # |ret| is a dbus.Boolean, but compares as int.
            if ret == 0:
                raise error.TestFail("ReleaseTcpPort returned false.")

            ret = pb.ReleaseUdpPort(dbus.UInt16(self._PORT), "")
            # |ret| is a dbus.Boolean, but compares as int.
            if ret == 0:
                raise error.TestFail("ReleaseUdpPort returned false.")

            # Test IPv4 and IPv6.
            for executable in ["iptables", "ip6tables"]:
                rules = self._iptables_rules(executable)
                self._check_not_included(
                        self._TCP_RULE, rules,
                        "ReleaseTcpPortAccess did not remove %s rule.",
                        executable)
                self._check_not_included(
                        self._UDP_RULE, rules,
                        "ReleaseUdpPortAccess did not remove %s rule.",
                        executable)

            # permission_broker should plug the firewall hole
            # when the requesting process exits.
            # Simulate the process exiting by closing |iface_w|.
            os.close(self.iface_w)

            # permission_broker checks every |_POLL_INTERVAL| seconds
            # for processes that have exited.
            # This is ugly, but it's either this or polling /var/log/messages.
            time.sleep(self._POLL_INTERVAL + 1)
            # Test IPv4 and IPv6.
            for executable in ["iptables", "ip6tables"]:
                rules = self._iptables_rules(executable)
                self._check_not_included(
                        self._IFACE_RULE, rules,
                        "permission_broker did not remove %s rule.",
                        executable)

        except dbus.DBusException as e:
            raise error.TestFail("D-Bus error: " + e.get_dbus_message())


    def cleanup(self):
        # File descriptors could already be closed.
        try:
            os.close(self.tcp_w)
            os.close(self.udp_w)
            os.close(self.iface_w)
        except OSError:
            pass

        # We don't want the cleanup() method to fail, so we ignore exit codes.
        # This also allows us to clean up iptables rules unconditionally.
        # The command will fail if the rule has already been deleted,
        # but it won't fail the test.
        for executable in ["iptables", "ip6tables"]:
            cmd = self._IPTABLES_DEL_CMD % (executable, "tcp", "tcp",
                                            self._PORT)
            utils.system(cmd, ignore_status=True)
            cmd = self._IPTABLES_DEL_CMD % (executable, "udp", "udp",
                                            self._PORT)
            utils.system(cmd, ignore_status=True)
            cmd = self._IPTABLES_DEL_CMD % (executable, "tcp", "tcp",
                                            self._PORT)
            cmd += " -i %s" % self._IFACE
            utils.system(cmd, ignore_status=True)
