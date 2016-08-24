# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side audio nodes s test using the Chameleon board."""

import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_port_finder

from autotest_lib.client.cros.chameleon import edid as edid_lib
from autotest_lib.server.cros.audio import audio_test



class audio_AudioNodeSwitch(audio_test.AudioTest):
    """Server side audio test.

    This test talks to a Chameleon board and a Cros device to verify
    audio nodes switch correctly.

    """
    version = 1
    _APPLY_EDID_DELAY = 5
    _PLUG_DELAY = 5
    _VOLUMES = {'INTERNAL_SPEAKER': 100,
                'HEADPHONE': 80,
                'HDMI': 60,
                'USB': 40,}

    def check_default_nodes(self):
        """Checks default audio nodes for devices with onboard audio support."""
        if audio_test_utils.has_internal_microphone(self.host):
            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (None, ['INTERNAL_MIC']))
        if audio_test_utils.has_internal_speaker(self.host):
            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (['INTERNAL_SPEAKER'], None))


    def set_active_volume_to_node_volume(self, node):
        """Sets Chrome volume to the specified volume of node.

        @param node: One of node type in self._VOLUMES.

        """
        self.audio_facade.set_chrome_active_volume(self._VOLUMES[node])


    def check_active_node_volume(self, node):
        """Checks the active node type and checks if its volume is as expected.

        @param node: The expected node.

        @raises: TestFail if node volume is not as expected.

        """
        # Checks the node type is the active node type.
        audio_test_utils.check_audio_nodes(self.audio_facade, ([node], None))
        # Checks if active volume is the node volume.
        volume, mute = self.audio_facade.get_chrome_active_volume_mute()
        expected_volume = self._VOLUMES[node]
        if volume != expected_volume:
            raise error.TestFail(
                    'Node %s volume %d != %d' % (node, volume, expected_volume))


    def switch_nodes_and_check_volume(self, nodes):
        """Switches between nodes and check the node volumes.

        @param nodes: A list of node types to check.

        """
        if len(nodes) == 1:
            self.check_active_node_volume(nodes[0])
        for node in nodes:
            # Switch nodes and check their volume.
            self.audio_facade.set_chrome_active_node_type(node, None)
            self.check_active_node_volume(node)


    def run_once(self, host, jack_node=False, hdmi_node=False, usb_node=False):
        self.host = host
        chameleon_board = host.chameleon
        audio_board = chameleon_board.get_audio_board()
        factory = self.create_remote_facade_factory(host)

        chameleon_board.reset()
        self.audio_facade = factory.create_audio_facade()
        self.display_facade = factory.create_display_facade()

        self.check_default_nodes()
        nodes = []
        if audio_test_utils.has_internal_speaker(self.host):
            self.set_active_volume_to_node_volume('INTERNAL_SPEAKER')
            nodes.append('INTERNAL_SPEAKER')
            self.switch_nodes_and_check_volume(nodes)


        if hdmi_node:
            edid_path = os.path.join(self.bindir,
                                     'test_data/edids/HDMI_DELL_U2410.txt')
            finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, self.display_facade)
            hdmi_port = finder.find_port('HDMI')
            hdmi_port.apply_edid(edid_lib.Edid.from_file(edid_path))
            time.sleep(self._APPLY_EDID_DELAY)
            hdmi_port.set_plug(True)
            time.sleep(self._PLUG_DELAY)

            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (['HDMI'], None))

            self.set_active_volume_to_node_volume('HDMI')
            nodes.append('HDMI')
            self.switch_nodes_and_check_volume(nodes)

        if jack_node:
            jack_plugger = audio_board.get_jack_plugger()
            jack_plugger.plug()
            time.sleep(self._PLUG_DELAY)
            audio_test_utils.dump_cros_audio_logs(host, self.audio_facade,
                                                  self.resultsdir)
            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (['HEADPHONE'], ['MIC']))

            self.set_active_volume_to_node_volume('HEADPHONE')
            nodes.append('HEADPHONE')
            self.switch_nodes_and_check_volume(nodes)

        if usb_node:
            widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

            source = widget_factory.create_widget(
                chameleon_audio_ids.CrosIds.USBOUT)
            recorder = widget_factory.create_widget(
                chameleon_audio_ids.ChameleonIds.USBIN)
            binder = widget_factory.create_binder(source, recorder)

            with chameleon_audio_helper.bind_widgets(binder):
                time.sleep(self._PLUG_DELAY)
                audio_test_utils.check_audio_nodes(self.audio_facade,
                                                   (['USB'], ['USB']))
                self.set_active_volume_to_node_volume('USB')
                nodes.append('USB')
                self.switch_nodes_and_check_volume(nodes)
            time.sleep(self._PLUG_DELAY)
            nodes.remove('USB')
            self.switch_nodes_and_check_volume(nodes)

        if jack_node:
            if usb_node:
                audio_test_utils.check_audio_nodes(self.audio_facade,
                                                   (['HEADPHONE'], ['MIC']))
            jack_plugger.unplug()
            time.sleep(self._PLUG_DELAY)
            nodes.remove('HEADPHONE')
            self.switch_nodes_and_check_volume(nodes)

        if hdmi_node:
            if usb_node or jack_node :
                audio_test_utils.check_audio_nodes(self.audio_facade,
                                                   (['HDMI'], None))
            hdmi_port.set_plug(False)
            time.sleep(self._PLUG_DELAY)
            nodes.remove('HDMI')
            self.switch_nodes_and_check_volume(nodes)

        self.check_default_nodes()

