# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dbus
import dbus.service
import dbus.mainloop.glib
import gobject
import logging
import os
import threading
import time

""" MockLorgnette provides mocked methods from the lorgnette
    D-Bus API so that we can perform an image scan operation in
    Chrome without access to a physical scanner. """

MethodCall = collections.namedtuple("MethodCall", ["method", "argument"])

class LorgnetteManager(dbus.service.Object):
    """ The lorgnette DBus Manager object instance.  Methods in this
        object are called whenever a DBus RPC method is invoked. """

    SCANNER_NAME = 'scanner1'
    SCANNER_MANUFACTURER = 'Chromascanner'
    SCANNER_MODEL = 'Fakebits2000'
    SCANNER_TYPE = 'Virtual'

    def __init__(self, bus, object_path, scan_image_data):
        dbus.service.Object.__init__(self, bus, object_path)
        self.method_calls = []
        self.scan_image_data = scan_image_data


    @dbus.service.method('org.chromium.lorgnette.Manager',
                         in_signature='', out_signature='a{sa{ss}}')
    def ListScanners(self):
        """Lists available scanners. """
        self.add_method_call('ListScanners', '')
        return { self.SCANNER_NAME: {
                       'Manufacturer': self.SCANNER_MANUFACTURER,
                       'Model': self.SCANNER_MODEL,
                       'Type': self.SCANNER_TYPE }}


    @dbus.service.method('org.chromium.lorgnette.Manager',
                         in_signature='sha{sv}', out_signature='')
    def ScanImage(self, device, out_fd, scan_properties):
        """Writes test image date to |out_fd|.  Do so in chunks since the
        entire dataset cannot be successfully written at once.

        @param device string name of the device to scan from.
        @param out_fd file handle for the output scan data.
        @param scan_properties dict containing parameters for the scan.

        """
        self.add_method_call('ScanImage', (device, scan_properties))
        scan_output_fd = out_fd.take()
        os.write(scan_output_fd, self.scan_image_data)
        os.close(scan_output_fd)

        # TODO(pstew): Ensure the timing between return of this method
        # and the EOF returned to Chrome at the end of this data stream
        # are distinct.  This comes naturally with a real scanner.
        time.sleep(1)


    def add_method_call(self, method, arg):
        """Note that a method call was made.

        @param method string the method that was called.
        @param arg tuple list of arguments that were called on |method|.

        """
        logging.info("Mock Lorgnette method %s called with argument %s",
                     method, arg)
        self.method_calls.append(MethodCall(method, arg))


    def get_method_calls(self):
        """Provide the method call list, clears this list internally.

        @return list of MethodCall objects

        """
        method_calls = self.method_calls
        self.method_calls = []
        return method_calls


class MockLorgnette(threading.Thread):
    """This thread object instantiates a mock lorgnette manager and
    runs a mainloop that receives DBus API messages. """
    LORGNETTE = "org.chromium.lorgnette"
    def __init__(self, image_file):
        threading.Thread.__init__(self)
        gobject.threads_init()
        self.image_file = image_file


    def __enter__(self):
        self.start()
        return self


    def __exit__(self, type, value, tb):
        self.quit()
        self.join()


    def run(self):
        """Runs the main loop."""
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus()
        name = dbus.service.BusName(self.LORGNETTE, self.bus)
        with open(self.image_file) as f:
            self.image_data = f.read()
        self.manager = LorgnetteManager(
                self.bus, '/org/chromium/lorgnette/Manager', self.image_data)
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()


    def quit(self):
        """Quits the main loop."""
        self.mainloop.quit()


    def get_method_calls(self):
        """Returns the method calls that were called on the mock object.

        @return list of MethodCall objects representing the methods called.

         """
        return self.manager.get_method_calls()


if __name__ == '__main__':
    MockLorgnette().run()
