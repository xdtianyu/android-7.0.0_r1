# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import operator
import os


# TODO: This is a quick workaround; some of our arm devices so far only
# support the HDMI EDIDs and the DP one at 1680x1050. A more proper
# solution is to build a database of supported resolutions and pixel
# clocks for each model and check if the EDID is in the supported list.
def is_edid_supported(host, interface, width, height):
    """Check whether the EDID is supported by DUT

    @param host: A CrosHost object.
    @param interface: The display interface, like 'HDMI'.
    @param width: The screen width
    @param height: The screen height

    @return: True if the check passes; False otherwise.
    """
    # TODO: Support client test that the host is not a CrosHost.
    platform = host.get_platform()
    prefix = platform.lower().split('_')[0]
    if prefix in ('snow', 'spring', 'skate', 'peach', 'veyron'):
        if interface == 'DP':
            return width == 1680 and height == 1050
    return True


class Edid(object):
    """Edid is an abstraction of EDID (Extended Display Identification Data).

    It provides methods to get the properties, manipulate the structure,
    import from a file, export to a file, etc.

    """

    BLOCK_SIZE = 128


    def __init__(self, data, skip_verify=False):
        """Construct an Edid.

        @param data: A byte-array of EDID data.
        @param skip_verify: True to skip the correctness check.
        """
        if not Edid.verify(data) and not skip_verify:
            raise ValueError('Not a valid EDID.')
        self.data = data


    @staticmethod
    def verify(data):
        """Verify the correctness of EDID.

        @param data: A byte-array of EDID data.

        @return True if the EDID is correct; False otherwise.
        """
        data_len = len(data)
        if data_len % Edid.BLOCK_SIZE != 0:
            logging.debug('EDID has an invalid length: %d', data_len)
            return False

        for start in xrange(0, data_len, Edid.BLOCK_SIZE):
            # Each block (128-byte) has a checksum at the last byte.
            checksum = reduce(operator.add,
                              map(ord, data[start:start+Edid.BLOCK_SIZE]))
            if checksum % 256 != 0:
                logging.debug('Wrong checksum in the block %d of EDID',
                              start / Edid.BLOCK_SIZE)
                return False

        return True


    @classmethod
    def from_file(cls, filename, skip_verify=False):
        """Construct an Edid from a file.

        @param filename: A string of filename.
        @param skip_verify: True to skip the correctness check.
        """
        if not os.path.exists(filename):
            raise ValueError('EDID file %r does not exist' % filename)

        if filename.upper().endswith('.TXT'):
            # Convert the EDID text format, returning from xrandr.
            data = reduce(operator.add,
                          map(lambda s: s.strip().decode('hex'),
                              open(filename).readlines()))
        else:
            data = open(filename).read()
        return cls(data, skip_verify)


    def to_file(self, filename):
        """Export the EDID to a file.

        @param filename: A string of filename.
        """
        with open(filename, 'w+') as f:
            f.write(self.data)


# A constant object to represent no EDID.
NO_EDID = Edid('', skip_verify=True)
