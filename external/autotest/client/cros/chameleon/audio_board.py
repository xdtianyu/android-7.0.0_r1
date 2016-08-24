# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the audio board interface."""

import logging

from autotest_lib.client.cros.chameleon import chameleon_audio_ids as ids


class AudioBoard(object):
    """AudioBoard is an abstraction of an audio board on a Chameleon board.

    It provides methods to control audio board.

    A ChameleonConnection object is passed to the construction.

    """
    def __init__(self, chameleon_connection):
        """Constructs an AudioBoard.

        @param chameleon_connection: A ChameleonConnection object.

        """
        self._audio_buses = {
                1: AudioBus(1, chameleon_connection),
                2: AudioBus(2, chameleon_connection)}

        self._jack_plugger = None
        try:
            self._jack_plugger = AudioJackPlugger(chameleon_connection)
        except AudioJackPluggerException:
            logging.warning(
                    'There is no jack plugger on this audio board.')
            self._jack_plugger = None

        self._bluetooth_controller = BluetoothController(chameleon_connection)


    def get_audio_bus(self, bus_index):
        """Gets an audio bus on this audio board.

        @param bus_index: The bus index 1 or 2.

        @returns: An AudioBus object.

        """
        return self._audio_buses[bus_index]


    def get_jack_plugger(self):
        """Gets an AudioJackPlugger on this audio board.

        @returns: An AudioJackPlugger object if there is an audio jack plugger.
                  None if there is no audio jack plugger.

        """
        return self._jack_plugger


    def get_bluetooth_controller(self):
        """Gets an BluetoothController on this audio board.

        @returns: An BluetoothController object.

        """
        return self._bluetooth_controller


class AudioBus(object):
    """AudioBus is an abstraction of an audio bus on an audio board.

    It provides methods to control audio bus.

    A ChameleonConnection object is passed to the construction.

    @properties:
        bus_index: The bus index 1 or 2.

    """
    # Maps port id defined in chameleon_audio_ids to endpoint name used in
    # chameleond audio bus API.
    _PORT_ID_AUDIO_BUS_ENDPOINT_MAP = {
            ids.ChameleonIds.LINEIN: 'Chameleon FPGA line-in',
            ids.ChameleonIds.LINEOUT: 'Chameleon FPGA line-out',
            ids.CrosIds.HEADPHONE: 'Cros device headphone',
            ids.CrosIds.EXTERNAL_MIC: 'Cros device external microphone',
            ids.PeripheralIds.SPEAKER: 'Peripheral speaker',
            ids.PeripheralIds.MIC: 'Peripheral microphone',
            ids.PeripheralIds.BLUETOOTH_DATA_RX:
                    'Bluetooth module output',
            ids.PeripheralIds.BLUETOOTH_DATA_TX:
                    'Bluetooth module input'}


    class AudioBusSnapshot(object):
        """Abstracts the snapshot of AudioBus for user to restore it later."""
        def __init__(self, endpoints):
            """Initializes an AudioBusSnapshot.

            @param endpoints: A set of endpoints to keep a copy.

            """
            self._endpoints = endpoints.copy()


    def __init__(self, bus_index, chameleon_connection):
        """Constructs an AudioBus.

        @param bus_index: The bus index 1 or 2.
        @param chameleon_connection: A ChameleonConnection object.

        """
        self.bus_index = bus_index
        self._chameleond_proxy = chameleon_connection.chameleond_proxy
        self._connected_endpoints = set()


    def _get_endpoint_name(self, port_id):
        """Gets the endpoint name used in audio bus API.

        @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                        PeripheralIds defined in chameleon_audio_ids.

        @returns: The endpoint name for the port used in audio bus API.

        """
        return self._PORT_ID_AUDIO_BUS_ENDPOINT_MAP[port_id]


    def _connect_endpoint(self, endpoint):
        """Connects an endpoint to audio bus.

        @param endpoint: An endpoint name in _PORT_ID_AUDIO_BUS_ENDPOINT_MAP.

        """
        logging.debug(
                'Audio bus %s is connecting endpoint %s',
                self.bus_index, endpoint)
        self._chameleond_proxy.AudioBoardConnect(self.bus_index, endpoint)
        self._connected_endpoints.add(endpoint)


    def _disconnect_endpoint(self, endpoint):
        """Disconnects an endpoint from audio bus.

        @param endpoint: An endpoint name in _PORT_ID_AUDIO_BUS_ENDPOINT_MAP.

        """
        logging.debug(
                'Audio bus %s is disconnecting endpoint %s',
                self.bus_index, endpoint)
        self._chameleond_proxy.AudioBoardDisconnect(self.bus_index, endpoint)
        self._connected_endpoints.remove(endpoint)


    def connect(self, port_id):
        """Connects an audio port to this audio bus.

        @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                        PeripheralIds defined in chameleon_audio_ids.

        """
        endpoint = self._get_endpoint_name(port_id)
        self._connect_endpoint(endpoint)


    def disconnect(self, port_id):
        """Disconnects an audio port from this audio bus.

        @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                        PeripheralIds defined in chameleon_audio_ids.

        """
        endpoint = self._get_endpoint_name(port_id)
        self._disconnect_endpoint(endpoint)


    def clear(self):
        """Disconnects all audio port from this audio bus."""
        self._disconnect_all_endpoints()


    def _disconnect_all_endpoints(self):
        """Disconnects all endpoints from this audio bus."""
        for endpoint in self._connected_endpoints.copy():
            self._disconnect_endpoint(endpoint)


    def get_snapshot(self):
        """Gets the snapshot of AudioBus so user can restore it later.

        @returns: An AudioBus.AudioBusSnapshot object.

        """
        return self.AudioBusSnapshot(self._connected_endpoints)


    def restore_snapshot(self, snapshot):
        """Restore the snapshot.

        @param: An AudioBus.AudioBusSnapshot object got from get_snapshot.

        """
        self._disconnect_all_endpoints()
        logging.debug('Restoring snapshot with %s', snapshot._endpoints)
        for endpoint in snapshot._endpoints:
            self._connect_endpoint(endpoint)


class AudioJackPluggerException(Exception):
    """Errors in AudioJackPlugger."""
    pass


class AudioJackPlugger(object):
    """AudioJackPlugger is an abstraction of plugger controlled by audio board.

    There is a motor in the audio box which can plug/unplug 3.5mm 4-ring
    audio cable to/from audio jack of Cros deivce.
    This motor is controlled by audio board.

    A ChameleonConnection object is passed to the construction.

    """
    def __init__(self, chameleon_connection):
        """Constructs an AudioJackPlugger.

        @param chameleon_connection: A ChameleonConnection object.

        @raises:
            AudioJackPluggerException if there is no jack plugger on
            this audio board.

        """
        self._chameleond_proxy = chameleon_connection.chameleond_proxy
        if not self._chameleond_proxy.AudioBoardHasJackPlugger():
            raise AudioJackPluggerException(
                'There is no jack plugger on audio board. '
                'Perhaps the audio board is not connected to audio box.')


    def plug(self):
        """Plugs the audio cable into audio jack of Cros device."""
        self._chameleond_proxy.AudioBoardAudioJackPlug()
        logging.info('Plugged 3.5mm audio cable to Cros device')


    def unplug(self):
        """Unplugs the audio cable from audio jack of Cros device."""
        self._chameleond_proxy.AudioBoardAudioJackUnplug()
        logging.info('Unplugged 3.5mm audio cable from Cros device')


class BluetoothController(object):
    """An abstraction of bluetooth module on audio board.

    There is a bluetooth module on the audio board. It can be controlled through
    API provided by chameleon proxy.

    """
    def __init__(self, chameleon_connection):
        """Constructs an BluetoothController.

        @param chameleon_connection: A ChameleonConnection object.

        """
        self._chameleond_proxy = chameleon_connection.chameleond_proxy


    def reset(self):
        """Resets the bluetooth module."""
        self._chameleond_proxy.AudioBoardResetBluetooth()
        logging.info('Resets bluetooth module on audio board.')


    def disable(self):
        """Disables the bluetooth module."""
        self._chameleond_proxy.AudioBoardDisableBluetooth()
        logging.info('Disables bluetooth module on audio board.')


    def is_enabled(self):
        """Checks if the bluetooth module is enabled.

        @returns: True if bluetooth module is enabled. False otherwise.

        """
        return self._chameleond_proxy.AudioBoardIsBluetoothEnabled()
