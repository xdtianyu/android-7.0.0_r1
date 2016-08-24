# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
from collections import namedtuple

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon

ChameleonPorts = namedtuple('ChameleonPorts', 'connected failed')


class ChameleonPortFinder(object):
    """
    Responsible for finding all ports connected to the chameleon board.

    It does not verify if these ports are connected to DUT.

    """

    def __init__(self, chameleon_board):
        """
        @param chameleon_board: a ChameleonBoard object representing the
                                Chameleon board whose ports we are interested
                                in finding.

        """
        self.chameleon_board = chameleon_board
        self.connected = None
        self.failed = None


    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 ports as the first element and failed ports as second element.

        """
        self.connected = self.chameleon_board.get_all_ports()
        self.failed = []

        return ChameleonPorts(self.connected, self.failed)


    def find_port(self, interface):
        """
        @param interface: string, the interface. e.g: HDMI, DP, VGA
        @returns a ChameleonPort object if port is found, else None.

        """
        connected_ports = self.find_all_ports().connected

        for port in connected_ports:
            if port.get_connector_type().lower() == interface.lower():
                return port

        return None


    def __str__(self):
        ports_to_str = lambda ports: ', '.join(
                '%s(%d)' % (p.get_connector_type(), p.get_connector_id())
                for p in ports)

        if self.connected is None:
            text = 'No port information. Did you run find_all_ports()?'
        elif self.connected == []:
            text = 'No port detected on the Chameleon board.'
        else:
            text = ('Detected %d connected port(s): %s. \t'
                    % (len(self.connected), ports_to_str(self.connected)))

        if self.failed:
            text += ('DUT failed to detect Chameleon ports: %s'
                     % ports_to_str(self.failed))

        return text


class ChameleonInputFinder(ChameleonPortFinder):
    """
    Responsible for finding all input ports connected to the chameleon board.

    """

    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 input ports as the first element and failed ports as second
                 element.

        """
        self.connected = self.chameleon_board.get_all_inputs()
        self.failed = []

        return ChameleonPorts(self.connected, self.failed)


class ChameleonOutputFinder(ChameleonPortFinder):
    """
    Responsible for finding all output ports connected to the chameleon board.

    """

    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 output ports as the first element and failed ports as the
                 second element.

        """
        self.connected = self.chameleon_board.get_all_outputs()
        self.failed = []

        return ChameleonPorts(self.connected, self.failed)


class ChameleonVideoInputFinder(ChameleonInputFinder):
    """
    Responsible for finding all video inputs connected to the chameleon board.

    It also verifies if these ports are connected to DUT.

    """

    REPLUG_DELAY_SEC = 1

    def __init__(self, chameleon_board, display_facade):
        """
        @param chameleon_board: a ChameleonBoard object representing the
                                Chameleon board whose ports we are interested
                                in finding.
        @param display_facade: a display facade object, to access the DUT
                               display functionality, either locally or
                               remotely.

        """
        super(ChameleonVideoInputFinder, self).__init__(chameleon_board)
        self.display_facade = display_facade
        self._TIMEOUT_VIDEO_STABLE_PROBE = 10


    def _yield_all_ports(self, failed_ports=None, raise_error=False):
        """
        Yields all connected video ports and ensures every of them plugged.

        @param failed_ports: A list to append the failed port or None.
        @param raise_error: True to raise TestFail if no connected video port.
        @yields every connected ChameleonVideoInput which is ensured plugged
                before yielding.

        @raises TestFail if raise_error is True and no connected video port.

        """
        yielded = False
        all_ports = super(ChameleonVideoInputFinder, self).find_all_ports()

        # unplug all ports
        for port in all_ports.connected:
            if port.has_video_support():
                chameleon.ChameleonVideoInput(port).unplug()

        for port in all_ports.connected:
            # Skip the non-video port.
            if not port.has_video_support():
                continue

            video_port = chameleon.ChameleonVideoInput(port)
            connector_type = video_port.get_connector_type()
            # Plug the port to make it visible.
            video_port.plug()
            try:
                # DUT takes some time to respond. Wait until the video signal
                # to stabilize and wait for the connector change.
                video_stable = video_port.wait_video_input_stable(
                        self._TIMEOUT_VIDEO_STABLE_PROBE)
                output = utils.wait_for_value_changed(
                        self.display_facade.get_external_connector_name,
                        old_value=False)

                if not output:
                    logging.warn('Maybe flaky that no display detected. Retry.')
                    video_port.unplug()
                    time.sleep(self.REPLUG_DELAY_SEC)
                    video_port.plug()
                    video_stable = video_port.wait_video_input_stable(
                            self._TIMEOUT_VIDEO_STABLE_PROBE)
                    output = utils.wait_for_value_changed(
                            self.display_facade.get_external_connector_name,
                            old_value=False)

                logging.info('CrOS detected external connector: %r', output)

                if output:
                    yield video_port
                    yielded = True
                else:
                    if failed_ports is not None:
                       failed_ports.append(video_port)
                    logging.error('CrOS failed to see any external display')
                    if not video_stable:
                        logging.warn('Chameleon timed out waiting CrOS video')
            finally:
                # Unplug the port not to interfere with other tests.
                video_port.unplug()

        if raise_error and not yielded:
            raise error.TestFail('No connected video port found between CrOS '
                                 'and Chameleon.')


    def iterate_all_ports(self):
        """
        Iterates all connected video ports and ensures every of them plugged.

        It is used via a for statement, like the following:

            finder = ChameleonVideoInputFinder(chameleon_board, display_facade)
            for chameleon_port in finder.iterate_all_ports()
                # chameleon_port is automatically plugged before this line.
                do_some_test_on(chameleon_port)
                # chameleon_port is automatically unplugged after this line.

        @yields every connected ChameleonVideoInput which is ensured plugged
                before yeilding.

        @raises TestFail if no connected video port.

        """
        return self._yield_all_ports(raise_error=True)


    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 video inputs as the first element and failed ports as second
                 element.

        """
        dut_failed_ports = []
        connected_ports = list(self._yield_all_ports(dut_failed_ports))
        self.connected = connected_ports
        self.failed = dut_failed_ports

        return ChameleonPorts(connected_ports, dut_failed_ports)


class ChameleonAudioInputFinder(ChameleonInputFinder):
    """
    Responsible for finding all audio inputs connected to the chameleon board.

    It does not verify if these ports are connected to DUT.

    """

    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 audio inputs as the first element and failed ports as second
                 element.

        """
        all_ports = super(ChameleonAudioInputFinder, self).find_all_ports()
        self.connected = [chameleon.ChameleonAudioInput(port)
                          for port in all_ports.connected
                          if port.has_audio_support()]
        self.failed = []

        return ChameleonPorts(self.connected, self.failed)


class ChameleonAudioOutputFinder(ChameleonOutputFinder):
    """
    Responsible for finding all audio outputs connected to the chameleon board.

    It does not verify if these ports are connected to DUT.

    """

    def find_all_ports(self):
        """
        @returns a named tuple ChameleonPorts() containing a list of connected
                 audio outputs as the first element and failed ports as second
                 element.

        """
        all_ports = super(ChameleonAudioOutputFinder, self).find_all_ports()
        self.connected = [chameleon.ChameleonAudioOutput(port)
                          for port in all_ports.connected
                          if port.has_audio_support()]
        self.failed = []

        return ChameleonPorts(self.connected, self.failed)
