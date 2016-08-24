# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side test to check nodes created for internal card."""

import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.server.cros.audio import audio_test


class audio_InternalCardNodes(audio_test.AudioTest):
    """Server side test to check audio nodes for internal card.

    This test talks to a Chameleon board and a Cros device to verify
    audio nodes created for internal cards are correct.

    """
    version = 1
    DELAY_AFTER_PLUGGING = 2
    DELAY_AFTER_UNPLUGGING = 2

    def run_once(self, host):
        chameleon_board = host.chameleon
        factory = self.create_remote_facade_factory(host)
        audio_facade = factory.create_audio_facade()

        chameleon_board.reset()

        jack_plugger = chameleon_board.get_audio_board().get_jack_plugger()

        expected_plugged_nodes_without_audio_jack = (
                [],
                ['POST_DSP_LOOPBACK',
                 'POST_MIX_LOOPBACK'])

        expected_plugged_nodes_with_audio_jack = (
                ['HEADPHONE'],
                ['MIC', 'POST_DSP_LOOPBACK',
                 'POST_MIX_LOOPBACK'])

        if audio_test_utils.has_internal_speaker(host):
            expected_plugged_nodes_without_audio_jack[0].append(
                    'INTERNAL_SPEAKER')
            expected_plugged_nodes_with_audio_jack[0].append(
                    'INTERNAL_SPEAKER')

        if audio_test_utils.has_internal_microphone(host):
            expected_plugged_nodes_without_audio_jack[1].append(
                    'INTERNAL_MIC')
            expected_plugged_nodes_with_audio_jack[1].append(
                    'INTERNAL_MIC')

        # Modify expected nodes for special boards.
        board_name = host.get_board().split(':')[1]

        if board_name == 'link':
            expected_plugged_nodes_without_audio_jack[1].append('KEYBOARD_MIC')
            expected_plugged_nodes_with_audio_jack[1].append('KEYBOARD_MIC')

        if board_name == 'samus':
            expected_plugged_nodes_without_audio_jack[1].append('AOKR')
            expected_plugged_nodes_with_audio_jack[1].append('AOKR')

        audio_test_utils.check_plugged_nodes(
                audio_facade, expected_plugged_nodes_without_audio_jack)

        try:
            jack_plugger.plug()
            time.sleep(self.DELAY_AFTER_PLUGGING)

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir)

            audio_test_utils.check_plugged_nodes(
                    audio_facade, expected_plugged_nodes_with_audio_jack)

        finally:
            jack_plugger.unplug()
            time.sleep(self.DELAY_AFTER_UNPLUGGING)

        audio_test_utils.check_plugged_nodes(
                audio_facade, expected_plugged_nodes_without_audio_jack)

