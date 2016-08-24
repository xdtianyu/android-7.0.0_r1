# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the framework for audio tests using chameleon."""

import logging
from contextlib import contextmanager

from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.chameleon import audio_widget
from autotest_lib.client.cros.chameleon import audio_widget_link
from autotest_lib.server.cros.bluetooth import bluetooth_device
from autotest_lib.client.cros.chameleon import chameleon_audio_ids as ids
from autotest_lib.client.cros.chameleon import chameleon_info


class AudioPort(object):
    """
    This class abstracts an audio port in audio test framework. A port is
    identified by its host and interface. Available hosts and interfaces
    are listed in chameleon_audio_ids.

    Properties:
        port_id: The port id defined in chameleon_audio_ids.
        host: The host of this audio port, e.g. 'Chameleon', 'Cros',
              'Peripheral'.
        interface: The interface of this audio port, e.g. 'HDMI', 'Headphone'.
        role: The role of this audio port, that is, 'source' or
              'sink'. Note that bidirectional interface like 3.5mm
              jack is separated to two interfaces 'Headphone' and
             'External Mic'.

    """
    def __init__(self, port_id):
        """Initialize an AudioPort with port id string.

        @param port_id: A port id string defined in chameleon_audio_ids.

        """
        logging.debug('Creating AudioPort with port_id: %s', port_id)
        self.port_id = port_id
        self.host = ids.get_host(port_id)
        self.interface = ids.get_interface(port_id)
        self.role = ids.get_role(port_id)
        logging.debug('Created AudioPort: %s', self)


    def __str__(self):
        """String representation of audio port.

        @returns: The string representation of audio port which is composed by
                  host, interface, and role.

        """
        return '( %s | %s | %s )' % (
                self.host, self.interface, self.role)


class AudioLinkFactoryError(Exception):
    """Error in AudioLinkFactory."""
    pass


class AudioLinkFactory(object):
    """
    This class provides method to create link that connects widgets.
    This is used by AudioWidgetFactory when user wants to create binder for
    widgets.

    Properties:
        _audio_bus_links: A dict containing mapping from index number
                          to object of AudioBusLink's subclass.
        _audio_board: An AudioBoard object to access Chameleon
                      audio board functionality.

    """

    # Maps pair of widgets to widget link of different type.
    LINK_TABLE = {
        (ids.CrosIds.HDMI, ids.ChameleonIds.HDMI):
                audio_widget_link.HDMIWidgetLink,
        (ids.CrosIds.HEADPHONE, ids.ChameleonIds.LINEIN):
                audio_widget_link.AudioBusToChameleonLink,
        (ids.ChameleonIds.LINEOUT, ids.CrosIds.EXTERNAL_MIC):
                audio_widget_link.AudioBusToCrosLink,
        (ids.ChameleonIds.LINEOUT, ids.PeripheralIds.SPEAKER):
                audio_widget_link.AudioBusChameleonToPeripheralLink,
        (ids.PeripheralIds.MIC, ids.ChameleonIds.LINEIN):
                audio_widget_link.AudioBusToChameleonLink,
        (ids.PeripheralIds.BLUETOOTH_DATA_RX,
         ids.ChameleonIds.LINEIN):
                audio_widget_link.AudioBusToChameleonLink,
        (ids.ChameleonIds.LINEOUT,
         ids.PeripheralIds.BLUETOOTH_DATA_TX):
                audio_widget_link.AudioBusChameleonToPeripheralLink,
        (ids.CrosIds.BLUETOOTH_HEADPHONE,
         ids.PeripheralIds.BLUETOOTH_DATA_RX):
                audio_widget_link.BluetoothHeadphoneWidgetLink,
        (ids.PeripheralIds.BLUETOOTH_DATA_TX,
         ids.CrosIds.BLUETOOTH_MIC):
                audio_widget_link.BluetoothMicWidgetLink,
        (ids.CrosIds.USBOUT, ids.ChameleonIds.USBIN):
                audio_widget_link.USBToChameleonWidgetLink,
        (ids.ChameleonIds.USBOUT, ids.CrosIds.USBIN):
                audio_widget_link.USBToCrosWidgetLink,
        # TODO(cychiang): Add link for other widget pairs.
    }

    def __init__(self, cros_host):
        """Initializes an AudioLinkFactory.

        @param cros_host: A CrosHost object to access Cros device.

        """
        # There are two audio buses on audio board. Initializes these links
        # to None. They may be changed to objects of AudioBusLink's subclass.
        self._audio_bus_links = {1: None, 2: None}
        self._cros_host = cros_host
        self._chameleon_board = cros_host.chameleon
        self._audio_board = self._chameleon_board.get_audio_board()
        self._bluetooth_device = None
        self._usb_ctrl = None


    def _acquire_audio_bus_index(self):
        """Acquires an available audio bus index that is not occupied yet.

        @returns: A number.

        @raises: AudioLinkFactoryError if there is no available
                 audio bus.
        """
        for index, bus in self._audio_bus_links.iteritems():
            if not (bus and bus.occupied):
                return index

        raise AudioLinkFactoryError('No available audio bus')


    def create_link(self, source, sink):
        """Creates a widget link for two audio widgets.

        @param source: An AudioWidget.
        @param sink: An AudioWidget.

        @returns: An object of WidgetLink's subclass.

        @raises: AudioLinkFactoryError if there is no link between
            source and sink.

        """
        # Finds the available link types from LINK_TABLE.
        link_type = self.LINK_TABLE.get((source.port_id, sink.port_id), None)
        if not link_type:
            raise AudioLinkFactoryError(
                    'No supported link between %s and %s' % (
                            source.port_id, sink.port_id))

        # There is only one dedicated HDMI cable, just use it.
        if link_type == audio_widget_link.HDMIWidgetLink:
            link = audio_widget_link.HDMIWidgetLink()

        # Acquires audio bus if there is available bus.
        # Creates a bus of AudioBusLink's subclass that is more
        # specific than AudioBusLink.
        # Controls this link using AudioBus object obtained from AudioBoard
        # object.
        elif issubclass(link_type, audio_widget_link.AudioBusLink):
            bus_index = self._acquire_audio_bus_index()
            link = link_type(self._audio_board.get_audio_bus(bus_index))
            self._audio_bus_links[bus_index] = link
        elif issubclass(link_type, audio_widget_link.BluetoothWidgetLink):
            # To connect bluetooth adapter on Cros device to bluetooth module on
            # chameleon board, we need to access bluetooth adapter on Cros host
            # using BluetoothDevice, and access bluetooth module on
            # audio board using BluetoothController. Finally, the MAC address
            # of bluetooth module is queried through chameleon_info because
            # it is not probeable on Chameleon board.

            # Initializes a BluetoothDevice object if needed. And reuse this
            # object for future bluetooth link usage.
            if not self._bluetooth_device:
                self._bluetooth_device = bluetooth_device.BluetoothDevice(
                        self._cros_host)

            link = link_type(
                    self._bluetooth_device,
                    self._audio_board.get_bluetooth_controller(),
                    chameleon_info.get_bluetooth_mac_address(
                            self._chameleon_board))
        elif issubclass(link_type, audio_widget_link.USBWidgetLink):
            # Aside from managing connection between USB audio gadget driver on
            # Chameleon with Cros device, USBWidgetLink also handles changing
            # the gadget driver's configurations, through the USBController that
            # is passed to it at initialization.
            if not self._usb_ctrl:
                self._usb_ctrl = self._chameleon_board.get_usb_controller()

            link = link_type(self._usb_ctrl)
        else:
            raise NotImplementedError('Link %s is not implemented' % link_type)

        return link


class AudioWidgetFactoryError(Exception):
    """Error in AudioWidgetFactory."""
    pass


class AudioWidgetFactory(object):
    """
    This class provides methods to create widgets and binder of widgets.
    User can use binder to setup audio paths. User can use widgets to control
    record/playback on different ports on Cros device or Chameleon.

    Properties:
        _audio_facade: An AudioFacadeRemoteAdapter to access Cros device audio
                       functionality. This is created by the
                       'factory' argument passed to the constructor.
        _chameleon_board: A ChameleonBoard object to access Chameleon
                          functionality.
        _link_factory: An AudioLinkFactory that creates link for widgets.

    """
    def __init__(self, factory, cros_host):
        """Initializes a AudioWidgetFactory

        @param factory: A facade factory to access Cros device functionality.
                        Currently only audio facade is used, but we can access
                        other functionalities including display and video by
                        facades created by this facade factory.
        @param cros_host: A CrosHost object to access Cros device.

        """
        self._audio_facade = factory.create_audio_facade()
        self._usb_facade = factory.create_usb_facade()
        self._cros_host = cros_host
        self._chameleon_board = cros_host.chameleon
        self._link_factory = AudioLinkFactory(cros_host)


    def create_widget(self, port_id):
        """Creates a AudioWidget given port id string.

        @param port_id: A port id string defined in chameleon_audio_ids.

        @returns: An AudioWidget that is actually a
                  (Chameleon/Cros/Peripheral)(Input/Output)Widget.

        """
        def _create_chameleon_handler(audio_port):
            """Creates a ChameleonWidgetHandler for a given AudioPort.

            @param audio_port: An AudioPort object.

            @returns: A Chameleon(Input/Output)WidgetHandler depending on
                      role of audio_port.

            """
            if audio_port.role == 'sink':
                return audio_widget.ChameleonInputWidgetHandler(
                        self._chameleon_board, audio_port.interface)
            else:
                if audio_port.port_id == ids.ChameleonIds.LINEOUT:
                    return audio_widget.ChameleonLineOutOutputWidgetHandler(
                            self._chameleon_board, audio_port.interface)
                else:
                    return audio_widget.ChameleonOutputWidgetHandler(
                            self._chameleon_board, audio_port.interface)


        def _create_cros_handler(audio_port):
            """Creates a CrosWidgetHandler for a given AudioPort.

            @param audio_port: An AudioPort object.

            @returns: A Cros(Input/Output)WidgetHandler depending on
                      role of audio_port.

            """
            is_usb = audio_port.port_id in [ids.CrosIds.USBIN,
                                            ids.CrosIds.USBOUT]
            audio_board = self._chameleon_board.get_audio_board()
            if audio_board:
                jack_plugger = audio_board.get_jack_plugger()
            else:
                jack_plugger = None

            if is_usb:
                plug_handler = audio_widget.USBPlugHandler(self._usb_facade)
            elif jack_plugger:
                plug_handler = audio_widget.JackPluggerPlugHandler(jack_plugger)
            else:
                plug_handler = audio_widget.DummyPlugHandler()

            if audio_port.role == 'sink':
                if is_usb:
                    return audio_widget.CrosUSBInputWidgetHandler(
                            self._audio_facade, plug_handler)
                else:
                    return audio_widget.CrosInputWidgetHandler(
                            self._audio_facade, plug_handler)
            else:
                return audio_widget.CrosOutputWidgetHandler(self._audio_facade,
                                                            plug_handler)


        def _create_audio_widget(audio_port, handler):
            """Creates an AudioWidget for given AudioPort using WidgetHandler.

            Creates an AudioWidget with the role of audio_port. Put
            the widget handler into the widget so the widget can handle
            action requests.

            @param audio_port: An AudioPort object.
            @param handler: A WidgetHandler object.

            @returns: An Audio(Input/Output)Widget depending on
                      role of audio_port.

            @raises: AudioWidgetFactoryError if fail to create widget.

            """
            if audio_port.host in ['Chameleon', 'Cros']:
                if audio_port.role == 'sink':
                    return audio_widget.AudioInputWidget(audio_port, handler)
                else:
                    return audio_widget.AudioOutputWidget(audio_port, handler)
            elif audio_port.host == 'Peripheral':
                return audio_widget.PeripheralWidget(audio_port, handler)
            else:
                raise AudioWidgetFactoryError(
                        'The host %s is not valid' % audio_port.host)


        audio_port = AudioPort(port_id)
        if audio_port.host == 'Chameleon':
            handler = _create_chameleon_handler(audio_port)
        elif audio_port.host == 'Cros':
            handler = _create_cros_handler(audio_port)
        elif audio_port.host == 'Peripheral':
            handler = audio_widget.PeripheralWidgetHandler()

        return _create_audio_widget(audio_port, handler)


    def _create_widget_binder(self, source, sink):
        """Creates a WidgetBinder for two AudioWidgets.

        @param source: An AudioWidget.
        @param sink: An AudioWidget.

        @returns: A WidgetBinder object.

        """
        return audio_widget_link.WidgetBinder(
                source, self._link_factory.create_link(source, sink), sink)


    def create_binder(self, *widgets):
        """Creates a WidgetBinder or a WidgetChainBinder for AudioWidgets.

        @param widgets: A list of widgets that should be linked in a chain.

        @returns: A WidgetBinder for two widgets. A WidgetBinderChain object
                  for three or more widgets.

        """
        if len(widgets) == 2:
            return self._create_widget_binder(widgets[0], widgets[1])
        binders = []
        for index in xrange(len(widgets) - 1):
            binders.append(
                    self._create_widget_binder(
                            widgets[index],  widgets[index + 1]))

        return audio_widget_link.WidgetBinderChain(binders)


def compare_recorded_result(golden_file, recorder, method, parameters=None):
    """Check recoded audio in a AudioInputWidget against a golden file.

    Compares recorded data with golden data by cross correlation method.
    Refer to audio_helper.compare_data for details of comparison.

    @param golden_file: An AudioTestData object that serves as golden data.
    @param recorder: An AudioInputWidget that has recorded some audio data.
    @param method: The method to compare recorded result. Currently,
                   'correlation' and 'frequency' are supported.
    @param parameters: A dict containing parameters for method.

    @returns: True if the recorded data and golden data are similar enough.

    """
    logging.info('Comparing recorded data with golden file %s ...',
                 golden_file.path)
    return audio_helper.compare_data(
            golden_file.get_binary(), golden_file.data_format,
            recorder.get_binary(), recorder.data_format, recorder.channel_map,
            method, parameters)


@contextmanager
def bind_widgets(binder):
    """Context manager for widget binders.

    Connects widgets in the beginning. Disconnects widgets and releases binder
    in the end.

    @param binder: A WidgetBinder object or a WidgetBinderChain object.

    E.g. with bind_widgets(binder):
             do something on widget.

    """
    try:
        binder.connect()
        yield
    finally:
        binder.disconnect()
        binder.release()
