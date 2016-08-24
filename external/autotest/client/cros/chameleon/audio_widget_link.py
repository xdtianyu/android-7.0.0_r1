# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the link between audio widgets."""

import logging
import time

from autotest_lib.client.cros.chameleon import audio_level
from autotest_lib.client.cros.chameleon import chameleon_audio_ids as ids
from autotest_lib.client.cros.chameleon import chameleon_bluetooth_audio


class WidgetBinderError(Exception):
    """Error in WidgetBinder."""
    pass


class WidgetBinder(object):
    """
    This class abstracts the binding controls between two audio widgets.

     ________          __________________          ______
    |        |        |      link        |        |      |
    | source |------->| input     output |------->| sink |
    |________|        |__________________|        |______|

    Properties:
        _source: An AudioWidget object. The audio source. This should be
                 an output widget.
        _sink: An AudioWidget object. The audio sink. This should be an
                 input widget.
        _link: An WidgetLink object to link source and sink.
        _connected: True if this binder is connected.
        _level_controller: A LevelController to set scale and balance levels of
                           source and sink.
    """
    def __init__(self, source, link, sink):
        """Initializes a WidgetBinder.

        After initialization, the binder is not connected, but the link
        is occupied until it is released.
        After connection, the channel map of link will be set to the sink
        widget, and it will remains the same until the sink widget is connected
        to a different link. This is to make sure sink widget knows the channel
        map of recorded data even after link is disconnected or released.

        @param source: An AudioWidget object for audio source.
        @param link: A WidgetLink object to connect source and sink.
        @param sink: An AudioWidget object for audio sink.

        """
        self._source = source
        self._link = link
        self._sink = sink
        self._connected = False
        self._link.occupied = True
        self._level_controller = audio_level.LevelController(
                self._source, self._sink)


    def connect(self):
        """Connects source and sink to link."""
        if self._connected:
            return

        logging.info('Connecting %s to %s', self._source.audio_port,
                     self._sink.audio_port)
        self._link.connect(self._source, self._sink)
        self._connected = True
        # Sets channel map of link to the sink widget so
        # sink widget knows the channel map of recorded data.
        self._sink.channel_map = self._link.channel_map
        self._level_controller.set_scale()


    def disconnect(self):
        """Disconnects source and sink from link."""
        if not self._connected:
            return

        logging.info('Disconnecting %s from %s', self._source.audio_port,
                     self._sink.audio_port)
        self._link.disconnect(self._source, self._sink)
        self._connected = False
        self._level_controller.reset()


    def release(self):
        """Releases the link used by this binder.

        @raises: WidgetBinderError if this binder is still connected.

        """
        if self._connected:
            raise WidgetBinderError('Can not release while connected')
        self._link.occupied = False


    def get_link(self):
        """Returns the link controlled by this binder.

        The link provides more controls than binder so user can do
        more complicated tests.

        @returns: An object of subclass of WidgetLink.

        """
        return self._link


class WidgetLinkError(Exception):
    """Error in WidgetLink."""
    pass


class WidgetLink(object):
    """
    This class abstracts the link between two audio widgets.

    Properties:
        name: A string. The link name.
        occupied: True if this widget is occupied by a widget binder.
        channel_map: A list containing current channel map. Checks docstring
                     of channel_map method of AudioInputWidget for details.

    """
    def __init__(self):
        self.name = 'Unknown'
        self.occupied = False
        self.channel_map = None


    def _check_widget_id(self, port_id, widget):
        """Checks that the port id of a widget is expected.

        @param port_id: An id defined in chameleon_audio_ids.
        @param widget: An AudioWidget object.

        @raises: WidgetLinkError if the port id of widget is not expected.
        """
        if widget.audio_port.port_id != port_id:
            raise WidgetLinkError(
                    'Link %s expects a %s widget, but gets a %s widget' % (
                            self.name, port_id, widget.audio_port.port_id))


    def connect(self, source, sink):
        """Connects source widget to sink widget.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        self._plug_input(source)
        self._plug_output(sink)


    def disconnect(self, source, sink):
        """Disconnects source widget from sink widget.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        self._unplug_input(source)
        self._unplug_output(sink)


class AudioBusLink(WidgetLink):
    """The abstraction of widget link using audio bus on audio board.

    This class handles two tasks.
    1. Audio bus routing.
    2. Plug/unplug jack using the widget handler on the DUT side.

    Note that audio jack is shared by headphone and external microphone on
    Cros device. So plugging/unplugging headphone widget will also affect
    external microphone. This should be handled outside of this class
    when we need to support complicated test case.

    Properties:
        _audio_bus: An AudioBus object.

    """
    def __init__(self, audio_bus):
        """Initializes an AudioBusLink.

        @param audio_bus: An AudioBus object.
        """
        super(AudioBusLink, self).__init__()
        self._audio_bus = audio_bus
        logging.debug('Create an AudioBusLink with bus index %d',
                      audio_bus.bus_index)


    def _plug_input(self, widget):
        """Plugs input of audio bus to the widget.

        @param widget: An AudioWidget object.

        """
        if widget.audio_port.host == 'Cros':
            widget.handler.plug()

        self._audio_bus.connect(widget.audio_port.port_id)

        logging.info(
                'Plugged audio board bus %d input to %s',
                self._audio_bus.bus_index, widget.audio_port)


    def _unplug_input(self, widget):
        """Unplugs input of audio bus from the widget.

        @param widget: An AudioWidget object.

        """
        if widget.audio_port.host == 'Cros':
            widget.handler.unplug()

        self._audio_bus.disconnect(widget.audio_port.port_id)

        logging.info(
                'Unplugged audio board bus %d input from %s',
                self._audio_bus.bus_index, widget.audio_port)


    def _plug_output(self, widget):
        """Plugs output of audio bus to the widget.

        @param widget: An AudioWidget object.

        """
        if widget.audio_port.host == 'Cros':
            widget.handler.plug()

        self._audio_bus.connect(widget.audio_port.port_id)

        logging.info(
                'Plugged audio board bus %d output to %s',
                self._audio_bus.bus_index, widget.audio_port)


    def _unplug_output(self, widget):
        """Unplugs output of audio bus from the widget.

        @param widget: An AudioWidget object.

        """
        if widget.audio_port.host == 'Cros':
            widget.handler.unplug()

        self._audio_bus.disconnect(widget.audio_port.port_id)
        logging.info(
                'Unplugged audio board bus %d output from %s',
                self._audio_bus.bus_index, widget.audio_port)


    def disconnect_audio_bus(self):
        """Disconnects all audio ports from audio bus.

        A snapshot of audio bus is retained so we can reconnect audio bus
        later.
        This method is useful when user wants to let Cros device detects
        audio jack after this link is connected. Some Cros devices
        have sensitive audio jack detection mechanism such that plugger of
        audio board can only be detected when audio bus is disconnected.

        """
        self._audio_bus_snapshot = self._audio_bus.get_snapshot()
        self._audio_bus.clear()


    def reconnect_audio_bus(self):
        """Reconnects audio ports to audio bus using snapshot."""
        self._audio_bus.restore_snapshot(self._audio_bus_snapshot)


class AudioBusToChameleonLink(AudioBusLink):
    """The abstraction for bus on audio board that is connected to Chameleon."""
    # This is the default channel map for 2-channel data recorded on
    # Chameleon through audio board.
    _DEFAULT_CHANNEL_MAP = [1, 0, None, None, None, None, None, None]

    def __init__(self, *args, **kwargs):
        super(AudioBusToChameleonLink, self).__init__(
            *args, **kwargs)
        self.name = ('Audio board bus %s to Chameleon' %
                     self._audio_bus.bus_index)
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        logging.debug(
                'Create an AudioBusToChameleonLink named %s with '
                'channel map %r', self.name, self.channel_map)


class AudioBusChameleonToPeripheralLink(AudioBusLink):
    """The abstraction for audio bus connecting Chameleon to peripheral."""
    # This is the channel map which maps 2-channel data at peripehral speaker
    # to 8 channel data at Chameleon.
    # The left channel at speaker comes from the second channel at Chameleon.
    # The right channel at speaker comes from the first channel at Chameleon.
    # Other channels at Chameleon are neglected.
    _DEFAULT_CHANNEL_MAP = [1, 0]

    def __init__(self, *args, **kwargs):
        super(AudioBusChameleonToPeripheralLink, self).__init__(
              *args, **kwargs)
        self.name = 'Audio board bus %s to peripheral' % self._audio_bus.bus_index
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        logging.debug(
                'Create an AudioBusToPeripheralLink named %s with '
                'channel map %r', self.name, self.channel_map)


class AudioBusToCrosLink(AudioBusLink):
    """The abstraction for audio bus that is connected to Cros device."""
    # This is the default channel map for 1-channel data recorded on
    # Cros device.
    _DEFAULT_CHANNEL_MAP = [0]

    def __init__(self, *args, **kwargs):
        super(AudioBusToCrosLink, self).__init__(
            *args, **kwargs)
        self.name = 'Audio board bus %s to Cros' % self._audio_bus.bus_index
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        logging.debug(
                'Create an AudioBusToCrosLink named %s with '
                'channel map %r', self.name, self.channel_map)


class USBWidgetLink(WidgetLink):
    """The abstraction for USB Cable."""

    # This is the default channel map for 2-channel data
    _DEFAULT_CHANNEL_MAP = [0, 1]
    _DELAY_BEFORE_PLUGGING_CROS_SECONDS = 3

    def __init__(self, usb_ctrl):
        """Initializes a USBWidgetLink.

        @param usb_ctrl: A USBController object.

        """
        super(USBWidgetLink, self).__init__()
        self.name = 'USB Cable'
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        self._usb_ctrl = usb_ctrl
        logging.debug(
                'Create a USBWidgetLink. Do nothing because USB cable'
                ' is dedicated')


    def connect(self, source, sink):
        """Connects source widget to sink widget.

        This method first identifies the Chameleon widget and plug it first so
        that it is visible to the Cros host for it to plug in the Cros widget.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        if source.audio_port.host == 'Chameleon':
            source.handler.plug()
            time.sleep(self._DELAY_BEFORE_PLUGGING_CROS_SECONDS)
            sink.handler.plug()
        else:
            sink.handler.plug()
            time.sleep(self._DELAY_BEFORE_PLUGGING_CROS_SECONDS)
            source.handler.plug()


    def disconnect(self, source, sink):
        """Disconnects source widget from sink widget.

        This method first identifies the Cros widget and unplugs it first while
        the Chameleon widget is still visible for the Cros host to know which
        USB port to unplug Cros widget from.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        if source.audio_port.host == 'Cros':
            source.handler.unplug()
            sink.handler.unplug()
        else:
            sink.handler.unplug()
            source.handler.unplug()


class USBToCrosWidgetLink(USBWidgetLink):
    """The abstraction for the USB cable connected to the Cros device."""

    def __init__(self, *args, **kwargs):
        """Initializes a USBToCrosWidgetLink."""
        super(USBToCrosWidgetLink, self).__init__(*args, **kwargs)
        self.name = 'USB Cable to Cros'
        logging.debug('Create a USBToCrosWidgetLink: %s', self.name)


class USBToChameleonWidgetLink(USBWidgetLink):
    """The abstraction for the USB cable connected to the Chameleon device."""

    def __init__(self, *args, **kwargs):
        """Initializes a USBToChameleonWidgetLink."""
        super(USBToChameleonWidgetLink, self).__init__(*args, **kwargs)
        self.name = 'USB Cable to Chameleon'
        logging.debug('Create a USBToChameleonWidgetLink: %s', self.name)


class HDMIWidgetLink(WidgetLink):
    """The abstraction for HDMI cable."""

    # This is the default channel map for 2-channel data recorded on
    # Chameleon through HDMI cable.
    _DEFAULT_CHANNEL_MAP = [1, 0, None, None, None, None, None, None]
    _DELAY_AFTER_PLUG_SECONDS = 6

    def __init__(self):
        super(HDMIWidgetLink, self).__init__()
        self.name = 'HDMI cable'
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        logging.debug(
                'Create an HDMIWidgetLink. Do nothing because HDMI cable'
                ' is dedicated')


    def _plug_input(self, widget):
        """Plugs input of HDMI cable to the widget using widget handler.

        @param widget: An AudioWidget object.

        """
        self._check_widget_id(ids.CrosIds.HDMI, widget)
        logging.info(
                'Plug HDMI cable input. Do nothing because HDMI cable should '
                'always be physically plugged to Cros device')


    def _unplug_input(self, widget):
        """Unplugs input of HDMI cable from the widget using widget handler.

        @param widget_handler: A WidgetHandler object.

        """
        self._check_widget_id(ids.CrosIds.HDMI, widget)
        logging.info(
                'Unplug HDMI cable input. Do nothing because HDMI cable should '
                'always be physically plugged to Cros device')


    def _plug_output(self, widget):
        """Plugs output of HDMI cable to the widget using widget handler.

        @param widget: An AudioWidget object.

        @raises: WidgetLinkError if widget handler interface is not HDMI.
        """
        self._check_widget_id(ids.ChameleonIds.HDMI, widget)
        # HDMI plugging emulation is done on Chameleon port.
        logging.info(
                'Plug HDMI cable output. This is emulated on Chameleon port')
        widget.handler.plug()
        time.sleep(self._DELAY_AFTER_PLUG_SECONDS)


    def _unplug_output(self, widget):
        """Unplugs output of HDMI cable from the widget using widget handler.

        @param widget: An AudioWidget object.

        @raises: WidgetLinkError if widget handler interface is not HDMI.
        """
        self._check_widget_id(ids.ChameleonIds.HDMI, widget)
        # HDMI plugging emulation is done on Chameleon port.
        logging.info(
                'Unplug HDMI cable output. This is emulated on Chameleon port')
        widget.handler.unplug()


class BluetoothWidgetLink(WidgetLink):
    """The abstraction for bluetooth link between Cros device and bt module."""
    # The delay after connection for cras to process the bluetooth connection
    # event and enumerate the bluetooth nodes.
    _DELAY_AFTER_CONNECT_SECONDS = 5

    def __init__(self, bt_adapter, audio_board_bt_ctrl, mac_address):
        """Initializes a BluetoothWidgetLink.

        @param bt_adapter: A BluetoothDevice object to control bluetooth
                           adapter on Cros device.
        @param audio_board_bt_ctrl: A BlueoothController object to control
                                    bluetooth module on audio board.
        @param mac_address: The MAC address of bluetooth module on audio board.

        """
        super(BluetoothWidgetLink, self).__init__()
        self._bt_adapter = bt_adapter
        self._audio_board_bt_ctrl = audio_board_bt_ctrl
        self._mac_address = mac_address


    def connect(self, source, sink):
        """Customizes the connecting sequence for bluetooth widget link.

        We need to enable bluetooth module first, then start connecting
        sequence from bluetooth adapter.
        The arguments source and sink are not used because BluetoothWidgetLink
        already has the access to bluetooth module on audio board and
        bluetooth adapter on Cros device.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        self.enable_bluetooth_module()
        self._adapter_connect_sequence()
        time.sleep(self._DELAY_AFTER_CONNECT_SECONDS)


    def disconnect(self, source, sink):
        """Customizes the disconnecting sequence for bluetooth widget link.

        The arguments source and sink are not used because BluetoothWidgetLink
        already has the access to bluetooth module on audio board and
        bluetooth adapter on Cros device.

        @param source: An AudioWidget object.
        @param sink: An AudioWidget object.

        """
        self._disable_adapter()
        self.disable_bluetooth_module()


    def enable_bluetooth_module(self):
        """Reset bluetooth module if it is not enabled."""
        if not self._audio_board_bt_ctrl.is_enabled():
            self._audio_board_bt_ctrl.reset()


    def disable_bluetooth_module(self):
        """Disables bluetooth module if it is enabled."""
        if self._audio_board_bt_ctrl.is_enabled():
            self._audio_board_bt_ctrl.disable()


    def _adapter_connect_sequence(self):
        """Scans, pairs, and connects bluetooth module to bluetooth adapter.

        If the device is already connected, skip the connection sequence.

        """
        if self._bt_adapter.device_is_connected(self._mac_address):
            logging.debug(
                    '%s is already connected, skip the connection sequence',
                    self._mac_address)
            return
        chameleon_bluetooth_audio.connect_bluetooth_module_full_flow(
                self._bt_adapter, self._mac_address)


    def _disable_adapter(self):
        """Turns off bluetooth adapter."""
        self._bt_adapter.reset_off()


    def adapter_connect_module(self):
        """Controls adapter to connect bluetooth module."""
        chameleon_bluetooth_audio.connect_bluetooth_module(
                self._bt_adapter, self._mac_address)

    def adapter_disconnect_module(self):
        """Controls adapter to disconnect bluetooth module."""
        self._bt_adapter.disconnect_device(self._mac_address)


class BluetoothHeadphoneWidgetLink(BluetoothWidgetLink):
    """The abstraction for link from Cros device headphone to bt module Rx."""

    def __init__(self, *args, **kwargs):
        """Initializes a BluetoothHeadphoneWidgetLink."""
        super(BluetoothHeadphoneWidgetLink, self).__init__(*args, **kwargs)
        self.name = 'Cros bluetooth headphone to peripheral bluetooth module'
        logging.debug('Create an BluetoothHeadphoneWidgetLink: %s', self.name)


class BluetoothMicWidgetLink(BluetoothWidgetLink):
    """The abstraction for link from bt module Tx to Cros device microphone."""

    # This is the default channel map for 1-channel data recorded on
    # Cros device using bluetooth microphone.
    _DEFAULT_CHANNEL_MAP = [0]

    def __init__(self, *args, **kwargs):
        """Initializes a BluetoothMicWidgetLink."""
        super(BluetoothMicWidgetLink, self).__init__(*args, **kwargs)
        self.name = 'Peripheral bluetooth module to Cros bluetooth mic'
        self.channel_map = self._DEFAULT_CHANNEL_MAP
        logging.debug('Create an BluetoothMicWidgetLink: %s', self.name)


class WidgetBinderChain(object):
    """Abstracts a chain of binders.

    This class supports connect, disconnect, release, just like WidgetBinder,
    except that this class handles a chain of WidgetBinders.

    """
    def __init__(self, binders):
        """Initializes a WidgetBinderChain.

        @param binders: A list of WidgetBinder.

        """
        self._binders = binders


    def connect(self):
        """Asks all binders to connect."""
        for binder in self._binders:
            binder.connect()


    def disconnect(self):
        """Asks all binders to disconnect."""
        for binder in self._binders:
            binder.disconnect()


    def release(self):
        """Asks all binders to release."""
        for binder in self._binders:
            binder.release()


    def get_binders(self):
        """Returns all the binders.

        @returns: A list of binders.

        """
        return self._binders
