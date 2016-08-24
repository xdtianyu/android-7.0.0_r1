#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cmd
import dbus
import dbus.exceptions
import dbus.mainloop.glib
import gobject
import threading

from functools import wraps


DBUS_ERROR = 'org.freedesktop.DBus.Error'
NEARD_PATH = '/org/neard/'
PROMPT = 'NFC> '

class NfcClientException(Exception):
    """Exception class for exceptions thrown by NfcClient."""


def print_message(message, newlines=2):
    """
    Prints the given message with extra wrapping newline characters.

    @param message: Message to print.
    @param newlines: Integer, specifying the number of '\n' characters that
            should be padded at the beginning and end of |message| before
            being passed to "print".

    """
    padding = newlines * '\n'
    message = padding + message + padding
    print message


def handle_errors(func):
    """
    Decorator for handling exceptions that are commonly raised by many of the
    methods in NfcClient.

    @param func: The function this decorator is wrapping.

    """
    @wraps(func)
    def _error_handler(*args):
        try:
            return func(*args)
        except dbus.exceptions.DBusException as e:
            if e.get_dbus_name() == DBUS_ERROR + '.ServiceUnknown':
                print_message('neard may have crashed or disappeared. '
                              'Check if neard is running and run "initialize" '
                              'from this shell.')
                return
            if e.get_dbus_name() == DBUS_ERROR + '.UnknownObject':
                print_message('Could not find object.')
                return
            print_message(str(e))
        except Exception as e:
            print_message(str(e))
    return _error_handler


class NfcClient(object):
    """
    neard D-Bus client

    """
    NEARD_SERVICE_NAME = 'org.neard'
    IMANAGER = NEARD_SERVICE_NAME + '.Manager'
    IADAPTER = NEARD_SERVICE_NAME + '.Adapter'
    ITAG = NEARD_SERVICE_NAME + '.Tag'
    IRECORD = NEARD_SERVICE_NAME + '.Record'
    IDEVICE = NEARD_SERVICE_NAME + '.Device'

    def __init__(self):
        self._mainloop = None
        self._mainloop_thread = None
        self._adapters = {}
        self._adapter_property_handler_matches = {}

    def begin(self):
        """
        Starts the D-Bus client.

        """
        # Here we run a GLib MainLoop in its own thread, so that the client can
        # listen to D-Bus signals while keeping the console interactive.
        self._dbusmainloop = dbus.mainloop.glib.DBusGMainLoop(
                set_as_default=True)
        dbus.mainloop.glib.threads_init()
        gobject.threads_init()

        def _mainloop_thread_func():
            self._mainloop = gobject.MainLoop()
            context = self._mainloop.get_context()
            self._run_loop = True
            while self._run_loop:
                context.iteration(True)
        self._mainloop_thread = threading.Thread(None, _mainloop_thread_func)
        self._mainloop_thread.start()

        self._bus = dbus.SystemBus()
        self.setup_manager()

    def end(self):
        """
        Stops the D-Bus client.

        """
        self._run_loop = False
        self._mainloop.quit()
        self._mainloop_thread.join()

    def restart(self):
        """Reinitializes the NFC client."""
        self.setup_manager()

    @handle_errors
    def _get_manager_proxy(self):
        return dbus.Interface(
                self._bus.get_object(self.NEARD_SERVICE_NAME, '/'),
                self.IMANAGER)

    @handle_errors
    def _get_adapter_proxy(self, adapter):
        return dbus.Interface(
                self._bus.get_object(self.NEARD_SERVICE_NAME, adapter),
                self.IADAPTER)

    def _get_cached_adapter_proxy(self, adapter):
        adapter_proxy = self._adapters.get(adapter, None)
        if not adapter_proxy:
            raise NfcClientException('Adapter "' + adapter + '" not found.')
        return adapter_proxy


    @handle_errors
    def _get_tag_proxy(self, tag):
        return dbus.Interface(
                self._bus.get_object(self.NEARD_SERVICE_NAME, tag),
                self.ITAG)

    @handle_errors
    def _get_device_proxy(self, device):
        return dbus.Interface(
                self._bus.get_object(self.NEARD_SERVICE_NAME, device),
                self.IDEVICE)

    @handle_errors
    def _get_record_proxy(self, record):
        return dbus.Interface(
                self._bus.get_object(self.NEARD_SERVICE_NAME, record),
                self.IRECORD)

    @handle_errors
    def _get_adapter_properties(self, adapter):
        adapter_proxy = self._get_cached_adapter_proxy(adapter)
        return adapter_proxy.GetProperties()

    def _get_adapters(self):
        props = self._manager.GetProperties()
        return props.get('Adapters', None)

    def setup_manager(self):
        """
        Creates a manager proxy and subscribes to adapter signals. This method
        will also initialize proxies for adapters if any are available.

        """
        # Create the manager proxy.
        self._adapters.clear()
        self._manager = self._get_manager_proxy()
        if not self._manager:
            print_message('Failed to create a proxy to the Manager interface.')
            return

        # Listen to the adapter added and removed signals.
        self._manager.connect_to_signal(
                'AdapterAdded',
                lambda adapter: self.register_adapter(str(adapter)))
        self._manager.connect_to_signal(
                'AdapterRemoved',
                lambda adapter: self.unregister_adapter(str(adapter)))

        # See if there are any adapters and create proxies for each.
        adapters = self._get_adapters()
        if adapters:
            for adapter in adapters:
                self.register_adapter(adapter)

    def register_adapter(self, adapter):
        """
        Registers an adapter proxy with the given object path and subscribes to
        adapter signals.

        @param adapter: string, containing the adapter's D-Bus object path.

        """
        print_message('Added adapter: ' + adapter)
        adapter_proxy = self._get_adapter_proxy(adapter)
        self._adapters[adapter] = adapter_proxy

        # Tag found/lost currently don't get fired. Monitor property changes
        # instead.
        if self._adapter_property_handler_matches.get(adapter, None) is None:
            self._adapter_property_handler_matches[adapter] = (
                    adapter_proxy.connect_to_signal(
                            'PropertyChanged',
                            (lambda name, value:
                                    self._adapter_property_changed_signal(
                                            adapter, name, value))))

    def unregister_adapter(self, adapter):
        """
        Removes the adapter proxy for the given object path from the internal
        cache of adapters.

        @param adapter: string, containing the adapter's D-Bus object path.

        """
        print_message('Removed adapter: ' + adapter)
        match = self._adapter_property_handler_matches.get(adapter, None)
        if match is not None:
            match.remove()
            self._adapter_property_handler_matches.pop(adapter)
        self._adapters.pop(adapter)

    def _adapter_property_changed_signal(self, adapter, name, value):
        if name == 'Tags' or name == 'Devices':
            print_message('Found ' + name + ': ' +
                          self._dbus_array_to_string(value))

    @handle_errors
    def show_adapters(self):
        """
        Prints the D-Bus object paths of all adapters that are available.

        """
        adapters = self._get_adapters()
        if not adapters:
            print_message('No adapters found.')
            return
        for adapter in adapters:
            print_message('  ' + str(adapter), newlines=0)
        print

    def _dbus_array_to_string(self, array):
        string = '[ '
        for value in array:
            string += ' ' + str(value) + ', '
        string += ' ]'
        return string

    def print_adapter_status(self, adapter):
        """
        Prints the properties of the given adapter.

        @param adapter: string, containing the adapter's D-Bus object path.

        """
        props = self._get_adapter_properties(adapter)
        if not props:
            return
        print_message('Status ' + adapter + ': ', newlines=0)
        for key, value in props.iteritems():
            if type(value) == dbus.Array:
                value = self._dbus_array_to_string(value)
            else:
                value = str(value)
            print_message('  ' + key + ' = ' + value, newlines=0)
        print

    @handle_errors
    def set_powered(self, adapter, powered):
        """
        Enables or disables the adapter.

        @param adapter: string, containing the adapter's D-Bus object path.
        @param powered: boolean that dictates whether the adapter will be
                enabled or disabled.

        """
        adapter_proxy = self._get_cached_adapter_proxy(adapter)
        if not adapter_proxy:
            return
        adapter_proxy.SetProperty('Powered', powered)

    @handle_errors
    def start_polling(self, adapter):
        """
        Starts polling for nearby tags and devices in "Initiator" mode.

        @param adapter: string, containing the adapter's D-Bus object path.

        """
        adapter_proxy = self._get_cached_adapter_proxy(adapter)
        adapter_proxy.StartPollLoop('Initiator')
        print_message('Started polling.')

    @handle_errors
    def stop_polling(self, adapter):
        """
        Stops polling for nearby tags and devices.

        @param adapter: string, containing the adapter's D-Bus object path.

        """
        adapter_proxy = self._get_cached_adapter_proxy(adapter)
        adapter_proxy.StopPollLoop()
        self._polling_stopped = True
        print_message('Stopped polling.')

    @handle_errors
    def show_tag_data(self, tag):
        """
        Prints the properties of the given tag, as well as the contents of any
        records associated with it.

        @param tag: string, containing the tag's D-Bus object path.

        """
        tag_proxy = self._get_tag_proxy(tag)
        if not tag_proxy:
            print_message('Tag "' + tag + '" not found.')
            return
        props = tag_proxy.GetProperties()
        print_message('Tag ' + tag + ': ', newlines=1)
        for key, value in props.iteritems():
            if key != 'Records':
                print_message('  ' + key + ' = ' + str(value), newlines=0)
        records = props['Records']
        if not records:
            return
        print_message('Records: ', newlines=1)
        for record in records:
            self.show_record_data(str(record))
        print

    @handle_errors
    def show_device_data(self, device):
        """
        Prints the properties of the given device, as well as the contents of
        any records associated with it.

        @param device: string, containing the device's D-Bus object path.

        """
        device_proxy = self._get_device_proxy(device)
        if not device_proxy:
            print_message('Device "' + device + '" not found.')
            return
        records = device_proxy.GetProperties()['Records']
        if not records:
            print_message('No records on device.')
            return
        print_message('Records: ', newlines=1)
        for record in records:
            self.show_record_data(str(record))
        print

    @handle_errors
    def show_record_data(self, record):
        """
        Prints the contents of the given record.

        @param record: string, containing the record's D-Bus object path.

        """
        record_proxy = self._get_record_proxy(record)
        if not record_proxy:
            print_message('Record "' + record + '" not found.')
            return
        props = record_proxy.GetProperties()
        print_message('Record ' + record + ': ', newlines=1)
        for key, value in props.iteritems():
            print '  ' + key + ' = ' + value
        print

    def _create_record_data(self, record_type, params):
        if record_type == 'Text':
            possible_keys = [ 'Encoding', 'Language', 'Representation' ]
            tag_data = { 'Type': 'Text' }
        elif record_type == 'URI':
            possible_keys = [ 'URI' ]
            tag_data = { 'Type': 'URI' }
        else:
            print_message('Writing record type "' + record_type +
                          '" currently not supported.')
            return None
        for key, value in params.iteritems():
            if key in possible_keys:
                tag_data[key] = value
        return tag_data

    @handle_errors
    def write_tag(self, tag, record_type, params):
        """
        Writes an NDEF record to the given tag.

        @param tag: string, containing the tag's D-Bus object path.
        @param record_type: The type of the record, e.g. Text or URI.
        @param params: dictionary, containing the parameters of the NDEF.

        """
        tag_data = self._create_record_data(record_type, params)
        if not tag_data:
            return
        tag_proxy = self._get_tag_proxy(tag)
        if not tag_proxy:
            print_message('Tag "' + tag + '" not found.')
            return
        tag_proxy.Write(tag_data)
        print_message('Tag written!')

    @handle_errors
    def push_to_device(self, device, record_type, params):
        """
        Pushes an NDEF record to the given device.

        @param device: string, containing the device's D-Bus object path.
        @param record_type: The type of the record, e.g. Text or URI.
        @param params: dictionary, containing the parameters of the NDEF.

        """
        record_data = self._create_record_data(record_type, params)
        if not record_data:
            return
        device_proxy = self._get_device_proxy(device)
        if not device_proxy:
            print_message('Device "' + device + '" not found.')
            return
        device_proxy.Push(record_data)
        print_message('NDEF pushed to device!')


class NfcConsole(cmd.Cmd):
    """
    Interactive console to interact with the NFC daemon.

    """
    def __init__(self):
        cmd.Cmd.__init__(self)
        self.prompt = PROMPT

    def begin(self):
        """
        Starts the interactive shell.

        """
        print_message('NFC console! Run "help" for a list of commands.',
                      newlines=1)
        self._nfc_client = NfcClient()
        self._nfc_client.begin()
        self.cmdloop()

    def can_exit(self):
        """Override"""
        return True

    def do_initialize(self, args):
        """Handles "initialize"."""
        if args:
            print_message('Command "initialize" expects no arguments.')
            return
        self._nfc_client.restart()

    def help_initialize(self):
        """Prints the help message for "initialize"."""
        print_message('Initializes the neard D-Bus client. This can be '
                      'run many times to restart the client in case of '
                      'neard failures or crashes.')

    def do_adapters(self, args):
        """Handles "adapters"."""
        if args:
            print_message('Command "adapters" expects no arguments.')
            return
        self._nfc_client.show_adapters()

    def help_adapters(self):
        """Prints the help message for "adapters"."""
        print_message('Displays the D-Bus object paths of the available '
                      'adapter objects.')

    def do_adapter_status(self, args):
        """Handles "adapter_status"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage: adapter_status <adapter>')
            return
        self._nfc_client.print_adapter_status(NEARD_PATH + args[0])

    def help_adapter_status(self):
        """Prints the help message for "adapter_status"."""
        print_message('Returns the properties of the given NFC adapter.\n\n'
                      '    Ex: "adapter_status nfc0"')

    def do_enable_adapter(self, args):
        """Handles "enable_adapter"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage: enable_adapter <adapter>')
            return
        self._nfc_client.set_powered(NEARD_PATH + args[0], True)

    def help_enable_adapter(self):
        """Prints the help message for "enable_adapter"."""
        print_message('Powers up the adapter. Ex: "enable_adapter nfc0"')

    def do_disable_adapter(self, args):
        """Handles "disable_adapter"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage: disable_adapter <adapter>')
            return
        self._nfc_client.set_powered(NEARD_PATH + args[0], False)

    def help_disable_adapter(self):
        """Prints the help message for "disable_adapter"."""
        print_message('Powers down the adapter. Ex: "disable_adapter nfc0"')

    def do_start_poll(self, args):
        """Handles "start_poll"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage: start_poll <adapter>')
            return
        self._nfc_client.start_polling(NEARD_PATH + args[0])

    def help_start_poll(self):
        """Prints the help message for "start_poll"."""
        print_message('Initiates a poll loop.\n\n    Ex: "start_poll nfc0"')

    def do_stop_poll(self, args):
        """Handles "stop_poll"."""
        args = args.split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage: stop_poll <adapter>')
            return
        self._nfc_client.stop_polling(NEARD_PATH + args[0])

    def help_stop_poll(self):
        """Prints the help message for "stop_poll"."""
        print_message('Stops a poll loop.\n\n    Ex: "stop_poll nfc0"')

    def do_read_tag(self, args):
        """Handles "read_tag"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage read_tag <tag>')
            return
        self._nfc_client.show_tag_data(NEARD_PATH + args[0])

    def help_read_tag(self):
        """Prints the help message for "read_tag"."""
        print_message('Reads the contents of a tag.  Ex: read_tag nfc0/tag0')

    def _parse_record_args(self, record_type, args):
        if record_type == 'Text':
            if len(args) < 5:
                print_message('Usage: write_tag <tag> Text <encoding> '
                              '<language> <representation>')
                return None
            if args[2] not in [ 'UTF-8', 'UTF-16' ]:
                print_message('Encoding must be one of "UTF-8" or "UTF-16".')
                return None
            return {
                'Encoding': args[2],
                'Language': args[3],
                'Representation': ' '.join(args[4:])
            }
        if record_type == 'URI':
            if len(args) != 3:
                print_message('Usage: write_tag <tag> URI <uri>')
                return None
            return {
                'URI': args[2]
            }
        print_message('Only types "Text" and "URI" are supported by this '
                      'script.')
        return None

    def do_write_tag(self, args):
        """Handles "write_tag"."""
        args = args.strip().split(' ')
        if len(args) < 3:
            print_message('Usage: write_tag <tag> [params]')
            return
        record_type = args[1]
        params = self._parse_record_args(record_type, args)
        if not params:
            return
        self._nfc_client.write_tag(NEARD_PATH + args[0],
                                   record_type, params)

    def help_write_tag(self):
        """Prints the help message for "write_tag"."""
        print_message('Writes the given data to a tag. Usage:\n'
                      '  write_tag <tag> Text <encoding> <language> '
                      '<representation>\n  write_tag <tag> URI <uri>')

    def do_read_device(self, args):
        """Handles "read_device"."""
        args = args.strip().split(' ')
        if len(args) != 1 or not args[0]:
            print_message('Usage read_device <device>')
            return
        self._nfc_client.show_device_data(NEARD_PATH + args[0])

    def help_read_device(self):
        """Prints the help message for "read_device"."""
        print_message('Reads the contents of a device.  Ex: read_device '
                      'nfc0/device0')

    def do_push_to_device(self, args):
        """Handles "push_to_device"."""
        args = args.strip().split(' ')
        if len(args) < 3:
            print_message('Usage: push_to_device <device> [params]')
            return
        record_type = args[1]
        params = self._parse_record_args(record_type, args)
        if not params:
            return
        self._nfc_client.push_to_device(NEARD_PATH + args[0],
                                        record_type, params)

    def help_push_to_device(self):
        """Prints the help message for "push_to_device"."""
        print_message('Pushes the given data to a device. Usage:\n'
                      '  push_to_device <device> Text <encoding> <language> '
                      '<representation>\n  push_to_device <device> URI <uri>')

    def do_exit(self, args):
        """
        Handles the 'exit' command.

        @param args: Arguments to the command. Unused.

        """
        if args:
            print_message('Command "exit" expects no arguments.')
            return
        resp = raw_input('Are you sure? (yes/no): ')
        if resp == 'yes':
            print_message('Goodbye!')
            self._nfc_client.end()
            return True
        if resp != 'no':
            print_message('Did not understand: ' + resp)
        return False

    def help_exit(self):
        """Handles the 'help exit' command."""
        print_message('Exits the console.')

    do_EOF = do_exit
    help_EOF = help_exit


def main():
    """Main function."""
    NfcConsole().begin()


if __name__ == '__main__':
    main()
