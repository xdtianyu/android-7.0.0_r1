# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_test

class bluetooth_SDP_ServiceBrowse(bluetooth_test.BluetoothTest):
    """
    Verify that the IUT behave correct during Service Browse procedure.
    """
    version = 1

    MAX_REC_CNT             = 100
    MAX_ATTR_BYTE_CNT       = 300
    PUBLIC_BROWSE_ROOT      = 0x1002
    SERVICE_CLASS_ID_LIST   = 0x0001
    BROWSE_GROUP_DESCRIPTOR = 0x1001
    GROUP_ID                = 0x0200


    def get_attribute_ssr_sar(self, class_id, attr_id, size):
        """Get service attributes using Service Search Request and Service
        Attribute Request.

        @param class_id: Class ID of service to check.
        @param attr_id: ID of attribute to check.
        @param size: Preferred size of UUID.

        @return attribute value if attribute exists, None otherwise

        """
        handles = self.tester.service_search_request(
                      [class_id], self.MAX_REC_CNT, size)
        if not (isinstance(handles, list) and len(handles) > 0):
            return None

        res = []
        for record_handle in handles:
            value = self.tester.service_attribute_request(
                        record_handle, self.MAX_ATTR_BYTE_CNT, [attr_id])
            if not (isinstance(value, list) and len(value) == 2 and
                value[0] == attr_id):
                return None
            res.append(value[1])

        return res


    def get_attribute_ssar(self, class_id, attr_id, size):
        """Get service attributes using Service Search Attribute Request.

        @param class_id: Class ID of service to check.
        @param attr_id: ID of attribute to check.
        @param size: Preferred size of UUID.

        @return attribute value if attribute exists, None otherwise

        """
        response = self.tester.service_search_attribute_request(
                       [class_id], self.MAX_ATTR_BYTE_CNT, [attr_id], size)

        if not isinstance(response, list):
            return None

        res = []
        for elem in response:
            if not (isinstance(elem, list) and len(elem) == 2 and
                    elem[0] == attr_id):
                return None
            res.append(elem[1])

        return res


    def test_attribute(self, class_id, attr_id, get_attribute):
        """Test service attributes using 16-bit, 32-bit and 128-bit
        size of UUID.

        @param class_id: Class ID of service to check.
        @param attr_id: ID of attribute to check.
        @param get_attribute: Method to use to get an attribute value.

        @return attribute value if attribute exists and values from three tests
        are equal, None otherwise

        """
        result_16 = get_attribute(class_id, attr_id, 16)

        for size in 32, 128:
            result_cur = get_attribute(class_id, attr_id, size)
            if result_16 != result_cur:
                return None

        return result_16


    def service_browse(self, get_attribute):
        """Execute a Service Browse procedure.

        @param get_attribute: Method to use to get an attribute value.

        @return sorted list of unique services on the DUT, or False if browse
        did not finish correctly

        """
        # Find services on top of hierarchy.
        root_services = self.test_attribute(self.PUBLIC_BROWSE_ROOT,
                                            self.SERVICE_CLASS_ID_LIST,
                                            get_attribute)
        if not root_services:
            return False

        # Find additional browse groups.
        group_ids = self.test_attribute(self.BROWSE_GROUP_DESCRIPTOR,
                                        self.GROUP_ID,
                                        get_attribute)
        if not group_ids:
            return False

        # Find services from all browse groups.
        all_services = []
        for group_id in group_ids:
            services = self.test_attribute(group_id,
                                           self.SERVICE_CLASS_ID_LIST,
                                           get_attribute)
            if not services:
                return False
            all_services.extend(services)

        # Ensure that root services are among all services.
        for service in root_services:
            if service not in all_services:
                return False

        # Sort all services and remove duplicates.
        all_services.sort()
        last = 0
        for service in all_services[1:]:
            if all_services[last] != service:
                last += 1
                all_services[last] = service

        return all_services[:last + 1]


    def correct_request(self):
        """Run basic tests for Service Browse procedure.

        @return True if all tests finishes correctly, False otherwise

        """
        # Connect to the DUT via L2CAP using SDP socket.
        self.tester.connect(self.adapter['Address'])

        browse_ssar = self.service_browse(self.get_attribute_ssar)
        if not browse_ssar:
            return False

        browse_ssr_sar = self.service_browse(self.get_attribute_ssr_sar)

        # Ensure that two different browse methods return the same results.
        return browse_ssar == browse_ssr_sar


    def run_once(self):
        # Reset the adapter to the powered on, discoverable state.
        if not (self.device.reset_on() and
                self.device.set_discoverable(True)):
            raise error.TestFail('DUT could not be reset to initial state')

        self.adapter = self.device.get_adapter_properties()

        # Setup the tester as a generic computer.
        if not self.tester.setup('computer'):
            raise error.TestFail('Tester could not be initialized')

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
