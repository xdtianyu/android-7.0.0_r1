# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This module provides the information of Chameleon board."""


import collections
import logging


# Mapping from Chameleon MAC address to other information including
# bluetooth MAC address on audio board.
ChameleonInfo = collections.namedtuple(
        'ChameleonInfo', ['bluetooth_mac_address'])

_CHAMELEON_BOARD_INFO = {
        '94:eb:2c:00:00:fb': ChameleonInfo('00:1F:84:01:03:68'),
        '94:eb:2c:00:00:f9': ChameleonInfo('00:1F:84:01:03:73'),
        '94:eb:2c:00:01:25': ChameleonInfo('00:1F:84:01:03:4F'),
        '94:eb:2c:00:01:27': ChameleonInfo('00:1F:84:01:03:5B'),
        '94:eb:2c:00:01:28': ChameleonInfo('00:1F:84:01:03:46'),
        '94:eb:2c:00:01:29': ChameleonInfo('00:1F:84:01:03:26'),
        '94:eb:2c:00:01:2b': ChameleonInfo('00:1F:84:01:03:5E'),
        '94:eb:2c:00:01:2d': ChameleonInfo('00:1F:84:01:03:B6'),
        '94:eb:2c:00:01:30': ChameleonInfo('00:1F:84:01:03:2F'),
        '94:eb:2c:00:01:3a': ChameleonInfo('00:1F:84:01:03:42'),
        '94:eb:2c:00:01:3b': ChameleonInfo('00:1F:84:01:03:44'),
        '94:eb:2c:00:01:3c': ChameleonInfo('00:1F:84:01:03:62'),
        '94:eb:2c:00:01:3d': ChameleonInfo('00:1F:84:01:03:59'),
        '94:eb:2c:00:01:3e': ChameleonInfo('00:1F:84:01:03:74'),
        '94:eb:2c:00:01:3f': ChameleonInfo('00:1F:84:01:03:8C'),
        '94:eb:2c:00:01:41': ChameleonInfo('00:1F:84:01:03:B3'),
        '94:eb:2c:10:06:65': ChameleonInfo('00:1F:84:01:03:6A'),
        '94:eb:2c:10:06:66': ChameleonInfo('00:1F:84:01:03:21'),
        '94:eb:2c:10:06:67': ChameleonInfo('00:1F:84:01:03:38'),
        '94:eb:2c:10:06:68': ChameleonInfo('00:1F:84:01:03:52'),
        '94:eb:2c:10:06:6c': ChameleonInfo('00:1F:84:01:03:2E'),
        '94:eb:2c:10:06:6d': ChameleonInfo('00:1F:84:01:03:84'),
        '94:eb:2c:10:06:6e': ChameleonInfo('00:1F:84:01:03:98'),
        '94:eb:2c:10:06:72': ChameleonInfo('00:1F:84:01:03:61'),
}

class ChameleonInfoError(Exception):
    """Error in chameleon_info."""
    pass


def get_bluetooth_mac_address(chameleon_board):
    """Gets bluetooth MAC address of a ChameleonBoard.

    @param chameleon_board: A ChameleonBoard object.

    @returns: A string for bluetooth MAC address of bluetooth module on the
              audio board.

    @raises: ChameleonInfoError if bluetooth MAC address of this Chameleon
             board can not be found.

    """
    chameleon_mac_address = chameleon_board.get_mac_address().lower()
    if chameleon_mac_address not in _CHAMELEON_BOARD_INFO:
        raise ChameleonInfoError(
                'Chameleon info not found for %s' % chameleon_mac_address)
    board_info = _CHAMELEON_BOARD_INFO[chameleon_mac_address]
    logging.debug('Chameleon board info: %r', board_info)
    return board_info.bluetooth_mac_address
