# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cgi
import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_test


class bluetooth_Sanity_LEDiscovery(bluetooth_test.BluetoothTest):
    """
    Verify that the client can discover the tester.
    """
    version = 1

    # How long should the tester remain discoverable?
    DISCOVERABLE_TIMEOUT=180

    def find_device(self):
        """Retrieve devices from client and look for tester.

        @return True if device has been found, False otherwise.

        """
        # Get the set of devices known to the DUT.
        devices = self.device.get_devices()
        if devices == False:
            raise error.TestFail('Could not retrieve devices from DUT')

        device_found = False
        for device in devices:
            if self.tester:
                if self.address == device['Address']:
                    logging.info('Found tester with RSSI %d',
                                 device.get('RSSI'))
                    # Check name as well; if the name and alias fields don't
                    # match, that means it hasn't been requested yet, so
                    # wait until next time.
                    if device.get('Name') != device['Alias']:
                        logging.info('Device name not yet received')
                        continue
                    if self.name != device['Alias']:
                        raise error.TestFail(
                                'Tester did not have expected name ' +
                                '"%s" != "%s"' % (device['Alias'],
                                                  self.name))
                    # Found the device
                    device_found = True
                    # Write out the RSSI now we've found it.
                    self.write_perf_keyval({'rssi': int(device.get('RSSI', 0))})
                    self.output_perf_value('rssi', int(device.get('RSSI', 0)),
                                           'dBm')

            if self.interactive:
                item_name = device['Address'].replace(':', '')
                html = '%s %s' % (cgi.escape(device['Address']),
                                  cgi.escape(device['Alias']))

                if device['Address'] in self.devices_discovered:
                    self.interactive.replace_list_item(item_name, html)
                else:
                    self.interactive.append_list_item('devices', item_name,
                                                      html)
                    self.devices_discovered.append(device['Address'])

                result = self.interactive.check_for_button()
                if result == 0:
                    device_found = True
                elif result != -1:
                    raise error.TestFail('User indicated test failed')

        return device_found


    def run_once(self):
        # Reset the adapter to the powered on state.
        if not self.device.reset_on():
            raise error.TestFail('DUT could not be reset to initial state')

        if self.tester:
            # Setup the tester as a generic LE peripheral.
            if not self.tester.setup('peripheral'):
                raise error.TestFail('Tester could not be initialized')
            # Enable general discoverable advertising on the tester
            self.tester.set_discoverable(True)
            self.tester.set_advertising(True)
            # Read the tester information so we know what we're looking for.
            ( address, bluetooth_version, manufacturer_id,
              supported_settings, current_settings, class_of_device,
              name, short_name ) = self.tester.read_info()
            self.address = address
            self.name = name

        if self.interactive:
            self.interactive.login()

            if self.tester:
                self.interactive.append_output(
                        '<p>The Tester is advertising as an LE peripheral. '
                        '<p>The DUT is in the observer/discovery state. '
                        '<p>Please verify that you can discover the tester ' +
                        ('<b>%s</b> with address <b>%s</b> from the device.' %
                         (cgi.escape(self.name),
                          cgi.escape(self.address))))
            else:
                self.interactive.append_output(
                        '<p>The DUT is in the observer/discovery state. '
                        '<p>Please verify that you can discover the device.')

            self.interactive.append_output('<h2>Devices Found</h2>')
            self.interactive.append_list('devices')
            self.devices_discovered = []

            if self.tester:
                self.interactive.append_buttons('Tester Found',
                                                'Tester Not Found')
            else:
                self.interactive.append_buttons('Device Found',
                                                'Device Not Found')

        # Discover devices from the DUT.
        for failed_attempts in range(0, 5):
            if not self.device.start_discovery():
                raise error.TestFail('Could not start discovery on DUT')
            try:
                utils.poll_for_condition(
                        condition=self.find_device,
                        desc='Device discovered from DUT',
                        timeout=self.DISCOVERABLE_TIMEOUT)
                # We only reach this if we find a device. Break out of the
                # failed_attempts loop to bypass the "reached the end"
                # condition.
                break
            except utils.TimeoutError:
                # Capture the timeout error and try once more through the
                # loop.
                pass
            finally:
                if not self.device.stop_discovery():
                    logging.warning('Failed to stop discovery on DUT')
        else:
            # We only reach this if we tried five times to find the device and
            # failed.
            raise error.TestFail('DUT could not discover device')
        # Record how many attempts this took, hopefully we'll one day figure
        # out a way to reduce this to zero and then the loop above can go
        # away.
        self.write_perf_keyval({'failed_attempts': failed_attempts })
        self.output_perf_value('failed_attempts', failed_attempts, 'attempts')


    def cleanup(self):
        """Set the tester back to not advertising."""
        if self.tester:
            self.tester.set_advertising(False)
            self.tester.set_discoverable(False)

        super(bluetooth_Sanity_LEDiscovery, self).cleanup()
