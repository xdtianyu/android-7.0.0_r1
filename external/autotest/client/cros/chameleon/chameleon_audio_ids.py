# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Audio port ids shared in Chameleon audio test framework"""


class ChameleonIds(object):
    """Ids for audio ports on Chameleon

    An Id is composed by host name 'Chameleon' and interface name e.g. 'HDMI'.
    Note that the interface name must match what get_connector_type method
    returns on a ChameleonPort so ChameleonPortFinder can find the port.

    """
    HDMI = 'Chameleon HDMI'
    LINEIN = 'Chameleon LineIn'
    LINEOUT = 'Chameleon LineOut'
    MIC = 'Chameleon Mic'
    USBIN = 'Chameleon USBIn'
    USBOUT = 'Chameleon USBOut'

    SINK_PORTS = [HDMI, LINEIN, MIC, USBIN]
    SOURCE_PORTS = [LINEOUT, USBOUT]


class CrosIds(object):
    """Ids for audio ports on Cros device.

    Note that an bidirectional interface like 3.5mm jack is separated to
    two interfaces, that is, 'Headphone' and 'External Mic'.

    """
    HDMI = 'Cros HDMI'
    HEADPHONE = 'Cros Headphone'
    EXTERNAL_MIC = 'Cros External Mic'
    SPEAKER = 'Cros Speaker'
    INTERNAL_MIC = 'Cros Internal Mic'
    BLUETOOTH_HEADPHONE = 'Cros Bluetooth Headphone'
    BLUETOOTH_MIC = 'Cros Bluetooth Mic'
    USBIN = 'Cros USBIn'
    USBOUT = 'Cros USBOut'

    SINK_PORTS = [EXTERNAL_MIC, INTERNAL_MIC, BLUETOOTH_MIC, USBIN]
    SOURCE_PORTS = [HDMI, HEADPHONE, SPEAKER, BLUETOOTH_HEADPHONE, USBOUT]


class PeripheralIds(object):
    """Ids for peripherals.

    These peripherals will be accessible by Cros device/Chameleon through
    audio board.

    """
    SPEAKER = 'Peripheral Speaker'
    MIC = 'Peripheral Mic'

    # Peripheral devices should have two roles but we only care one.
    # For example, to test internal microphone on Cros device:
    #
    #                                         (air)
    #                    Peripheral Speaker -------> Internal Microphone
    #                         ------                  ------
    # Chameleon LineOut ----> |    |                  |    |
    #                         ------                  ------
    #                        Audio board             Cros device
    #
    # In the above example, peripheral speaker is a sink as it takes signal
    # from audio board. It should be a source as peripheral speaker transfer
    # signal to internal microphone of Cros device,
    # However, we do not abstract air as a link because it does not contain
    # properties like level, channel_map, occupied to manipulate.
    # So, we set peripheral speaker to be a sink to reflect the part related
    # to audio bus.
    #
    # For example, to test internal speaker on Cros device:
    #
    #                                         (air)
    #                    Peripheral Micropone <----- Internal Speaker
    #                         ------                  ------
    # Chameleon LineIn <----  |    |                  |    |
    #                         ------                  ------
    #                        Audio board             Cros device
    #
    # In the above example, peripheral microphone is a source as it feeds signal
    # to audio board. It should be a sink as peripheral microphone receives
    # signal from internal speaker of Cros device.
    # However, we do not abstract air as a link because it does not contain
    # properties like level, channel_map, occupied to manipulate.
    # So, we set peripheral microphone to be a source to reflect the part related
    # to audio bus.

    BLUETOOTH_DATA_RX = ('Peripheral Bluetooth Signal Output and Data Receiver')
    BLUETOOTH_DATA_TX = ('Peripheral Bluetooth Signal Input and Data '
                         'Transmitter')

    # Bluetooth module has both source and sink roles.
    # When Cros device plays audio to bluetooth module data receiver through
    # bluetooth connection, bluetooth module is a sink of audio signal.
    # When we route audio signal from bluetooth signal output to Chameleon
    # Line-In, bluetooth module is a signal source in terms of audio bus.
    #
    #                     Bluetooth module
    #
    #                    signal     data      (bluetooth)
    #                    output    receiver <------------ Bluetooth Headphone
    #                         ------                        ------
    # Chameleon LineIn <----  |    |                        |    |
    #                         ------                        ------
    #                        Audio board                   Cros device
    #
    #
    # When Cros device records audio from bluetooth module data transmitter
    # through bluetooth connection, bluetooth module is a source of audio
    # signal. When we route audio signal from Chameleon Line-Out to bluetooth
    # signal input, bluetooth module is a signal sink in terms of audio bus.
    #
    #                     Bluetooth module
    #
    #                    signal     data      (bluetooth)
    #                    input    transmitter -----------> Bluetooth Microphone
    #                         ------                        ------
    # Chameleon LineOut ----> |    |                        |    |
    #                         ------                        ------
    #                        Audio board                   Cros device
    #
    # From above diagram it is clear that "signal output" is connected to
    # "data receiver", while "signal input" is connected to "data transmitter".
    # To simplify the number of nodes, we group "signal output" and
    # "data receiver" into one Id, and group "signal input" and
    # "data transmitter" into one Id. Also, we let these two Ids be both source
    # and sink.
    SOURCE_PORTS = [MIC, BLUETOOTH_DATA_RX, BLUETOOTH_DATA_TX]
    SINK_PORTS = [SPEAKER, BLUETOOTH_DATA_RX, BLUETOOTH_DATA_TX]


SINK_PORTS = []
for cls in [ChameleonIds, CrosIds, PeripheralIds]:
    SINK_PORTS += cls.SINK_PORTS

SOURCE_PORTS = []
for cls in [ChameleonIds, CrosIds, PeripheralIds]:
    SOURCE_PORTS += cls.SOURCE_PORTS


def get_host(port_id):
    """Parses given port_id to get host name.

    @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                    PeripheralIds.

    @returns: Host name. A string in ['Chameleon', 'Cros', 'Peripheral'].

    @raises: ValueError if port_id is invalid.

    """
    host = port_id.split()[0]
    if host not in ['Chameleon', 'Cros', 'Peripheral']:
        raise ValueError('Not a valid port id: %r' % port_id)
    return host


def get_interface(port_id):
    """Parses given port_id to get interface name.

    @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                    PeripheralIds.

    @returns: Interface name. A string, e.g. 'HDMI', 'LineIn'.

    @raises: ValueError if port_id is invalid.

    """
    try:
        return port_id.split(' ', 1)[1]
    except IndexError:
        raise ValueError('Not a valid port id: %r' % port_id)


def get_role(port_id):
    """Gets the role of given port_id.

    @param port_id: A string, that is, id in ChameleonIds, CrosIds, or
                    PeripheralIds.

    @returns: 'source' or 'sink'.

    @raises: ValueError if port_id is invalid.

    """
    if port_id in SOURCE_PORTS:
        return 'source'
    if port_id in SINK_PORTS:
        return 'sink'
    if port_id in SOURCE_PORTS and port_id in SINK_PORTS:
        return 'sink | source'
    raise ValueError('Not a valid port id: %r' % port_id)
