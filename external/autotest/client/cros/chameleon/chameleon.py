# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import logging
import socket
import time
import xmlrpclib
from contextlib import contextmanager

from PIL import Image

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_board
from autotest_lib.client.cros.chameleon import edid as edid_lib
from autotest_lib.client.cros.chameleon import usb_controller


CHAMELEON_PORT = 9992


class ChameleonConnectionError(error.TestError):
    """Indicates that connecting to Chameleon failed.

    It is fatal to the test unless caught.
    """
    pass


class ChameleonConnection(object):
    """ChameleonConnection abstracts the network connection to the board.

    ChameleonBoard and ChameleonPort use it for accessing Chameleon RPC.

    """

    def __init__(self, hostname, port=CHAMELEON_PORT):
        """Constructs a ChameleonConnection.

        @param hostname: Hostname the chameleond process is running.
        @param port: Port number the chameleond process is listening on.

        @raise ChameleonConnectionError if connection failed.
        """
        self.chameleond_proxy = ChameleonConnection._create_server_proxy(
                hostname, port)


    @staticmethod
    def _create_server_proxy(hostname, port):
        """Creates the chameleond server proxy.

        @param hostname: Hostname the chameleond process is running.
        @param port: Port number the chameleond process is listening on.

        @return ServerProxy object to chameleond.

        @raise ChameleonConnectionError if connection failed.
        """
        remote = 'http://%s:%s' % (hostname, port)
        chameleond_proxy = xmlrpclib.ServerProxy(remote, allow_none=True)
        # Call a RPC to test.
        try:
            chameleond_proxy.GetSupportedPorts()
        except (socket.error,
                xmlrpclib.ProtocolError,
                httplib.BadStatusLine) as e:
            raise ChameleonConnectionError(e)
        return chameleond_proxy


class ChameleonBoard(object):
    """ChameleonBoard is an abstraction of a Chameleon board.

    A Chameleond RPC proxy is passed to the construction such that it can
    use this proxy to control the Chameleon board.

    User can use host to access utilities that are not provided by
    Chameleond XMLRPC server, e.g. send_file and get_file, which are provided by
    ssh_host.SSHHost, which is the base class of ChameleonHost.

    """

    def __init__(self, chameleon_connection, chameleon_host=None):
        """Construct a ChameleonBoard.

        @param chameleon_connection: ChameleonConnection object.
        @param chameleon_host: ChameleonHost object. None if this ChameleonBoard
                               is not created by a ChameleonHost.
        """
        self.host = chameleon_host
        self._chameleond_proxy = chameleon_connection.chameleond_proxy
        self._usb_ctrl = usb_controller.USBController(chameleon_connection)
        if self._chameleond_proxy.HasAudioBoard():
            self._audio_board = audio_board.AudioBoard(chameleon_connection)
        else:
            self._audio_board = None
            logging.info('There is no audio board on this Chameleon.')

    def reset(self):
        """Resets Chameleon board."""
        self._chameleond_proxy.Reset()


    def get_all_ports(self):
        """Gets all the ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbePorts()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_all_inputs(self):
        """Gets all the input ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbeInputs()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_all_outputs(self):
        """Gets all the output ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbeOutputs()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_label(self):
        """Gets the label which indicates the display connection.

        @return: A string of the label, like 'hdmi', 'dp_hdmi', etc.
        """
        connectors = []
        for port in self._chameleond_proxy.ProbeInputs():
            if self._chameleond_proxy.HasVideoSupport(port):
                connector = self._chameleond_proxy.GetConnectorType(port).lower()
                connectors.append(connector)
        # Eliminate duplicated ports. It simplifies the labels of dual-port
        # devices, i.e. dp_dp categorized into dp.
        return '_'.join(sorted(set(connectors)))


    def get_audio_board(self):
        """Gets the audio board on Chameleon.

        @return: An AudioBoard object.
        """
        return self._audio_board


    def get_usb_controller(self):
        """Gets the USB controller on Chameleon.

        @return: A USBController object.
        """
        return self._usb_ctrl


    def get_mac_address(self):
        """Gets the MAC address of Chameleon.

        @return: A string for MAC address.
        """
        return self._chameleond_proxy.GetMacAddress()


class ChameleonPort(object):
    """ChameleonPort is an abstraction of a general port of a Chameleon board.

    It only contains some common methods shared with audio and video ports.

    A Chameleond RPC proxy and an port_id are passed to the construction.
    The port_id is the unique identity to the port.
    """

    def __init__(self, chameleond_proxy, port_id):
        """Construct a ChameleonPort.

        @param chameleond_proxy: Chameleond RPC proxy object.
        @param port_id: The ID of the input port.
        """
        self.chameleond_proxy = chameleond_proxy
        self.port_id = port_id


    def get_connector_id(self):
        """Returns the connector ID.

        @return: A number of connector ID.
        """
        return self.port_id


    def get_connector_type(self):
        """Returns the human readable string for the connector type.

        @return: A string, like "VGA", "DVI", "HDMI", or "DP".
        """
        return self.chameleond_proxy.GetConnectorType(self.port_id)


    def has_audio_support(self):
        """Returns if the input has audio support.

        @return: True if the input has audio support; otherwise, False.
        """
        return self.chameleond_proxy.HasAudioSupport(self.port_id)


    def has_video_support(self):
        """Returns if the input has video support.

        @return: True if the input has video support; otherwise, False.
        """
        return self.chameleond_proxy.HasVideoSupport(self.port_id)


    def plug(self):
        """Asserts HPD line to high, emulating plug."""
        logging.info('Plug Chameleon port %d', self.port_id)
        self.chameleond_proxy.Plug(self.port_id)


    def unplug(self):
        """Deasserts HPD line to low, emulating unplug."""
        logging.info('Unplug Chameleon port %d', self.port_id)
        self.chameleond_proxy.Unplug(self.port_id)


    def set_plug(self, plug_status):
        """Sets plug/unplug by plug_status.

        @param plug_status: True to plug; False to unplug.
        """
        if plug_status:
            self.plug()
        else:
            self.unplug()


    @property
    def plugged(self):
        """
        @returns True if this port is plugged to Chameleon, False otherwise.

        """
        return self.chameleond_proxy.IsPlugged(self.port_id)


class ChameleonVideoInput(ChameleonPort):
    """ChameleonVideoInput is an abstraction of a video input port.

    It contains some special methods to control a video input.
    """

    _DUT_STABILIZE_TIME = 3
    _DURATION_UNPLUG_FOR_EDID = 5
    _TIMEOUT_VIDEO_STABLE_PROBE = 10
    _EDID_ID_DISABLE = -1

    def __init__(self, chameleon_port):
        """Construct a ChameleonVideoInput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id


    def wait_video_input_stable(self, timeout=None):
        """Waits the video input stable or timeout.

        @param timeout: The time period to wait for.

        @return: True if the video input becomes stable within the timeout
                 period; otherwise, False.
        """
        is_input_stable = self.chameleond_proxy.WaitVideoInputStable(
                                self.port_id, timeout)

        # If video input of Chameleon has been stable, wait for DUT software
        # layer to be stable as well to make sure all the configurations have
        # been propagated before proceeding.
        if is_input_stable:
            logging.info('Video input has been stable. Waiting for the DUT'
                         ' to be stable...')
            time.sleep(self._DUT_STABILIZE_TIME)
        return is_input_stable


    def read_edid(self):
        """Reads the EDID.

        @return: An Edid object or NO_EDID.
        """
        edid_binary = self.chameleond_proxy.ReadEdid(self.port_id)
        if edid_binary is None:
            return edid_lib.NO_EDID
        # Read EDID without verify. It may be made corrupted as intended
        # for the test purpose.
        return edid_lib.Edid(edid_binary.data, skip_verify=True)


    def apply_edid(self, edid):
        """Applies the given EDID.

        @param edid: An Edid object or NO_EDID.
        """
        if edid is edid_lib.NO_EDID:
          self.chameleond_proxy.ApplyEdid(self.port_id, self._EDID_ID_DISABLE)
        else:
          edid_binary = xmlrpclib.Binary(edid.data)
          edid_id = self.chameleond_proxy.CreateEdid(edid_binary)
          self.chameleond_proxy.ApplyEdid(self.port_id, edid_id)
          self.chameleond_proxy.DestroyEdid(edid_id)


    @contextmanager
    def use_edid(self, edid):
        """Uses the given EDID in a with statement.

        It sets the EDID up in the beginning and restores to the original
        EDID in the end. This function is expected to be used in a with
        statement, like the following:

            with chameleon_port.use_edid(edid):
                do_some_test_on(chameleon_port)

        @param edid: An EDID object.
        """
        # Set the EDID up in the beginning.
        plugged = self.plugged
        if plugged:
            self.unplug()

        original_edid = self.read_edid()
        logging.info('Apply EDID on port %d', self.port_id)
        self.apply_edid(edid)

        if plugged:
            time.sleep(self._DURATION_UNPLUG_FOR_EDID)
            self.plug()
            self.wait_video_input_stable(self._TIMEOUT_VIDEO_STABLE_PROBE)

        try:
            # Yeild to execute the with statement.
            yield
        finally:
            # Restore the original EDID in the end.
            current_edid = self.read_edid()
            if original_edid.data != current_edid.data:
                logging.info('Restore the original EDID.')
                self.apply_edid(original_edid)


    def use_edid_file(self, filename):
        """Uses the given EDID file in a with statement.

        It sets the EDID up in the beginning and restores to the original
        EDID in the end. This function is expected to be used in a with
        statement, like the following:

            with chameleon_port.use_edid_file(filename):
                do_some_test_on(chameleon_port)

        @param filename: A path to the EDID file.
        """
        return self.use_edid(edid_lib.Edid.from_file(filename))


    def fire_hpd_pulse(self, deassert_interval_usec, assert_interval_usec=None,
                       repeat_count=1, end_level=1):

        """Fires one or more HPD pulse (low -> high -> low -> ...).

        @param deassert_interval_usec: The time in microsecond of the
                deassert pulse.
        @param assert_interval_usec: The time in microsecond of the
                assert pulse. If None, then use the same value as
                deassert_interval_usec.
        @param repeat_count: The count of HPD pulses to fire.
        @param end_level: HPD ends with 0 for LOW (unplugged) or 1 for
                HIGH (plugged).
        """
        self.chameleond_proxy.FireHpdPulse(
                self.port_id, deassert_interval_usec,
                assert_interval_usec, repeat_count, int(bool(end_level)))


    def fire_mixed_hpd_pulses(self, widths):
        """Fires one or more HPD pulses, starting at low, of mixed widths.

        One must specify a list of segment widths in the widths argument where
        widths[0] is the width of the first low segment, widths[1] is that of
        the first high segment, widths[2] is that of the second low segment...
        etc. The HPD line stops at low if even number of segment widths are
        specified; otherwise, it stops at high.

        @param widths: list of pulse segment widths in usec.
        """
        self.chameleond_proxy.FireMixedHpdPulses(self.port_id, widths)


    def capture_screen(self):
        """Captures Chameleon framebuffer.

        @return An Image object.
        """
        return Image.fromstring(
                'RGB',
                self.get_resolution(),
                self.chameleond_proxy.DumpPixels(self.port_id).data)


    def get_resolution(self):
        """Gets the source resolution.

        @return: A (width, height) tuple.
        """
        # The return value of RPC is converted to a list. Convert it back to
        # a tuple.
        return tuple(self.chameleond_proxy.DetectResolution(self.port_id))


    def set_content_protection(self, enable):
        """Sets the content protection state on the port.

        @param enable: True to enable; False to disable.
        """
        self.chameleond_proxy.SetContentProtection(self.port_id, enable)


    def is_content_protection_enabled(self):
        """Returns True if the content protection is enabled on the port.

        @return: True if the content protection is enabled; otherwise, False.
        """
        return self.chameleond_proxy.IsContentProtectionEnabled(self.port_id)


    def is_video_input_encrypted(self):
        """Returns True if the video input on the port is encrypted.

        @return: True if the video input is encrypted; otherwise, False.
        """
        return self.chameleond_proxy.IsVideoInputEncrypted(self.port_id)


    def start_capturing_video(self, box=None):
        """
        Captures video frames. Asynchronous, returns immediately.

        @param box: int tuple, left, upper, right, lower pixel coordinates.
                    Defines the rectangular boundary within which to capture.
        """

        if box is None:
            self.chameleond_proxy.StartCapturingVideo(self.port_id)
        else:
            self.chameleond_proxy.StartCapturingVideo(self.port_id, *box)


    def stop_capturing_video(self):
        """
        Stops the ongoing video frame capturing.

        """
        self.chameleond_proxy.StopCapturingVideo(self.port_id)


    def get_captured_frame_count(self):
        """
        @return: int, the number of frames that have been captured.

        """
        return self.chameleond_proxy.GetCapturedFrameCount()


    def read_captured_frame(self, index):
        """
        @param index: int, index of the desired captured frame.
        @return: xmlrpclib.Binary object containing a byte-array of the pixels.

        """

        frame = self.chameleond_proxy.ReadCapturedFrame(index)
        return Image.fromstring('RGB',
                                self.get_captured_resolution(),
                                frame.data)


    def get_captured_checksums(self, start_index=0, stop_index=None):
        """
        @param start_index: int, index of the frame to start with.
        @param stop_index: int, index of the frame (excluded) to stop at.
        @return: a list of checksums of frames captured.

        """
        return self.chameleond_proxy.GetCapturedChecksums(start_index,
                                                          stop_index)


    def get_captured_resolution(self):
        """
        @return: (width, height) tuple, the resolution of captured frames.

        """
        return self.chameleond_proxy.GetCapturedResolution()



class ChameleonAudioInput(ChameleonPort):
    """ChameleonAudioInput is an abstraction of an audio input port.

    It contains some special methods to control an audio input.
    """

    def __init__(self, chameleon_port):
        """Construct a ChameleonAudioInput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id


    def start_capturing_audio(self):
        """Starts capturing audio."""
        return self.chameleond_proxy.StartCapturingAudio(self.port_id)


    def stop_capturing_audio(self):
        """Stops capturing audio.

        Returns:
          A tuple (remote_path, format).
          remote_path: The captured file path on Chameleon.
          format: A dict containing:
            file_type: 'raw' or 'wav'.
            sample_format: 'S32_LE' for 32-bit signed integer in little-endian.
              Refer to aplay manpage for other formats.
            channel: channel number.
            rate: sampling rate.
        """
        remote_path, data_format = self.chameleond_proxy.StopCapturingAudio(
                self.port_id)
        return remote_path, data_format


class ChameleonAudioOutput(ChameleonPort):
    """ChameleonAudioOutput is an abstraction of an audio output port.

    It contains some special methods to control an audio output.
    """

    def __init__(self, chameleon_port):
        """Construct a ChameleonAudioOutput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id


    def start_playing_audio(self, path, data_format):
        """Starts playing audio.

        @param path: The path to the file to play on Chameleon.
        @param data_format: A dict containing data format. Currently Chameleon
                            only accepts data format:
                            dict(file_type='raw', sample_format='S32_LE',
                                 channel=8, rate=48000).

        """
        self.chameleond_proxy.StartPlayingAudio(self.port_id, path, data_format)


    def stop_playing_audio(self):
        """Stops capturing audio."""
        self.chameleond_proxy.StopPlayingAudio(self.port_id)


def make_chameleon_hostname(dut_hostname):
    """Given a DUT's hostname, returns the hostname of its Chameleon.

    @param dut_hostname: Hostname of a DUT.

    @return Hostname of the DUT's Chameleon.
    """
    host_parts = dut_hostname.split('.')
    host_parts[0] = host_parts[0] + '-chameleon'
    return '.'.join(host_parts)


def create_chameleon_board(dut_hostname, args):
    """Given either DUT's hostname or argments, creates a ChameleonBoard object.

    If the DUT's hostname is in the lab zone, it connects to the Chameleon by
    append the hostname with '-chameleon' suffix. If not, checks if the args
    contains the key-value pair 'chameleon_host=IP'.

    @param dut_hostname: Hostname of a DUT.
    @param args: A string of arguments passed from the command line.

    @return A ChameleonBoard object.

    @raise ChameleonConnectionError if unknown hostname.
    """
    connection = None
    hostname = make_chameleon_hostname(dut_hostname)
    if utils.host_is_in_lab_zone(hostname):
        connection = ChameleonConnection(hostname)
    else:
        args_dict = utils.args_to_dict(args)
        hostname = args_dict.get('chameleon_host', None)
        port = args_dict.get('chameleon_port', CHAMELEON_PORT)
        if hostname:
            connection = ChameleonConnection(hostname, port)
        else:
            raise ChameleonConnectionError('No chameleon_host is given in args')

    return ChameleonBoard(connection)
