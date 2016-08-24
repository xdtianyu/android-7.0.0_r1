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
Class for Telnet control of Aeroflex 832X and 833X Series Attenuator Modules

This class provides a wrapper to the Aeroflex attenuator modules for purposes
of simplifying and abstracting control down to the basic necessities. It is
not the intention of the module to expose all functionality, but to allow
interchangeable HW to be used.

See http://www.aeroflex.com/ams/weinschel/PDFILES/IM-608-Models-8320-&-8321-preliminary.pdf
"""


from acts.controllers import attenuator
from acts.controllers.attenuator_lib import _tnhelper


class AttenuatorInstrument(attenuator.AttenuatorInstrument):

    def __init__(self, num_atten=0):
        super().__init__(num_atten)

        self._tnhelper = _tnhelper._TNHelper(tx_cmd_separator="\r\n",
                                             rx_cmd_separator="\r\n",
                                             prompt=">")
        self.properties = None

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

        # work around a bug in IO, but this is a good thing to do anyway
        self._tnhelper.cmd("*CLS", False)

        if self.num_atten == 0:
            self.num_atten = int(self._tnhelper.cmd("RFCONFIG? CHAN"))

        configstr = self._tnhelper.cmd("RFCONFIG? ATTN 1")

        self.properties = dict(zip(['model', 'max_atten', 'min_step',
                                    'unknown', 'unknown2', 'cfg_str'],
                                   configstr.split(", ", 5)))

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

        self._tnhelper.cmd("ATTN " + str(idx+1) + " " + str(value), False)

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

        atten_val = self._tnhelper.cmd("ATTN? " + str(idx+1))

        return float(atten_val)
