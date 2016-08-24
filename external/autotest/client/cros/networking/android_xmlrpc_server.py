#!/usr/bin/python3.4

# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import contextlib
import errno
import logging
import queue
import select
import signal
import threading
import time

from xmlrpc.server import SimpleXMLRPCServer

from acts.utils import get_current_epoch_time
import acts.controllers.android_device as android_device
import acts.test_utils.wifi.wifi_test_utils as utils


class Map(dict):
    """A convenience class that makes dictionary values accessible via dot
    operator.

    Example:
        >> m = Map({"SSID": "GoogleGuest"})
        >> m.SSID
        GoogleGuest
    """
    def __init__(self, *args, **kwargs):
        super(Map, self).__init__(*args, **kwargs)
        for arg in args:
            if isinstance(arg, dict):
                for k, v in arg.items():
                    self[k] = v
        if kwargs:
            for k, v in kwargs.items():
                self[k] = v


    def __getattr__(self, attr):
        return self.get(attr)


    def __setattr__(self, key, value):
        self.__setitem__(key, value)


# This is copied over from client/cros/xmlrpc_server.py so that this
# daemon has no autotest dependencies.
class XmlRpcServer(threading.Thread):
    """Simple XMLRPC server implementation.

    In theory, Python should provide a sane XMLRPC server implementation as
    part of its standard library.  In practice the provided implementation
    doesn't handle signals, not even EINTR.  As a result, we have this class.

    Usage:

    server = XmlRpcServer(('localhost', 43212))
    server.register_delegate(my_delegate_instance)
    server.run()

    """

    def __init__(self, host, port):
        """Construct an XmlRpcServer.

        @param host string hostname to bind to.
        @param port int port number to bind to.

        """
        super(XmlRpcServer, self).__init__()
        logging.info('Binding server to %s:%d', host, port)
        self._server = SimpleXMLRPCServer((host, port), allow_none=True)
        self._server.register_introspection_functions()
        self._keep_running = True
        self._delegates = []
        # Gracefully shut down on signals.  This is how we expect to be shut
        # down by autotest.
        signal.signal(signal.SIGTERM, self._handle_signal)
        signal.signal(signal.SIGINT, self._handle_signal)


    def register_delegate(self, delegate):
        """Register delegate objects with the server.

        The server will automagically look up all methods not prefixed with an
        underscore and treat them as potential RPC calls.  These methods may
        only take basic Python objects as parameters, as noted by the
        SimpleXMLRPCServer documentation.  The state of the delegate is
        persisted across calls.

        @param delegate object Python object to be exposed via RPC.

        """
        self._server.register_instance(delegate)
        self._delegates.append(delegate)


    def run(self):
        """Block and handle many XmlRpc requests."""
        logging.info('XmlRpcServer starting...')
        with contextlib.ExitStack() as stack:
            for delegate in self._delegates:
                stack.enter_context(delegate)
            while self._keep_running:
                try:
                    self._server.handle_request()
                except select.error as v:
                    # In a cruel twist of fate, the python library doesn't
                    # handle this kind of error.
                    if v[0] != errno.EINTR:
                        raise
                except Exception as e:
                    logging.error("Error in handle request: %s" % str(e))
        logging.info('XmlRpcServer exited.')


    def _handle_signal(self, _signum, _frame):
        """Handle a process signal by gracefully quitting.

        SimpleXMLRPCServer helpfully exposes a method called shutdown() which
        clears a flag similar to _keep_running, and then blocks until it sees
        the server shut down.  Unfortunately, if you call that function from
        a signal handler, the server will just hang, since the process is
        paused for the signal, causing a deadlock.  Thus we are reinventing the
        wheel with our own event loop.

        """
        self._server.server_close()
        self._keep_running = False


class XmlRpcServerError(Exception):
    """Raised when an error is encountered in the XmlRpcServer."""


class AndroidXmlRpcDelegate(object):
    """Exposes methods called remotely during WiFi autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XMLRPC server.
    """

    WEP40_HEX_KEY_LEN = 10
    WEP104_HEX_KEY_LEN = 26
    SHILL_DISCONNECTED_STATES = ['idle']
    SHILL_CONNECTED_STATES =  ['portal', 'online', 'ready']
    DISCONNECTED_SSID = '0x'
    DISCOVERY_POLLING_INTERVAL = 1


    def __init__(self, serial_number):
        """Initializes the ACTS library components.

        @param serial_number Serial number of the android device to be tested,
               None if there is only one device connected to the host.

        """
        if not serial_number:
            ads = android_device.get_all_instances()
            if not ads:
                msg = "No android device found, abort!"
                logging.error(msg)
                raise XmlRpcServerError(msg)
            self.ad = ads[0]
        elif serial_number in android_device.list_adb_devices():
            self.ad = android_device.AndroidDevice(serial_number)
        else:
            msg = ("Specified Android device %s can't be found, abort!"
                   ) % serial_number
            logging.error(msg)
            raise XmlRpcServerError(msg)


    def __enter__(self):
        logging.debug('Bringing up AndroidXmlRpcDelegate.')
        self.ad.get_droid()
        self.ad.ed.start()
        return self


    def __exit__(self, exception, value, traceback):
        logging.debug('Tearing down AndroidXmlRpcDelegate.')
        self.ad.terminate_all_sessions()


    # Commands start.
    def ready(self):
        """Confirm that the XMLRPC server is up and ready to serve.

        @return True (always).

        """
        logging.debug('ready()')
        return True


    def list_controlled_wifi_interfaces(self):
        return ['wlan0']


    def set_device_enabled(self, wifi_interface, enabled):
        """Enable or disable the WiFi device.

        @param wifi_interface: string name of interface being modified.
        @param enabled: boolean; true if this device should be enabled,
                false if this device should be disabled.
        @return True if it worked; false, otherwise

        """
        return utils.wifi_toggle_state(self.ad, enabled)


    def sync_time_to(self, epoch_seconds):
        """Sync time on the DUT to |epoch_seconds| from the epoch.

        @param epoch_seconds: float number of seconds from the epoch.

        """
        self.ad.droid.setTime(epoch_seconds)
        return True


    def clean_profiles(self):
        return True


    def create_profile(self, profile_name):
        return True


    def push_profile(self, profile_name):
        return True


    def remove_profile(self, profile_name):
        return True


    def pop_profile(self, profile_name):
        return True


    def disconnect(self, ssid):
        """Attempt to disconnect from the given ssid.

        Blocks until disconnected or operation has timed out.  Returns True iff
        disconnect was successful.

        @param ssid string network to disconnect from.
        @return bool True on success, False otherwise.

        """
        # Android had no explicit disconnect, so let's just forget the network.
        return self.delete_entries_for_ssid(ssid)


    def get_active_wifi_SSIDs(self):
        """Get the list of all SSIDs in the current scan results.

        @return list of string SSIDs with at least one BSS we've scanned.

        """
        ssids = []
        try:
            self.ad.droid.wifiStartScan()
            self.ad.ed.pop_event('WifiManagerScanResultsAvailable')
            scan_results = self.ad.droid.wifiGetScanResults()
            for result in scan_results:
                if utils.WifiEnums.SSID_KEY in result:
                    ssids.append(result[utils.WifiEnums.SSID_KEY])
        except queue.Empty:
            logging.error("Scan results available event timed out!")
        except Exception as e:
            logging.error("Scan results error: %s" % str(e))
        finally:
            logging.debug(ssids)
            return ssids


    def wait_for_service_states(self, ssid, states, timeout_seconds):
        """Wait for SSID to reach one state out of a list of states.

        @param ssid string the network to connect to (e.g. 'GoogleGuest').
        @param states tuple the states for which to wait
        @param timeout_seconds int seconds to wait for a state

        @return (result, final_state, wait_time) tuple of the result for the
                wait.
        """
        current_con = self.ad.droid.wifiGetConnectionInfo()
        # Check the current state to see if we're connected/disconnected.
        if set(states).intersection(set(self.SHILL_CONNECTED_STATES)):
            if current_con[utils.WifiEnums.SSID_KEY] == ssid:
                return True, '', 0
            wait_event = 'WifiNetworkConnected'
        elif set(states).intersection(set(self.SHILL_DISCONNECTED_STATES)):
            if current_con[utils.WifiEnums.SSID_KEY] == self.DISCONNECTED_SSID:
                return True, '', 0
            wait_event = 'WifiNetworkDisconnected'
        else:
            assert 0, "Unhandled wait states received: %r" % states
        final_state = ""
        wait_time = -1
        result = False
        logging.debug(current_con)
        try:
            self.ad.droid.wifiStartTrackingStateChange()
            start_time = get_current_epoch_time()
            wait_result = self.ad.ed.pop_event(wait_event, timeout_seconds)
            end_time = get_current_epoch_time()
            wait_time = (end_time - start_time) / 1000
            if wait_event == 'WifiNetworkConnected':
                actual_ssid = wait_result['data'][utils.WifiEnums.SSID_KEY]
                assert actual_ssid == ssid, ("Expected to connect to %s, but "
                        "connected to %s") % (ssid, actual_ssid)
            result = True
        except queue.Empty:
            logging.error("No state change available yet!")
        except Exception as e:
            logging.error("State change error: %s" % str(e))
        finally:
            logging.debug((result, final_state, wait_time))
            self.ad.droid.wifiStopTrackingStateChange()
            return result, final_state, wait_time


    def delete_entries_for_ssid(self, ssid):
        """Delete all saved entries for an SSID.

        @param ssid string of SSID for which to delete entries.
        @return True on success, False otherwise.

        """
        try:
            utils.wifi_forget_network(self.ad, ssid)
        except Exception as e:
            logging.error(str(e))
            return False
        return True


    def connect_wifi(self, raw_params):
        """Block and attempt to connect to wifi network.

        @param raw_params serialized AssociationParameters.
        @return serialized AssociationResult

        """
        # Prepare data objects.
        params = Map(raw_params)
        params.security_config = Map(raw_params['security_config'])
        params.bgscan_config = Map(raw_params['bgscan_config'])
        logging.debug('connect_wifi(). Params: %r' % params)
        network_config = {
            "SSID": params.ssid,
            "hiddenSSID":  True if params.is_hidden else False
        }
        assoc_result = {
            "discovery_time" : 0,
            "association_time" : 0,
            "configuration_time" : 0,
            "failure_reason" : "Oops!",
            "xmlrpc_struct_type_key" : "AssociationResult"
        }
        duration = lambda: (get_current_epoch_time() - start_time) / 1000
        try:
            # Verify that the network was found, if the SSID is not hidden.
            if not params.is_hidden:
                start_time = get_current_epoch_time()
                found = False
                while duration() < params.discovery_timeout and not found:
                    active_ssids = self.get_active_wifi_SSIDs()
                    found = params.ssid in active_ssids
                    if not found:
                        time.sleep(self.DISCOVERY_POLLING_INTERVAL)
                assoc_result["discovery_time"] = duration()
                assert found, ("Could not find %s in scan results: %r") % (
                        params.ssid, active_ssids)
            result = False
            if params.security_config.security == "psk":
                network_config["password"] = params.security_config.psk
            elif params.security_config.security == "wep":
                network_config["wepTxKeyIndex"] = params.security_config.wep_default_key
                # Convert all ASCII keys to Hex
                wep_hex_keys = []
                for key in params.security_config.wep_keys:
                    if len(key) == self.WEP40_HEX_KEY_LEN or \
                       len(key) == self.WEP104_HEX_KEY_LEN:
                        wep_hex_keys.append(key)
                    else:
                        hex_key = ""
                        for byte in bytes(key, 'utf-8'):
                            hex_key += '%x' % byte
                        wep_hex_keys.append(hex_key)
                network_config["wepKeys"] = wep_hex_keys
            # Associate to the network.
            self.ad.droid.wifiStartTrackingStateChange()
            start_time = get_current_epoch_time()
            result = self.ad.droid.wifiConnect(network_config)
            assert result, "wifiConnect call failed."
            # Verify connection successful and correct.
            logging.debug('wifiConnect result: %s. Waiting for connection' % result);
            timeout = params.association_timeout + params.configuration_timeout
            connect_result = self.ad.ed.pop_event(
                utils.WifiEventNames.WIFI_CONNECTED, timeout)
            assoc_result["association_time"] = duration()
            actual_ssid = connect_result['data'][utils.WifiEnums.SSID_KEY]
            logging.debug('Connected to SSID: %s' % params.ssid);
            assert actual_ssid == params.ssid, ("Expected to connect to %s, "
                "connected to %s") % (params.ssid, actual_ssid)
            result = True
        except queue.Empty:
            msg = "Failed to connect to %s with %s" % (params.ssid,
                params.security_config.security)
            logging.error(msg)
            assoc_result["failure_reason"] = msg
            result = False
        except Exception as e:
            msg = str(e)
            logging.error(msg)
            assoc_result["failure_reason"] = msg
            result = False
        finally:
            assoc_result["success"] = result
            logging.debug(assoc_result)
            self.ad.droid.wifiStopTrackingStateChange()
            return assoc_result


    def init_test_network_state(self):
        """Create a clean slate for tests with respect to remembered networks.

        @return True iff operation succeeded, False otherwise.
        """
        try:
            utils.wifi_test_device_init(self.ad)
        except AssertionError as e:
            logging.error(str(e))
            return False
        return True


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Cros Wifi Xml RPC server.')
    parser.add_argument('-s', '--serial-number', action='store', default=None,
                         help='Serial Number of the device to test.')
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG)
    logging.debug("android_xmlrpc_server main...")
    server = XmlRpcServer('localhost', 9989)
    server.register_delegate(AndroidXmlRpcDelegate(args.serial_number))
    server.run()
