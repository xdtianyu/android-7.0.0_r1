# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ConfigParser
import io
import collections
import logging
import shlex
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dbus_send

BUS_NAME = 'org.freedesktop.Avahi'
INTERFACE_SERVER = 'org.freedesktop.Avahi.Server'

ServiceRecord = collections.namedtuple(
        'ServiceRecord',
        ['interface', 'protocol', 'name', 'record_type', 'domain',
         'hostname', 'address', 'port', 'txt'])


def avahi_config(options, src_file='/etc/avahi/avahi-daemon.conf', host=None):
    """Creates a temporary avahi-daemon.conf file with the specified changes.

    Avahi daemon uses a text configuration file with sections and values
    assigned to options on that section. This function creates a new config
    file based on the one provided and a set of changes. The changes are
    specified as triples of section, option and value that override the existing
    options on the config file. If a value of None is specified for any triplet,
    the corresponding option will be removed from the file.

    @param options: A list of triplets of the form (section, option, value).
    @param src_file: The default config file to use as a base for the changes.
    @param host: An optional host object if running against a remote host.
    @return: The filename of a temporary file with the new configuration file.

    """
    run = utils.run if host is None else host.run
    existing_config = run('cat %s 2> /dev/null' % src_file).stdout
    conf = ConfigParser.SafeConfigParser()
    conf.readfp(io.BytesIO(existing_config))

    for section, option, value in options:
        if not conf.has_section(section):
            conf.add_section(section)
        if value is None:
            conf.remove_option(section, option)
        else:
            conf.set(section, option, value)

    tmp_conf_file = run('mktemp -t avahi-conf.XXXX').stdout.strip()
    lines = []
    for section in conf.sections():
        lines.append('[%s]' % section)
        for option in conf.options(section):
            lines.append('%s=%s' % (option, conf.get(section, option)))
    run('cat <<EOF >%s\n%s\nEOF\n' % (tmp_conf_file, '\n'.join(lines)))
    return tmp_conf_file


def avahi_ping(host=None):
    """Returns True when the avahi-deamon's DBus interface is ready.

    After your launch avahi-daemon, there is a short period of time where the
    daemon is running but the DBus interface isn't ready yet. This functions
    blocks for a few seconds waiting for a ping response from the DBus API
    and returns wether it got a response.

    @param host: An optional host object if running against a remote host.
    @return boolean: True if Avahi is up and in a stable state.

    """
    result = dbus_send.dbus_send(BUS_NAME, INTERFACE_SERVER, '/', 'GetState',
                                 host=host, timeout_seconds=2,
                                 tolerate_failures=True)
    # AVAHI_ENTRY_GROUP_ESTABLISHED == 2
    return result is not None and result.response == 2


def avahi_start(config_file=None, host=None):
    """Start avahi-daemon with the provided config file.

    This function waits until the avahi-daemon is ready listening on the DBus
    interface. If avahi fails to be ready after 10 seconds, an error is raised.

    @param config_file: The filename of the avahi-daemon config file or None to
            use the default.
    @param host: An optional host object if running against a remote host.

    """
    run = utils.run if host is None else host.run
    env = ''
    if config_file is not None:
        env = ' AVAHI_DAEMON_CONF="%s"' % config_file
    run('start avahi %s' % env, ignore_status=False)
    # Wait until avahi is ready.
    deadline = time.time() + 10.
    while time.time() < deadline:
        if avahi_ping(host=host):
            return
        time.sleep(0.1)
    raise error.TestError('avahi-daemon is not ready after 10s running.')


def avahi_stop(ignore_status=False, host=None):
    """Stop the avahi daemon.

    @param ignore_status: True to ignore failures while stopping avahi.
    @param host: An optional host object if running against a remote host.

    """
    run = utils.run if host is None else host.run
    run('stop avahi', ignore_status=ignore_status)


def avahi_start_on_iface(iface, host=None):
    """Starts avahi daemon listening only on the provided interface.

    @param iface: A string with the interface name.
    @param host: An optional host object if running against a remote host.

    """
    run = utils.run if host is None else host.run
    opts = [('server', 'allow-interfaces', iface),
            ('server', 'deny-interfaces', None)]
    conf = avahi_config(opts, host=host)
    avahi_start(config_file=conf, host=host)
    run('rm %s' % conf)


def avahi_get_hostname(host=None):
    """Get the lan-unique hostname of the the device.

    @param host: An optional host object if running against a remote host.
    @return string: the lan-unique hostname of the DUT.

    """
    result = dbus_send.dbus_send(
            BUS_NAME, INTERFACE_SERVER, '/', 'GetHostName',
            host=host, timeout_seconds=2, tolerate_failures=True)
    return None if result is None else result.response


def avahi_get_domain_name(host=None):
    """Get the current domain name being used by Avahi.

    @param host: An optional host object if running against a remote host.
    @return string: the current domain name being used by Avahi.

    """
    result = dbus_send.dbus_send(
            BUS_NAME, INTERFACE_SERVER, '/', 'GetDomainName',
            host=host, timeout_seconds=2, tolerate_failures=True)
    return None if result is None else result.response


def avahi_browse(host=None, ignore_local=True):
    """Browse mDNS service records with avahi-browse.

    Some example avahi-browse output (lines are wrapped for readability):

    localhost ~ # avahi-browse -tarlp
    +;eth1;IPv4;E58E8561-3BCA-4910-ABC7-BD8779D7D761;_serbus._tcp;local
    +;eth1;IPv4;E58E8561-3BCA-4910-ABC7-BD8779D7D761;_privet._tcp;local
    =;eth1;IPv4;E58E8561-3BCA-4910-ABC7-BD8779D7D761;_serbus._tcp;local;\
        9bcd92bbc1f91f2ee9c9b2e754cfd22e.local;172.22.23.237;0;\
        "ver=1.0" "services=privet" "id=11FB0AD6-6C87-433E-8ACB-0C68EE78CDBD"
    =;eth1;IPv4;E58E8561-3BCA-4910-ABC7-BD8779D7D761;_privet._tcp;local;\
        9bcd92bbc1f91f2ee9c9b2e754cfd22e.local;172.22.23.237;8080;\
        "ty=Unnamed Device" "txtvers=3" "services=_camera" "model_id=///" \
        "id=FEE9B312-1F2B-4B9B-813C-8482FA75E0DB" "flags=AB" "class=BB"

    @param host: An optional host object if running against a remote host.
    @param ignore_local: boolean True to ignore local service records.
    @return list of ServiceRecord objects parsed from output.

    """
    run = utils.run if host is None else host.run
    flags = ['--terminate',  # Terminate after looking for a short time.
             '--all',  # Show all services, regardless of type.
             '--resolve',  # Resolve the services discovered.
             '--parsable',  # Print service records in a parsable format.
    ]
    if ignore_local:
        flags.append('--ignore-local')
    result = run('avahi-browse %s' % ' '.join(flags))
    records = []
    for line in result.stdout.strip().splitlines():
        parts = line.split(';')
        if parts[0] == '+':
            # Skip it, just parse the resolved record.
            continue
        # Do minimal parsing of the TXT record.
        parts[-1] = shlex.split(parts[-1])
        records.append(ServiceRecord(*parts[1:]))
        logging.debug('Found %r', records[-1])
    return records
