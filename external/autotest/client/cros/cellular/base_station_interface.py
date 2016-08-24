# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base class interface for base station emulators."""

# This is essentially all documentation; no code (other than raise
# NotImplementedError()) should go here."""

import air_state_verifier
import cellular


class BaseStationInterface(object):
    """A generic base station emulator."""
    def Start(self):
        raise NotImplementedError()

    def Stop(self):
        raise NotImplementedError()

    def GetAirStateVerifier(self):
        return air_state_verifier.AirStateVerifierPermissive(self)

    def SetBsIpV4(self, ip1, ip2):
        """Sets base station IPv4 addresses."""
        raise NotImplementedError()

    def SetBsNetmaskV4(self, netmask):
        """Sets base station netmask."""
        raise NotImplementedError()

    def SetFrequencyBand(self, band):
        """Sets the frequency used by the BS.  BS must be stopped.

        Arguments:
            band: A band number, from the UMTS bands summarized at
                  http://en.wikipedia.org/wiki/UMTS_frequency_bands
                  Use band 5 for 800MHz C2k/EV-DO,  2 for 1900MHz C2k/EV-DO
        """
        raise NotImplementedError()

    def SetPlmn(self, mcc, mnc):
        """Sets the mobile country and network codes.  BS must be stopped."""
        raise NotImplementedError()

    def SetPower(self, dbm):
        """Sets the output power of the base station.

        Arguments:
            dbm: Power, in dBm.  See class Power for useful constants.
        """
        raise NotImplementedError()

    def SetUeDnsV4(self, dns1, dns2):
        """Set the DNS values provided to the UE.  Emulator must be stopped.
        """
        raise NotImplementedError()

    def SetUeIpV4(self, ip1, ip2=None):
        """Set the IP addresses provided to the UE.  Emulator must be stopped.

        Arguments:
            ip1: IP address, as a dotted-quad string.
            ip2: Secondary IP address.  Not set if not supplied.
        """
        raise NotImplementedError()

    def GetUeDataStatus(self):
        """Gets the data call status of the UE."""
        raise NotImplementedError()

    def PrepareForStatusChange(self):
        """Prepare for a future call to WaitForStatusChange.

        There's a race in WaitForStatusChange; if we tell the modem to
        connect, it might connect before we get around to calling
        PrepareForStatusChange.

        As a special case for 8960, this tells the instrument to make the
        next GetUeStatus call block on a status change.
        """
        raise NotImplementedError()

    def WaitForStatusChange(self,
                            interested=None,
                            timeout=cellular.DEFAULT_TIMEOUT):
        """When UE status changes (to a value in interested), return the value.

        Arguments:
            interested: if non-None, only transitions to these states will
                        cause a return
            timeout: in seconds.
        """
        raise NotImplementedError()

    def WaitForSmsReceive(self,
                          timeout=cellular.DEFAULT_TIMEOUT):
        """Return received SMS is received from the UE.

        Arguments:
            timeout: in seconds.
        """
        raise NotImplementedError()

    def SendSms(self,
                message,
                o_address=cellular.SmsAddress('8960'),
                dcs=0xf0):
        """Sends the supplied SMS message."""
        raise NotImplementedError()

