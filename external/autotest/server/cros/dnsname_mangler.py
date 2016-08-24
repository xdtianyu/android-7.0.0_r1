# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import socket

from autotest_lib.client.common_lib import error


# See server/cros/network/wifi_test_context_manager.py for commandline
# flags to control IP addresses in WiFi tests.
DEFAULT_FAILURE_MESSAGE = (
        'Cannot infer DNS name of companion device from an IP address.')
ATTENUATOR_FAILURE_MESSAGE = (
        'Cannot infer DNS name of WiFi variable attenuator from a client IP '
        'address.  Use --atten_addr=<ip or dns name>')
BLUETOOTH_TESTER_FAILURE_MESSAGE = (
        'Remote host cannot be an IP address unless tester specified with '
         '--args tester=IP')
ROUTER_FAILURE_MESSAGE = (
        'Cannot infer DNS name of WiFi router from a client IP address.')


def is_ip_address(hostname):
    """Infers whether |hostname| could be an IP address.

    @param hostname: string DNS name or IP address.
    @return True iff hostname is a valid IP address.

    """
    try:
        socket.inet_aton(hostname)
        return True
    except socket.error:
        return False


def get_companion_device_addr(client_hostname,
                              suffix,
                              cmdline_override=None,
                              not_dnsname_msg=DEFAULT_FAILURE_MESSAGE,
                              allow_failure=False):
    """Build a usable hostname for a test companion device from the client name.

    Optionally, override the generated name with a commandline provided version.

    @param client_hostname: string DNS name of device under test (the client).
    @param suffix: string suffix to append to the client hostname.
    @param cmdline_override: optional DNS name of companion device.  If this is
            given, it overrides the generated client based hostname.
    @param not_dnsname_msg: string message to include in the exception raised
            if the client hostname is found to be an IP address rather than a
            DNS name.
    @param allow_failure: boolean True iff we should return None on failure to
            infer a DNS name.
    @return string DNS name of companion device or None if |allow_failure|
            is True and no DNS name can be inferred.

    """
    if cmdline_override is not None:
        return cmdline_override
    if is_ip_address(client_hostname):
        logging.error('%r looks like an IP address?', client_hostname)
        if allow_failure:
            return None
        raise error.TestError(not_dnsname_msg)
    parts = client_hostname.split('.', 1)
    parts[0] = parts[0] + suffix
    return '.'.join(parts)


def get_router_addr(client_hostname, cmdline_override=None):
    """Build a hostname for a WiFi router from the client hostname.

    Optionally override that hostname with the provided command line hostname.

    @param client_hostname: string DNS name of the client.
    @param cmdline_override: string DNS name of the router provided
            via commandline arguments.
    @return usable DNS name for router host.

    """
    return get_companion_device_addr(
            client_hostname,
            '-router',
            cmdline_override=cmdline_override,
            not_dnsname_msg=ROUTER_FAILURE_MESSAGE)


def get_attenuator_addr(client_hostname,
                        cmdline_override=None,
                        allow_failure=False):
    """Build a hostname for a WiFi variable attenuator from the client hostname.

    Optionally override that hostname with the provided command line hostname.

    @param client_hostname: string DNS name of the client.
    @param cmdline_override: string DNS name of the variable attenuator
            controller provided via commandline arguments.
    @param allow_failure: boolean True iff we should return None on failure to
            infer a DNS name.
    @return usable DNS name for attenuator controller.

    """
    return get_companion_device_addr(
            client_hostname,
            '-attenuator',
            cmdline_override=cmdline_override,
            not_dnsname_msg=ATTENUATOR_FAILURE_MESSAGE,
            allow_failure=allow_failure)


def get_tester_addr(client_hostname, cmdline_override=None):
    """Build a hostname for a Bluetooth test device from the client hostname.

    Optionally override that hostname with the provided command line hostname.

    @param client_hostname: string DNS name of the client.
    @param cmdline_override: string DNS name of the Bluetooth tester
            provided via commandline arguments.
    @return usable DNS name for Bluetooth tester device.

    """
    return get_companion_device_addr(
            client_hostname,
            '-router',
            cmdline_override=cmdline_override,
            not_dnsname_msg=BLUETOOTH_TESTER_FAILURE_MESSAGE)
