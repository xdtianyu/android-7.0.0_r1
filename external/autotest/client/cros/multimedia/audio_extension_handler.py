# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handler for audio extension functionality."""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.multimedia import facade_resource

class AudioExtensionHandlerError(Exception):
    pass


class AudioExtensionHandler(object):
    def __init__(self, extension):
        """Initializes an AudioExtensionHandler.

        @param extension: Extension got from telemetry chrome wrapper.

        """
        self._extension = extension
        self._check_api_available()


    def _check_api_available(self):
        """Checks chrome.audio is available.

        @raises: AudioExtensionHandlerError if extension is not available.

        """
        success = utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "chrome.audio") != None),
                expected_value=True)
        if not success:
            raise AudioExtensionHandlerError('chrome.audio is not available.')


    @facade_resource.retry_chrome_call
    def get_audio_info(self):
        """Gets the audio info from Chrome audio API.

        @returns: An array of [outputInfo, inputInfo].
                  outputInfo is an array of output node info dicts. Each dict
                  contains these key-value pairs:
                     string  id
                         The unique identifier of the audio output device.

                     string  name
                         The user-friendly name (e.g. "Bose Amplifier").

                     boolean isActive
                         True if this is the current active device.

                     boolean isMuted
                         True if this is muted.

                     double  volume
                         The output volume ranging from 0.0 to 100.0.

                  inputInfo is an arrya of input node info dicts. Each dict
                  contains these key-value pairs:
                     string  id
                         The unique identifier of the audio input device.

                     string  name
                         The user-friendly name (e.g. "USB Microphone").

                     boolean isActive
                         True if this is the current active device.

                     boolean isMuted
                         True if this is muted.

                     double  gain
                         The input gain ranging from 0.0 to 100.0.

        """
        self._extension.ExecuteJavaScript('window.__audio_info = null;')
        self._extension.ExecuteJavaScript(
                "chrome.audio.getInfo(function(outputInfo, inputInfo) {"
                "window.__audio_info = [outputInfo, inputInfo];})")
        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__audio_info") != None),
                expected_value=True)
        return self._extension.EvaluateJavaScript("window.__audio_info")


    def _get_active_id(self):
        """Gets active output and input node id.

        Assume there is only one active output node and one active input node.

        @returns: (output_id, input_id) where output_id and input_id are
                  strings for active node id.

        """
        output_nodes, input_nodes = self.get_audio_info()

        return (self._get_active_id_from_nodes(output_nodes),
                self._get_active_id_from_nodes(input_nodes))


    def _get_active_id_from_nodes(self, nodes):
        """Gets active node id from nodes.

        Assume there is only one active node.

        @param nodes: A list of input/output nodes got from get_audio_info().

        @returns: node['id'] where node['isActive'] is True.

        @raises: AudioExtensionHandlerError if active id is not unique.

        """
        active_ids = [x['id'] for x in nodes if x['isActive']]
        if len(active_ids) != 1:
            logging.error(
                    'Node info contains multiple active nodes: %s', nodes)
            raise AudioExtensionHandlerError(
                    'Active id should be unique')

        return active_ids[0]



    @facade_resource.retry_chrome_call
    def set_active_volume(self, volume):
        """Sets the active audio output volume using chrome.audio API.

        This method also unmutes the node.

        @param volume: Volume to set (0~100).

        """
        output_id, _ = self._get_active_id()
        logging.debug('output_id: %s', output_id)

        self._extension.ExecuteJavaScript('window.__set_volume_done = null;')

        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setProperties(
                    '%s',
                    {isMuted: false, volume: %s},
                    function() {window.__set_volume_done = true;});
                """
                % (output_id, volume))

        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_volume_done") != None),
                expected_value=True)


    @facade_resource.retry_chrome_call
    def set_mute(self, mute):
        """Mutes the active audio output using chrome.audio API.

        @param mute: True to mute. False otherwise.

        """
        output_id, _ = self._get_active_id()
        logging.debug('output_id: %s', output_id)

        is_muted_string = 'true' if mute else 'false'

        self._extension.ExecuteJavaScript('window.__set_mute_done = null;')

        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setProperties(
                    '%s',
                    {isMuted: %s},
                    function() {window.__set_mute_done = true;});
                """
                % (output_id, is_muted_string))

        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_mute_done") != None),
                expected_value=True)


    @facade_resource.retry_chrome_call
    def get_active_volume_mute(self):
        """Gets the volume state of active audio output using chrome.audio API.

        @param returns: A tuple (volume, mute), where volume is 0~100, and mute
                        is True if node is muted, False otherwise.

        """
        output_nodes, _ = self.get_audio_info()
        active_id = self._get_active_id_from_nodes(output_nodes)
        for node in output_nodes:
            if node['id'] == active_id:
                return (node['volume'], node['isMuted'])


    @facade_resource.retry_chrome_call
    def set_active_node_id(self, node_id):
        """Sets the active node by node id.

        The current active node will be disabled first if the new active node
        is different from the current one.

        @param node_id: The node id obtained from cras_utils.get_cras_nodes.
                        Chrome.audio also uses this id to specify input/output
                        nodes.

        @raises AudioExtensionHandlerError if there is no such id.

        """
        if node_id in self._get_active_id():
            logging.debug('Node %s is already active.', node_id)
            return

        logging.debug('Setting active id to %s', node_id)

        self._extension.ExecuteJavaScript('window.__set_active_done = null;')

        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setActiveDevices(
                    ['%s'],
                    function() {window.__set_active_done = true;});
                """
                % (node_id))

        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_active_done") != None),
                expected_value=True)
