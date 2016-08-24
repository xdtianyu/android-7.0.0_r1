# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dbus
import logging
import pipes
import re
import shlex

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


# Represents the result of a dbus-send call.  |sender| refers to the temporary
# bus name of dbus-send, |responder| to the remote process, and |response|
# contains the parsed response.
DBusSendResult = collections.namedtuple('DBusSendResult', ['sender',
                                                           'responder',
                                                           'response'])
# Used internally.
DictEntry = collections.namedtuple('DictEntry', ['key', 'value'])


def _build_token_stream(headerless_dbus_send_output):
    """A tokenizer for dbus-send output.

    The output is basically just like splitting on whitespace, except that
    strings are kept together by " characters.

    @param headerless_dbus_send_output: list of lines of dbus-send output
            without the meta-information prefix.
    @return list of tokens in dbus-send output.
    """
    return shlex.split(' '.join(headerless_dbus_send_output))


def _parse_value(token_stream):
    """Turn a stream of tokens from dbus-send output into native python types.

    @param token_stream: output from _build_token_stream() above.

    """
    # Assumes properly tokenized output (strings with spaces handled).
    # Assumes tokens are pre-stripped
    token_type = token_stream.pop(0)
    if token_type == 'variant':
        token_type = token_stream.pop(0)
    if token_type == 'object':
        token_type = token_stream.pop(0)  # Should be 'path'
    token_value = token_stream.pop(0)
    INT_TYPES = ('int16', 'uint16', 'int32', 'uint32',
                 'int64', 'uint64', 'byte')
    if token_type in INT_TYPES:
        return int(token_value)
    if token_type == 'string' or token_type == 'path':
        return token_value  # shlex removed surrounding " chars.
    if token_type == 'boolean':
        return token_value == 'true'
    if token_type == 'double':
        return float(token_value)
    if token_type == 'array':
        values = []
        while token_stream[0] != ']':
            values.append(_parse_value(token_stream))
        token_stream.pop(0)
        if values and all([isinstance(x, DictEntry) for x in values]):
            values = dict(values)
        return values
    if token_type == 'dict':
        assert token_value == 'entry('
        key = _parse_value(token_stream)
        value = _parse_value(token_stream)
        assert token_stream.pop(0) == ')'
        return DictEntry(key=key, value=value)
    raise error.TestError('Unhandled DBus type found: %s' % token_type)


def _parse_dbus_send_output(dbus_send_stdout):
    """Turn dbus-send output into usable Python types.

    This looks like:

    localhost ~ # dbus-send --system --dest=org.chromium.flimflam \
            --print-reply --reply-timeout=2000 / \
            org.chromium.flimflam.Manager.GetProperties
    method return sender=:1.12 -> dest=:1.37 reply_serial=2
       array [
          dict entry(
             string "ActiveProfile"
             variant             string "/profile/default"
          )
          dict entry(
             string "ArpGateway"
             variant             boolean true
          )
          ...
       ]

    @param dbus_send_output: string stdout from dbus-send
    @return a DBusSendResult.

    """
    lines = dbus_send_stdout.strip().splitlines()
    # The first line contains meta-information about the response
    header = lines[0]
    lines = lines[1:]
    dbus_address_pattern = '[:\d\\.]+'
    match = re.match('method return sender=(%s) -> dest=(%s) reply_serial=\d+' %
                     (dbus_address_pattern, dbus_address_pattern), header)
    if match is None:
        raise error.TestError('Could not parse dbus-send header: %s' % header)

    sender = match.group(1)
    responder = match.group(2)
    token_stream = _build_token_stream(lines)
    ret_val = _parse_value(token_stream)
    # Note that DBus permits multiple response values, and this is not handled.
    logging.debug('Got DBus response: %r', ret_val)
    return DBusSendResult(sender=sender, responder=responder, response=ret_val)


def _build_arg_string(raw_args):
    """Construct a string of arguments to a DBus method as dbus-send expects.

    @param raw_args list of dbus.* type objects to seriallize.
    @return string suitable for dbus-send.

    """
    dbus.Boolean
    int_map = {
            dbus.Int16: 'int16:',
            dbus.Int32: 'int32:',
            dbus.Int64: 'int64:',
            dbus.UInt16: 'uint16:',
            dbus.UInt32: 'uint32:',
            dbus.UInt64: 'uint64:',
            dbus.Double: 'double:',
            dbus.Byte: 'byte:',
    }
    arg_list = []
    for arg in raw_args:
        if isinstance(arg, dbus.String):
            arg_list.append(pipes.quote('string:%s' %
                                        arg.replace('"', r'\"')))
            continue
        if isinstance(arg, dbus.Boolean):
            if arg:
                arg_list.append('boolean:true')
            else:
                arg_list.append('boolean:false')
            continue
        for prim_type, prefix in int_map.iteritems():
            if isinstance(arg, prim_type):
                arg_list.append(prefix + str(arg))
                continue

        raise error.TestError('No support for serializing %r' % arg)
    return ' '.join(arg_list)


def dbus_send(bus_name, interface, object_path, method_name, args=None,
              host=None, timeout_seconds=2, tolerate_failures=False):
    """Call dbus-send without arguments.

    @param bus_name: string identifier of DBus connection to send a message to.
    @param interface: string DBus interface of object to call method on.
    @param object_path: string DBus path of remote object to call method on.
    @param method_name: string name of method to call.
    @param args: optional list of arguments.  Arguments must be of types
            from the python dbus module.
    @param host: An optional host object if running against a remote host.
    @param timeout_seconds: number of seconds to wait for a response.
    @param tolerate_failures: boolean True to ignore problems receiving a
            response.

    """
    run = utils.run if host is None else host.run
    cmd = ('dbus-send --system --print-reply --reply-timeout=%d --dest=%s '
           '%s %s.%s' % (int(timeout_seconds * 1000), bus_name,
                         object_path, interface, method_name))
    if args is not None:
        cmd = cmd + ' ' + _build_arg_string(args)
    result = run(cmd, ignore_status=tolerate_failures)
    if result.exit_status != 0:
        logging.debug('%r', result.stdout)
        return None
    return _parse_dbus_send_output(result.stdout)


def get_property(bus_name, interface, object_path, property_name, host=None):
    """A helpful wrapper that extracts the value of a DBus property.

    @param bus_name: string identifier of DBus connection to send a message to.
    @param interface: string DBus interface exposing the property.
    @param object_path: string DBus path of remote object to call method on.
    @param property_name: string name of property to get.
    @param host: An optional host object if running against a remote host.

    """
    return dbus_send(bus_name, dbus.PROPERTIES_IFACE, object_path, 'Get',
                     args=[dbus.String(interface), dbus.String(property_name)],
                     host=host)
