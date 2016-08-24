# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error


_AVAILABLE_AUDIO_CLIENTS = set(('loop', 'interactive'))


def _get_client(fb_client_name, available_clients, test_name, machine,
                args_str):
    """Instantiates a feedback client.

    @param fb_client_name: Name of the desired client.
    @param available_clients: Set of available client names to choose from.
    @param test_name: The name of the test.
    @param machine: A dictionary describing the test host and DUT.
    @param args_str: String containing comma-separate, implementation-specific
                     arguments.

    @return An instance of client.common_lib.feedback.client.Client.

    @raise error.TestError: Requested client is invalid/unavailable/unknown.
    """
    if not fb_client_name:
        raise error.TestError('Feedback client name is empty')
    if fb_client_name not in available_clients:
        raise error.TestError(
                'Feedback client (%s) is unknown or unavailble for this test' %
                fb_client_name)

    dut_name = '%s-%s' % (machine['hostname'],
                          machine['host_attributes']['serials'])
    args = args_str.split(',') if args_str else []

    if fb_client_name == 'loop':
        from autotest_lib.server.brillo.feedback import closed_loop_audio_client
        return closed_loop_audio_client.Client(*args)
    elif fb_client_name == 'interactive':
        from autotest_lib.client.common_lib.feedback import tester_feedback_client
        return tester_feedback_client.Client(test_name, dut_name, *args)
    else:
        raise error.TestError(
                'Feedback client (%s) unknown despite being listed as '
                'available for this test' % fb_client_name)


def get_audio_client(fb_client_name, test_name, machine, args_str):
    """Instantiates an audio feedback client."""
    return _get_client(fb_client_name, _AVAILABLE_AUDIO_CLIENTS, test_name,
                       machine, args_str)
