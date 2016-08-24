# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides cras audio utilities."""

import logging
import re

from autotest_lib.client.cros.audio import cmd_utils

_CRAS_TEST_CLIENT = '/usr/bin/cras_test_client'


class CrasUtilsError(Exception):
    pass


def playback(*args, **kargs):
    """A helper function to execute the playback_cmd.

    @param args: args passed to playback_cmd.
    @param kargs: kargs passed to playback_cmd.

    """
    cmd_utils.execute(playback_cmd(*args, **kargs))


def capture(*args, **kargs):
    """A helper function to execute the capture_cmd.

    @param args: args passed to capture_cmd.
    @param kargs: kargs passed to capture_cmd.

    """
    cmd_utils.execute(capture_cmd(*args, **kargs))


def playback_cmd(playback_file, block_size=None, duration=None,
                 channels=2, rate=48000):
    """Gets a command to playback a file with given settings.

    @param playback_file: the name of the file to play. '-' indicates to
                          playback raw audio from the stdin.
    @param block_size: the number of frames per callback(dictates latency).
    @param duration: seconds to playback
    @param channels: number of channels.
    @param rate: the sampling rate

    @returns: The command args put in a list of strings.

    """
    args = [_CRAS_TEST_CLIENT]
    args += ['--playback_file', playback_file]
    if block_size is not None:
        args += ['--block_size', str(block_size)]
    if duration is not None:
        args += ['--duration', str(duration)]
    args += ['--num_channels', str(channels)]
    args += ['--rate', str(rate)]
    return args


def capture_cmd(
        capture_file, block_size=None, duration=10, channels=1, rate=48000):
    """Gets a command to capture the audio into the file with given settings.

    @param capture_file: the name of file the audio to be stored in.
    @param block_size: the number of frames per callback(dictates latency).
    @param duration: seconds to record. If it is None, duration is not set,
                     and command will keep capturing audio until it is
                     terminated.
    @param channels: number of channels.
    @param rate: the sampling rate.

    @returns: The command args put in a list of strings.

    """
    args = [_CRAS_TEST_CLIENT]
    args += ['--capture_file', capture_file]
    if block_size is not None:
        args += ['--block_size', str(block_size)]
    if duration is not None:
        args += ['--duration', str(duration)]
    args += ['--num_channels', str(channels)]
    args += ['--rate', str(rate)]
    return args


def loopback(*args, **kargs):
    """A helper function to execute loopback_cmd.

    @param args: args passed to loopback_cmd.
    @param kargs: kargs passed to loopback_cmd.

    """

    cmd_utils.execute(loopback_cmd(*args, **kargs))


def loopback_cmd(output_file, duration=10, channels=2, rate=48000):
    """Gets a command to record the loopback.

    @param output_file: The name of the file the loopback to be stored in.
    @param channels: The number of channels of the recorded audio.
    @param duration: seconds to record.
    @param rate: the sampling rate.

    @returns: The command args put in a list of strings.

    """
    args = [_CRAS_TEST_CLIENT]
    args += ['--loopback_file', output_file]
    args += ['--duration_seconds', str(duration)]
    args += ['--num_channels', str(channels)]
    args += ['--rate', str(rate)]
    return args


def get_cras_nodes_cmd():
    """Gets a command to query the nodes from Cras.

    @returns: The command to query nodes information from Cras using dbus-send.

    """
    return ('dbus-send --system --type=method_call --print-reply '
            '--dest=org.chromium.cras /org/chromium/cras '
            'org.chromium.cras.Control.GetNodes')


def set_system_volume(volume):
    """Set the system volume.

    @param volume: the system output vlume to be set(0 - 100).

    """
    get_cras_control_interface().SetOutputVolume(volume)


def set_node_volume(node_id, volume):
    """Set the volume of the given output node.

    @param node_id: the id of the output node to be set the volume.
    @param volume: the volume to be set(0-100).

    """
    get_cras_control_interface().SetOutputNodeVolume(node_id, volume)


def set_capture_gain(gain):
    """Set the system capture gain.

    @param gain the capture gain in db*100 (100 = 1dB)

    """
    get_cras_control_interface().SetInputGain(gain)


def get_cras_control_interface(private=False):
    """Gets Cras DBus control interface.

    @param private: Set to True to use a new instance for dbus.SystemBus
                    instead of the shared instance.

    @returns: A dBus.Interface object with Cras Control interface.

    @raises: ImportError if this is not called on Cros device.

    """
    try:
        import dbus
    except ImportError, e:
        logging.exception(
                'Can not import dbus: %s. This method should only be '
                'called on Cros device.', e)
        raise
    bus = dbus.SystemBus(private=private)
    cras_object = bus.get_object('org.chromium.cras', '/org/chromium/cras')
    return dbus.Interface(cras_object, 'org.chromium.cras.Control')


def get_cras_nodes():
    """Gets nodes information from Cras.

    @returns: A dict containing information of each node.

    """
    return get_cras_control_interface().GetNodes()


def get_selected_nodes():
    """Gets selected output nodes and input nodes.

    @returns: A tuple (output_nodes, input_nodes) where each
              field is a list of selected node IDs returned from Cras DBus API.
              Note that there may be multiple output/input nodes being selected
              at the same time.

    """
    output_nodes = []
    input_nodes = []
    nodes = get_cras_nodes()
    for node in nodes:
        if node['Active']:
            if node['IsInput']:
                input_nodes.append(node['Id'])
            else:
                output_nodes.append(node['Id'])
    return (output_nodes, input_nodes)


def set_selected_output_node_volume(volume):
    """Sets the selected output node volume.

    @param volume: the volume to be set (0-100).

    """
    selected_output_node_ids, _ = get_selected_nodes()
    for node_id in selected_output_node_ids:
        set_node_volume(node_id, volume)


def get_active_stream_count():
    """Gets the number of active streams.

    @returns: The number of active streams.

    """
    return int(get_cras_control_interface().GetNumberOfActiveStreams())


def set_system_mute(is_mute):
    """Sets the system mute switch.

    @param is_mute: Set True to mute the system playback.

    """
    get_cras_control_interface().SetOutputMute(is_mute)


def set_capture_mute(is_mute):
    """Sets the capture mute switch.

    @param is_mute: Set True to mute the capture.

    """
    get_cras_control_interface().SetInputMute(is_mute)


def node_type_is_plugged(node_type, nodes_info):
    """Determine if there is any node of node_type plugged.

    This method is used in has_loopback_dongle in cros_host, where
    the call is executed on autotest server. Use get_cras_nodes instead if
    the call can be executed on Cros device.

    Since Cras only reports the plugged node in GetNodes, we can
    parse the return value to see if there is any node with the given type.
    For example, if INTERNAL_MIC is of intereset, the pattern we are
    looking for is:

    dict entry(
       string "Type"
       variant             string "INTERNAL_MIC"
    )

    @param node_type: A str representing node type defined in CRAS_NODE_TYPES.
    @param nodes_info: A str containing output of command get_nodes_cmd.

    @returns: True if there is any node of node_type plugged. False otherwise.

    """
    match = re.search(r'string "Type"\s+variant\s+string "%s"' % node_type,
                      nodes_info)
    return True if match else False


# Cras node types reported from Cras DBus control API.
CRAS_OUTPUT_NODE_TYPES = ['HEADPHONE', 'INTERNAL_SPEAKER', 'HDMI', 'USB',
                          'BLUETOOTH', 'UNKNOWN']
CRAS_INPUT_NODE_TYPES = ['MIC', 'INTERNAL_MIC', 'USB', 'BLUETOOTH',
                         'POST_DSP_LOOPBACK', 'POST_MIX_LOOPBACK', 'UNKNOWN',
                         'KEYBOARD_MIC', 'AOKR']
CRAS_NODE_TYPES = CRAS_OUTPUT_NODE_TYPES + CRAS_INPUT_NODE_TYPES


def get_filtered_node_types(callback):
    """Returns the pair of filtered output node types and input node types.

    @param callback: A callback function which takes a node as input parameter
                     and filter the node based on its return value.

    @returns: A tuple (output_node_types, input_node_types) where each
              field is a list of node types defined in CRAS_NODE_TYPES,
              and their 'attribute_name' is True.

    """
    output_node_types = []
    input_node_types = []
    nodes = get_cras_nodes()
    for node in nodes:
        if callback(node):
            node_type = str(node['Type'])
            if node_type not in CRAS_NODE_TYPES:
                raise RuntimeError(
                        'node type %s is not valid' % node_type)
            if node['IsInput']:
                input_node_types.append(node_type)
            else:
                output_node_types.append(node_type)
    return (output_node_types, input_node_types)


def get_selected_node_types():
    """Returns the pair of active output node types and input node types.

    @returns: A tuple (output_node_types, input_node_types) where each
              field is a list of selected node types defined in CRAS_NODE_TYPES.

    """
    def is_selected(node):
        """Checks if a node is selected.

        A node is selected if its Active attribute is True.

        @returns: True is a node is selected, False otherwise.

        """
        return node['Active']

    return get_filtered_node_types(is_selected)


def get_plugged_node_types():
    """Returns the pair of plugged output node types and input node types.

    @returns: A tuple (output_node_types, input_node_types) where each
              field is a list of plugged node types defined in CRAS_NODE_TYPES.

    """
    def is_plugged(node):
        """Checks if a node is plugged and is not unknown node.

        Cras DBus API only reports plugged node, so every node reported by Cras
        DBus API is plugged. However, we filter out UNKNOWN node here because
        the existence of unknown node depends on the number of redundant
        playback/record audio device created on audio card. Also, the user of
        Cras will ignore unknown nodes.

        @returns: True if a node is plugged and is not an UNKNOWN node.

        """
        return node['Type'] != 'UNKNOWN'

    return get_filtered_node_types(is_plugged)


def set_selected_node_types(output_node_types, input_node_types):
    """Sets selected node types.

    @param output_node_types: A list of output node types. None to skip setting.
    @param input_node_types: A list of input node types. None to skip setting.

    """
    if len(output_node_types) == 1:
        set_single_selected_output_node(output_node_types[0])
    elif output_node_types:
        set_selected_output_nodes(output_node_types)
    if len(input_node_types) == 1:
        set_single_selected_input_node(input_node_types[0])
    elif input_node_types:
        set_selected_input_nodes(input_node_types)


def set_single_selected_output_node(node_type):
    """Sets one selected output node.

    Note that Chrome UI uses SetActiveOutputNode of Cras DBus API
    to select one output node.

    @param node_type: A node type.

    """
    nodes = get_cras_nodes()
    for node in nodes:
        if node['IsInput']:
            continue
        if node['Type'] == node_type:
            set_active_output_node(node['Id'])


def set_single_selected_input_node(node_type):
    """Sets one selected input node.

    Note that Chrome UI uses SetActiveInputNode of Cras DBus API
    to select one input node.

    @param node_type: A node type.

    """
    nodes = get_cras_nodes()
    for node in nodes:
        if not node['IsInput']:
            continue
        if node['Type'] == node_type:
            set_active_input_node(node['Id'])


def set_selected_output_nodes(types):
    """Sets selected output node types.

    Note that Chrome UI uses SetActiveOutputNode of Cras DBus API
    to select one output node. Here we use add/remove active output node
    to support multiple nodes.

    @param types: A list of output node types.

    """
    nodes = get_cras_nodes()
    for node in nodes:
        if node['IsInput']:
            continue
        if node['Type'] in types:
            add_active_output_node(node['Id'])
        elif node['Active']:
            remove_active_output_node(node['Id'])


def set_selected_input_nodes(types):
    """Sets selected input node types.

    Note that Chrome UI uses SetActiveInputNode of Cras DBus API
    to select one input node. Here we use add/remove active input node
    to support multiple nodes.

    @param types: A list of input node types.

    """
    nodes = get_cras_nodes()
    for node in nodes:
        if not node['IsInput']:
            continue
        if node['Type'] in types:
            add_active_input_node(node['Id'])
        elif node['Active']:
            remove_active_input_node(node['Id'])


def set_active_input_node(node_id):
    """Sets one active input node.

    @param node_id: node id.

    """
    get_cras_control_interface().SetActiveInputNode(node_id)


def set_active_output_node(node_id):
    """Sets one active output node.

    @param node_id: node id.

    """
    get_cras_control_interface().SetActiveOutputNode(node_id)


def add_active_output_node(node_id):
    """Adds an active output node.

    @param node_id: node id.

    """
    get_cras_control_interface().AddActiveOutputNode(node_id)


def add_active_input_node(node_id):
    """Adds an active input node.

    @param node_id: node id.

    """
    get_cras_control_interface().AddActiveInputNode(node_id)


def remove_active_output_node(node_id):
    """Removes an active output node.

    @param node_id: node id.

    """
    get_cras_control_interface().RemoveActiveOutputNode(node_id)


def remove_active_input_node(node_id):
    """Removes an active input node.

    @param node_id: node id.

    """
    get_cras_control_interface().RemoveActiveInputNode(node_id)


def get_node_id_from_node_type(node_type, is_input):
    """Gets node id from node type.

    @param types: A node type defined in CRAS_NODE_TYPES.
    @param is_input: True if the node is input. False otherwise.

    @returns: A string for node id.

    @raises: CrasUtilsError: if unique node id can not be found.

    """
    nodes = get_cras_nodes()
    find_ids = []
    for node in nodes:
        if node['Type'] == node_type and node['IsInput'] == is_input:
            find_ids.append(node['Id'])
    if len(find_ids) != 1:
        raise CrasUtilsError(
                'Can not find unique node id from node type %s' % node_type)
    return find_ids[0]
