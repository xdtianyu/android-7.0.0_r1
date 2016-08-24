# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the utilities for bluetooth audio using chameleon."""

import logging
import time

from autotest_lib.client.bin import utils


_PIN = '0000'
_SEARCH_TIMEOUT = 30.0
_PAIRING_TIMEOUT = 5.0
_CONNECT_TIMEOUT = 15.0


class ChameleonBluetoothAudioError(Exception):
    """Error in this module."""
    pass


def connect_bluetooth_module_full_flow(bt_adapter, target_mac_address,
                             timeout=_SEARCH_TIMEOUT):
    """Controls Cros device to connect to bluetooth module on audio board.

    @param bt_adapter: A BluetoothDevice object to control bluetooth adapter
                       on Cros device.
    @param target_mac_address: The MAC address of bluetooth module to be
                               connected.
    @param timeout: Timeout in seconds to search for bluetooth module.

    @raises: ChameleonBluetoothAudioError if Cros device fails to connect to
             bluetooth module on audio board.

    """
    # Resets bluetooth adapter on Cros device.
    if not bt_adapter.reset_on():
        raise ChameleonBluetoothAudioError(
                'Failed to reset bluetooth adapter on Cros host.'
                ' You should check if controller is available on Cros host'
                ' using bluetoothctl.')

    # Starts discovery mode of bluetooth adapter.
    if not bt_adapter.start_discovery():
        raise ChameleonBluetoothAudioError(
                'Failed to start discovery on bluetooth adapter on Cros host')

    def _find_device():
        """Controls bluetooth adapter to search for bluetooth module.

        @returns: True if there is a bluetooth device with MAC address
                  matches target_mac_address. False otherwise.

        """
        return bt_adapter.has_device(target_mac_address)

    # Searches for bluetooth module with given MAC address.
    found_device = utils.wait_for_value(_find_device, True, timeout_sec=timeout)

    if not found_device:
        raise ChameleonBluetoothAudioError(
                'Can not find bluetooth module with MAC address %s' %
                target_mac_address)

    pair_legacy_bluetooth_module(bt_adapter, target_mac_address)

    # Disconnects from bluetooth module to clean up the state.
    if not bt_adapter.disconnect_device(target_mac_address):
        raise ChameleonBluetoothAudioError(
                'Failed to let Cros device disconnect from bluetooth module %s' %
                target_mac_address)

    # Connects to bluetooth module.
    connect_bluetooth_module(bt_adapter, target_mac_address)

    logging.info('Bluetooth module at %s is connected', target_mac_address)


def connect_bluetooth_module(bt_adapter, target_mac_address,
                             timeout=_CONNECT_TIMEOUT):
    """Controls Cros device to connect to bluetooth module on audio board.

    @param bt_adapter: A BluetoothDevice object to control bluetooth adapter
                       on Cros device.
    @param target_mac_address: The MAC address of bluetooth module to be
                               connected.
    @param timeout: Timeout in seconds to connect bluetooth module.

    @raises: ChameleonBluetoothAudioError if Cros device fails to connect to
             bluetooth module on audio board.

    """
    def _connect_device():
        success = bt_adapter.connect_device(target_mac_address)
        if not success:
            logging.debug('Can not connect device, retry in 1 second.')
            time.sleep(1)
            return False
        logging.debug('Connection established.')
        return True

    # Connects bluetooth module with given MAC address.
    connected = utils.wait_for_value(_connect_device, True, timeout_sec=timeout)
    if not connected:
        raise ChameleonBluetoothAudioError(
                'Failed to let Cros device connect to bluetooth module %s' %
                target_mac_address)


def pair_legacy_bluetooth_module(bt_adapter, target_mac_address, pin=_PIN,
                                 pairing_timeout=_PAIRING_TIMEOUT, retries=3):
    """Pairs Cros device bluetooth adapter with legacy bluetooth module.

    @param bt_adapter: A BluetoothDevice object to control bluetooth adapter
                       on Cros device.
    @param target_mac_address: The MAC address of bluetooth module to be
                               paired.
    @param pin: The pin for legacy pairing.
    @param timeout: Timeout in seconds to pair bluetooth module in a trial.
    @param retries: Number of retries if pairing fails.

    @raises: ChameleonBluetoothAudioError if Cros device fails to pair
             bluetooth module on audio board after all the retries.

    """
    # Pairs the bluetooth adapter with bluetooth module.
    for trial in xrange(retries):
        if bt_adapter.pair_legacy_device(
            target_mac_address, pin, pairing_timeout):
                logging.debug('Pairing to %s succeeded', target_mac_address)
                return
        elif trial == retries - 1:
            raise ChameleonBluetoothAudioError(
                    'Failed to pair Cros device and bluetooth module %s' %
                    target_mac_address)

        logging.debug('Retry for pairing...')
