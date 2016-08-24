# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_test
import uuid

class bluetooth_SDP_ServiceSearchRequestBasic(bluetooth_test.BluetoothTest):
    """
    Verify the correct behaviour of the device when searching for services.
    """
    version = 1

    SDP_SERVER_CLASS_ID                = 0x1000
    NO_EXISTING_SERVICE_CLASS_ID       = 0x0001
    FAKE_SERVICES_CNT                  = 300
    FAKE_SERVICES_PATH                 = '/autotest/fake_service_'
    FAKE_SERVICES_CLASS_ID             = 0xABCD
    BLUETOOTH_BASE_UUID                = 0x0000000000001000800000805F9B34FB
    INVALID_PDU_SIZE                   = 9875
    ERROR_CODE_INVALID_REQUEST_SYNTAX  = 0x0003
    ERROR_CODE_INVALID_PDU_SIZE        = 0x0004


    def correct_request(self):
        """Search the existing service on the DUT using the Tester.

        @return True if found, False if not found

        """
        # connect to the DUT via L2CAP using SDP socket
        self.tester.connect(self.adapter['Address'])

        for size in 16, 32, 128:
            # test case TP/SERVER/SS/BV-01-C:
            # at least the SDP server service exists
            resp = self.tester.service_search_request(
                   [self.SDP_SERVER_CLASS_ID], 3, size)
            if resp != [0]:
                return False
            # test case TP/SERVER/SS/BV-04-C:
            # Service with Class ID = 0x0001 should never exist, as this UUID is
            # reserved as Bluetooth Core Specification UUID
            resp = self.tester.service_search_request(
                   [self.NO_EXISTING_SERVICE_CLASS_ID], 3, size)
            if resp != []:
                return False
            # test case TP/SERVER/SS/BV-03-C:
            # request the fake services' Class ID to force SDP to use
            # continuation state
            resp = self.tester.service_search_request(
                   [self.FAKE_SERVICES_CLASS_ID],
                   self.FAKE_SERVICES_CNT * 2,
                   size)
            if len(resp) != self.FAKE_SERVICES_CNT:
                return False
            # test case TP/SERVER/SS/BI-01-C:
            # send a Service Search Request with intentionally invalid PDU size
            resp = self.tester.service_search_request(
                   [self.SDP_SERVER_CLASS_ID], 3, size,
                   forced_pdu_size=self.INVALID_PDU_SIZE)
            if resp != self.ERROR_CODE_INVALID_PDU_SIZE:
                return False
            # test case TP/SERVER/SS/BI-02-C:
            # send a Service Search Request with invalid syntax
            resp = self.tester.service_search_request(
                   [self.SDP_SERVER_CLASS_ID], 3, size, invalid_request=True)
            if resp != self.ERROR_CODE_INVALID_REQUEST_SYNTAX:
                return False

        return True


    def run_once(self):
        # Reset the adapter to the powered on, discoverable state.
        if not (self.device.reset_on() and
                self.device.set_discoverable(True)):
            raise error.TestFail('DUT could not be reset to initial state')

        self.adapter = self.device.get_adapter_properties()

        # Setup the tester as a generic computer.
        if not self.tester.setup('computer'):
            raise error.TestFail('Tester could not be initialized')

        # Create many fake services with the same Class ID
        for num in range(0, self.FAKE_SERVICES_CNT):
            path_str = self.FAKE_SERVICES_PATH + str(num)
            uuid128 = ((self.FAKE_SERVICES_CLASS_ID << 96) +
                      self.BLUETOOTH_BASE_UUID)
            uuid_str = str(uuid.UUID(int=uuid128))
            self.device.register_profile(path_str, uuid_str, {})

        # Since radio is involved, this test is not 100% reliable; instead we
        # repeat a few times until it succeeds.
        for failed_attempts in range(0, 5):
            if self.correct_request():
                break
        else:
            raise error.TestFail('Expected device was not found')

        # Record how many attempts this took, hopefully we'll one day figure out
        # a way to reduce this to zero and then the loop above can go away.
        self.write_perf_keyval({'failed_attempts': failed_attempts })
