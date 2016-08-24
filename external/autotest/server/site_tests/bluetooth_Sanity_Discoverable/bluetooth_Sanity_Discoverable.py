# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cgi
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.server.cros.bluetooth import bluetooth_test


class bluetooth_Sanity_Discoverable(bluetooth_test.BluetoothTest):
    """
    Verify that the client is discoverable from the tester.
    """
    version = 1


    def discover_device(self):
        """Discover the Bluetooth Device using the Tester.

        @return True if found with expected name, False if not found,
          Exception if found device did not have expected name.

        """
        devices = self.tester.discover_devices()
        if devices == False:
            raise error.TestFail('Tester could not discover devices')

        # Iterate the devices we received in the discovery phase and
        # look for the DUT.
        for address, address_type, rssi, flags, eirdata in devices:
            if address == self.adapter['Address']:
                logging.info('Found device with RSSI %d', rssi)
                found_device = True

                # Make sure we picked up a name for the device along
                # the way.
                eir = bluetooth_socket.parse_eir(eirdata)
                try:
                    eir_name = eir[bluetooth_socket.EIR_NAME_COMPLETE]
                    if eir_name != self.adapter['Alias']:
                        raise error.TestFail(
                                'Device did not have expected name ' +
                                '"%s" != "%s"' %
                                (eir_name, self.adapter['Alias']))

                    # Write out the RSSI now we've found it.
                    self.write_perf_keyval({'rssi': int(rssi)})
                    self.output_perf_value('rssi', int(rssi), 'dBm')
                    return True
                except KeyError:
                    logging.warning('Device did not have a name')
        else:
            logging.warning('Failed to find device')

        return False

    def run_once(self):
        # Reset the adapter to the powered on, discoverable state.
        if not (self.device.reset_on() and
                self.device.set_discoverable(True)):
            raise error.TestFail('DUT could not be reset to initial state')

        self.adapter = self.device.get_adapter_properties()

        if self.interactive:
            self.interactive.login()
            self.interactive.append_output(
                    '<p>The DUT is in the discoverable state. '
                    '<p>Please verify that you can discover the device ' +
                    ('<b>%s</b> with address <b>%s</b> from the tester.' %
                     (cgi.escape(self.adapter['Alias']),
                      cgi.escape(self.adapter['Address']))))

        if self.tester:
            # Setup the tester as a generic computer.
            if not self.tester.setup('computer'):
                raise error.TestFail('Tester could not be initialized')

            # Since radio is involved, this test is not 100% reliable; instead
            # we repeat a few times until it succeeds.
            for failed_attempts in range(0, 5):
                if self.discover_device():
                    break
            else:
                raise error.TestFail('Expected device was not found')

            # Record how many attempts this took, hopefully we'll one day figure
            # out a way to reduce this to zero and then the loop above can go
            # away.
            self.write_perf_keyval({'failed_attempts': failed_attempts })
            self.output_perf_value('failed_attempts', failed_attempts,
                                   'attempts')

        if self.interactive:
            self.interactive.append_buttons('Device Found', 'Device Not Found')
            result = self.interactive.wait_for_button(timeout=600)
            if result != 0:
                raise error.TestFail('User indicated test failed')
