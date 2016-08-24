#!/usr/bin/env python3.4

#   Copyright 2016- The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

"""
Class for Telnet control of Mini-Circuits RCDAT series attenuators

This class provides a wrapper to the MC-RCDAT attenuator modules for purposes
of simplifying and abstracting control down to the basic necessities. It is
not the intention of the module to expose all functionality, but to allow
interchangeable HW to be used.

See http://www.minicircuits.com/softwaredownload/Prog_Manual-6-Programmable_Attenuator.pdf
"""


from acts.controllers import attenuator
from acts.controllers.attenuator_lib import _tnhelper


class AttenuatorInstrument(attenuator.AttenuatorInstrument):
    r"""This provides a specific telnet-controlled implementation of AttenuatorInstrument for
    Mini-Circuits RC-DAT attenuators.

    With the exception of telnet-specific commands, all functionality is defined by the
    AttenuatorInstrument class. Because telnet is a stateful protocol, the functionality of
    AttenuatorInstrument is contingent upon a telnet connection being established.
    """

    def __init__(self, num_atten=0):
        super().__init__(num_atten)
        self._tnhelper = _tnhelper._TNHelper(tx_cmd_separator="\r\n",
                                             rx_cmd_separator="\n\r",
                                             prompt="")

    def __del__(self):
        if self.is_open():
            self.close()

    def open(self, host, port=23):
        r"""Opens a telnet connection to the desired AttenuatorInstrument and queries basic
        information.

        Parameters
        ----------
        host : A valid hostname (IP address or DNS-resolvable name) to an MC-DAT attenuator
        instrument.
        port : An optional port number (defaults to telnet default 23)
        """

        self._tnhelper.open(host, port)

        if self.num_atten == 0:
            self.num_atten = 1

        config_str = self._tnhelper.cmd("MN?")

        if config_str.startswith("MN="):
            config_str = config_str[len("MN="):]

        self.properties = dict(zip(['model', 'max_freq', 'max_atten'], config_str.split("-", 2)))
        self.max_atten = float(self.properties['max_atten'])

    def is_open(self):
        r"""This function returns the state of the telnet connection to the underlying
        AttenuatorInstrument.

        Returns
        -------
        Bool
            True if there is a successfully open connection to the AttenuatorInstrument
        """

        return bool(self._tnhelper.is_open())

    def close(self):
        r"""Closes a telnet connection to the desired AttenuatorInstrument.

        This should be called as part of any teardown procedure prior to the attenuator
        instrument leaving scope.
        """

        self._tnhelper.close()

    def set_atten(self, idx, value):
        r"""This function sets the attenuation of an attenuator given its index in the instrument.

        Parameters
        ----------
        idx : This zero-based index is the identifier for a particular attenuator in an
        instrument.
        value : This is a floating point value for nominal attenuation to be set.

        Raises
        ------
        InvalidOperationError
            This error occurs if the underlying telnet connection to the instrument is not open.
        IndexError
            If the index of the attenuator is greater than the maximum index of the underlying
            instrument, this error will be thrown. Do not count on this check programmatically.
        ValueError
            If the requested set value is greater than the maximum attenuation value, this error
            will be thrown. Do not count on this check programmatically.
        """

        if not self.is_open():
            raise attenuator.InvalidOperationError("Connection not open!")

        if idx >= self.num_atten:
            raise IndexError("Attenuator index out of range!", self.num_atten, idx)

        if value > self.max_atten:
            raise ValueError("Attenuator value out of range!", self.max_atten, value)

        self._tnhelper.cmd("SETATT=" + str(value))

    def get_atten(self, idx):
        r"""This function returns the current attenuation from an attenuator at a given index in
        the instrument.

        Parameters
        ----------
        idx : This zero-based index is the identifier for a particular attenuator in an instrument.

        Raises
        ------
        InvalidOperationError
            This error occurs if the underlying telnet connection to the instrument is not open.

        Returns
        -------
        float
            Returns a the current attenuation value
        """

        if not self.is_open():
            raise attenuator.InvalidOperationError("Connection not open!")

#       Potentially redundant safety check removed for the moment
#       if idx >= self.num_atten:
#           raise IndexError("Attenuator index out of range!", self.num_atten, idx)

        atten_val = self._tnhelper.cmd("ATT?")

        return float(atten_val)
