# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

"""
This module contains functions that are commonly used by tests while
interacting with a ChromeNetworkingTestContext.

"""

LONG_TIMEOUT = 120
SHORT_TIMEOUT = 10

def get_ui_property(network, property_name, expansion_level=1):
    """
    Returns the value of the property by applying a '.'-delimited path
    expansion, or None if the property is not found.

    @param network: A JSON dictionary containing network data, as returned by
            chrome.networkingPrivate.
    @param property_name: The property to obtain.
    @param expansion_level: Denotes the number of levels to descend through
            property based on the path expansion. For example, for property
            "A.B.C":

                level: 0
                return: props["A.B.C"]

                level: 1
                return: props["A"]["B.C"]

                level: >2
                return: props["A"]["B"]["C"]

    @return The value of the requested property, or None if not found.

    """
    path = property_name.split('.', expansion_level)
    result = network
    for key in path:
        value = result.get(key, None)
        if value is None:
            return None
        result = value
    return result


def check_ui_property(chrome_networking_test_context,
                      network_guid,
                      property_name,
                      expected_value,
                      expansion_level=1,
                      timeout=LONG_TIMEOUT):
    """
    Polls until the given network property has the expected value.

    @param chrome_networking_test_context: Instance of
            chrome_networking_test_context.ChromeNetworkingTestContext.
    @param network_guid: GUID of the network.
    @param property_name: Property to check.
    @param expected_value: Value the property is expected to obtain.
    @param expansion_level: Path expansion depth.
    @param timeout: Timeout interval in which the property should reach the
            expected value.

    @raises error.TestFail, if the check doesn't pass within |timeout|.

    """
    def _compare_props():
        network = call_test_function_check_success(
                chrome_networking_test_context,
                'getNetworkInfo',
                ('"' + network_guid + '"',))
        value = get_ui_property(network, property_name, expansion_level)
        return value == expected_value
    utils.poll_for_condition(
            _compare_props,
            error.TestFail('Property "' + property_name + '" on network "' +
                           network_guid + '" never obtained value "' +
                           expected_value + '"'),
            timeout)


def simple_network_sanity_check(
        network, expected_name, expected_type, check_name_prefix=True):
    """
    Simple check to ensure that the network type and name match the expected
    values.

    @param network: A JSON dictionary containing network data, as returned by
            chrome.networkingPrivate.
    @param expected_name: The expected value of the 'Name' property.
    @param expected_type: The expected value of the 'Type' property.
    @param check_name_prefix: If True, the check will not fail, as long as the
            value of the 'Name' property starts with |expected_name|. If False,
            this function will check for an exact match.

    @raises error.TestFail if any of the checks doesn't pass.

    """
    if network['Type'] != expected_type:
        raise error.TestFail(
                'Expected network of type "' + expected_type + '", found ' +
                network["Type"])

    network_name = network['Name']
    name_error_message = (
            'Network name "%s" did not match the expected: %s (Check prefix '
            'only=%s).' % (network_name, expected_name, check_name_prefix))
    if ((check_name_prefix and not network_name.startswith(expected_name)) or
        (not check_name_prefix and network_name != expected_name)):
        raise error.TestFail(name_error_message)


def call_test_function_check_success(
        chrome_networking_test_context, function, args, timeout=SHORT_TIMEOUT):
    """
    Invokes the given function and makes sure that it succeeds. If the function
    succeeds then the result is returned, otherwise an error.TestFail is
    raised.

    @param chrome_networking_test_context: Instance of
            chrome_networking_test_context.ChromeNetworkingTestContext.
    @param function: String, containing the network test function to execute.
    @param args: Tuple of arguments to pass to |function|.
    @param timeout: Timeout in which the function should terminate.

    @raises: error.TestFail, if function returns with failure.

    @return: The result value of the function. If |function| doesn't have a
            result value, the Python equivalent of the JS "null" will be
            returned.

    """
    call_status = chrome_networking_test_context.call_test_function(
            timeout, function, *args)
    if call_status['status'] != chrome_networking_test_context.STATUS_SUCCESS:
        raise error.TestFail('Function "' + function + '" did not return with '
                             'status SUCCESS: ' + str(call_status))
    return call_status['result']
