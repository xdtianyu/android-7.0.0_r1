#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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

import time

from acts.utils import rand_ascii_str
from acts.test_utils.tel.tel_subscription_utils import \
    get_subid_from_slot_index
from acts.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_subscription_utils import get_default_data_sub_id
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_test_utils import ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import is_droid_in_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import rat_generation_from_rat
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_data_attach_for_subscription


def wifi_tethering_cleanup(log, provider, client_list):
    """Clean up steps for WiFi Tethering.

    Make sure provider turn off tethering.
    Make sure clients reset WiFi and turn on cellular data.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.

    Returns:
        True if no error happened. False otherwise.
    """
    for client in client_list:
        client.droid.telephonyToggleDataConnection(True)
        if not WifiUtils.wifi_reset(log, client):
            log.error("Reset client WiFi failed. {}".format(client.serial))
            return False
    if not provider.droid.wifiIsApEnabled():
        log.error("Provider WiFi tethering stopped.")
        return False
    if not WifiUtils.stop_wifi_tethering(log, provider):
        log.error("Provider strop WiFi tethering failed.")
        return False
    return True


def wifi_tethering_setup_teardown(log,
                                  provider,
                                  client_list,
                                  ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                                  check_interval=30,
                                  check_iteration=4,
                                  do_cleanup=True,
                                  ssid=None,
                                  password=None):
    """Test WiFi Tethering.

    Turn off WiFi on clients.
    Turn off data and reset WiFi on clients.
    Verify no Internet access on clients.
    Turn on WiFi tethering on provider.
    Clients connect to provider's WiFI.
    Verify Internet on provider and clients.
    Tear down WiFi tethering setup and clean up.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        ap_band: setup WiFi tethering on 2G or 5G.
            This is optional, default value is WifiUtils.WIFI_CONFIG_APBAND_2G
        check_interval: delay time between each around of Internet connection check.
            This is optional, default value is 30 (seconds).
        check_iteration: check Internet connection for how many times in total.
            This is optional, default value is 4 (4 times).
        do_cleanup: after WiFi tethering test, do clean up to tear down tethering
            setup or not. This is optional, default value is True.
        ssid: use this string as WiFi SSID to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.
        password: use this string as WiFi password to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.

    Returns:
        True if no error happened. False otherwise.
    """
    log.info("--->Start wifi_tethering_setup_teardown<---")
    log.info("Provider: {}".format(provider.serial))
    if not provider.droid.connectivityIsTetheringSupported():
        log.error("Provider does not support tethering. Stop tethering test.")
        return False

    if ssid is None:
        ssid = rand_ascii_str(10)
    if password is None:
        password = rand_ascii_str(8)

    # No password
    if password == "":
        password = None

    try:
        for client in client_list:
            log.info("Client: {}".format(client.serial))
            WifiUtils.wifi_toggle_state(log, client, False)
            client.droid.telephonyToggleDataConnection(False)
        log.info("WiFI Tethering: Verify client have no Internet access.")
        for client in client_list:
            if verify_http_connection(log, client):
                log.error("Turn off Data on client fail. {}".format(
                    client.serial))
                return False

        log.info(
            "WiFI Tethering: Turn on WiFi tethering on {}. SSID: {}, password: {}".format(
                provider.serial, ssid, password))

        if not WifiUtils.start_wifi_tethering(log, provider, ssid, password,
                                              ap_band):
            log.error("Provider start WiFi tethering failed.")
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        log.info("Provider {} check Internet connection.".format(
            provider.serial))
        if not verify_http_connection(log, provider):
            return False
        for client in client_list:
            log.info(
                "WiFI Tethering: {} connect to WiFi and verify AP band correct.".format(
                    client.serial))
            if not ensure_wifi_connected(log, client, ssid, password):
                log.error("Client connect to WiFi failed.")
                return False

            wifi_info = client.droid.wifiGetConnectionInfo()
            if ap_band == WifiUtils.WIFI_CONFIG_APBAND_5G:
                if wifi_info["is_24ghz"]:
                    log.error("Expected 5g network. WiFi Info: {}".format(
                        wifi_info))
                    return False
            else:
                if wifi_info["is_5ghz"]:
                    log.error("Expected 2g network. WiFi Info: {}".format(
                        wifi_info))
                    return False

            log.info("Client{} check Internet connection.".format(
                client.serial))
            if (not wait_for_wifi_data_connection(log, client, True) or
                    not verify_http_connection(log, client)):
                log.error("No WiFi Data on client: {}.".format(client.serial))
                return False

        if not tethering_check_internet_connection(
                log, provider, client_list, check_interval, check_iteration):
            return False

    finally:
        if (do_cleanup and
            (not wifi_tethering_cleanup(log, provider, client_list))):
            return False
    return True


def tethering_check_internet_connection(log, provider, client_list,
                                        check_interval, check_iteration):
    """During tethering test, check client(s) and provider Internet connection.

    Do the following for <check_iteration> times:
        Delay <check_interval> seconds.
        Check Tethering provider's Internet connection.
        Check each client's Internet connection.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        check_interval: delay time between each around of Internet connection check.
        check_iteration: check Internet connection for how many times in total.

    Returns:
        True if no error happened. False otherwise.
    """
    for i in range(1, check_iteration):
        time.sleep(check_interval)
        log.info(
            "Provider {} check Internet connection after {} seconds.".format(
                provider.serial, check_interval * i))
        if not verify_http_connection(log, provider):
            return False
        for client in client_list:
            log.info(
                "Client {} check Internet connection after {} seconds.".format(
                    client.serial, check_interval * i))
            if not verify_http_connection(log, client):
                return False
    return True


def wifi_cell_switching(log, ad, wifi_network_ssid, wifi_network_pass, nw_gen):
    """Test data connection network switching when phone on <nw_gen>.

    Ensure phone is on <nw_gen>
    Ensure WiFi can connect to live network,
    Airplane mode is off, data connection is on, WiFi is on.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.
    Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.

    Args:
        log: log object.
        ad: android object.
        wifi_network_ssid: ssid for live wifi network.
        wifi_network_pass: password for live wifi network.
        nw_gen: network generation the phone should be camped on.

    Returns:
        True if pass.
    """
    try:

        if not ensure_network_generation_for_subscription(
                log, ad, get_default_data_sub_id(ad), nw_gen,
                MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            log.error("Device failed to register in {}".format(nw_gen))
            return False

        # Ensure WiFi can connect to live network
        log.info("Make sure phone can connect to live network by WIFI")
        if not ensure_wifi_connected(log, ad, wifi_network_ssid,
                                     wifi_network_pass):
            log.error("WiFi connect fail.")
            return False
        log.info("Phone connected to WIFI.")

        log.info("Step1 Airplane Off, WiFi On, Data On.")
        toggle_airplane_mode(log, ad, False)
        WifiUtils.wifi_toggle_state(log, ad, True)
        ad.droid.telephonyToggleDataConnection(True)
        if (not wait_for_wifi_data_connection(log, ad, True) or
                not verify_http_connection(log, ad)):
            log.error("Data is not on WiFi")
            return False

        log.info("Step2 WiFi is Off, Data is on Cell.")
        WifiUtils.wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True) or
                not verify_http_connection(log, ad)):
            log.error("Data did not return to cell")
            return False

        log.info("Step3 WiFi is On, Data is on WiFi.")
        WifiUtils.wifi_toggle_state(log, ad, True)
        if (not wait_for_wifi_data_connection(log, ad, True) or
                not verify_http_connection(log, ad)):
            log.error("Data did not return to WiFi")
            return False

        log.info("Step4 WiFi is Off, Data is on Cell.")
        WifiUtils.wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True) or
                not verify_http_connection(log, ad)):
            log.error("Data did not return to cell")
            return False
        return True

    finally:
        WifiUtils.wifi_toggle_state(log, ad, False)


def airplane_mode_test(log, ad):
    """ Test airplane mode basic on Phone and Live SIM.

    Ensure phone attach, data on, WiFi off and verify Internet.
    Turn on airplane mode to make sure detach.
    Turn off airplane mode to make sure attach.
    Verify Internet connection.

    Args:
        log: log object.
        ad: android object.

    Returns:
        True if pass; False if fail.
    """
    if not ensure_phones_idle(log, [ad]):
        log.error("Failed to return phones to idle.")
        return False

    try:
        ad.droid.telephonyToggleDataConnection(True)
        WifiUtils.wifi_toggle_state(log, ad, False)

        log.info("Step1: ensure attach")
        if not toggle_airplane_mode(log, ad, False):
            log.error("Failed initial attach")
            return False
        if not verify_http_connection(log, ad):
            log.error("Data not available on cell.")
            return False

        log.info("Step2: enable airplane mode and ensure detach")
        if not toggle_airplane_mode(log, ad, True):
            log.error("Failed to enable Airplane Mode")
            return False
        if not wait_for_cell_data_connection(log, ad, False):
            log.error("Failed to disable cell data connection")
            return False
        if verify_http_connection(log, ad):
            log.error("Data available in airplane mode.")
            return False

        log.info("Step3: disable airplane mode and ensure attach")
        if not toggle_airplane_mode(log, ad, False):
            log.error("Failed to disable Airplane Mode")
            return False

        if not wait_for_cell_data_connection(log, ad, True):
            log.error("Failed to enable cell data connection")
            return False

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        log.info("Step4 verify internet")
        return verify_http_connection(log, ad)
    finally:
        toggle_airplane_mode(log, ad, False)


def data_connectivity_single_bearer(log, ad, nw_gen):
    """Test data connection: single-bearer (no voice).

    Turn off airplane mode, enable Cellular Data.
    Ensure phone data generation is expected.
    Verify Internet.
    Disable Cellular Data, verify Internet is inaccessible.
    Enable Cellular Data, verify Internet.

    Args:
        log: log object.
        ad: android object.
        nw_gen: network generation the phone should on.

    Returns:
        True if success.
        False if failed.
    """
    ensure_phones_idle(log, [ad])

    if not ensure_network_generation_for_subscription(
            log, ad, get_default_data_sub_id(ad), nw_gen,
            MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
        log.error("Device failed to reselect in {}s.".format(
            MAX_WAIT_TIME_NW_SELECTION))
        return False

    try:
        log.info("Step1 Airplane Off, Data On.")
        toggle_airplane_mode(log, ad, False)
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True):
            log.error("Failed to enable data connection.")
            return False

        log.info("Step2 Verify internet")
        if not verify_http_connection(log, ad):
            log.error("Data not available on cell.")
            return False

        log.info("Step3 Turn off data and verify not connected.")
        ad.droid.telephonyToggleDataConnection(False)
        if not wait_for_cell_data_connection(log, ad, False):
            log.error("Step3 Failed to disable data connection.")
            return False

        if verify_http_connection(log, ad):
            log.error("Step3 Data still available when disabled.")
            return False

        log.info("Step4 Re-enable data.")
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True):
            log.error("Step4 failed to re-enable data.")
            return False
        if not verify_http_connection(log, ad):
            log.error("Data not available on cell.")
            return False

        if not is_droid_in_network_generation_for_subscription(
                log, ad, get_default_data_sub_id(ad), nw_gen,
                NETWORK_SERVICE_DATA):
            log.error("Failed: droid is no longer on correct network")
            log.info("Expected:{}, Current:{}".format(
                nw_gen, rat_generation_from_rat(
                    get_network_rat_for_subscription(
                        log, ad, get_default_data_sub_id(
                            ad), NETWORK_SERVICE_DATA))))
            return False
        return True
    finally:
        ad.droid.telephonyToggleDataConnection(True)


def change_data_sim_and_verify_data(log, ad, sim_slot):
    """Change Data SIM and verify Data attach and Internet access

    Args:
        log: log object.
        ad: android device object.
        sim_slot: SIM slot index.

    Returns:
        Data SIM changed successfully, data attached and Internet access is OK.
    """
    sub_id = get_subid_from_slot_index(log, ad, sim_slot)
    log.info("Change Data to subId: {}, SIM slot: {}".format(sub_id, sim_slot))
    set_subid_for_data(ad, sub_id)
    if not wait_for_data_attach_for_subscription(log, ad, sub_id,
                                                 MAX_WAIT_TIME_NW_SELECTION):
        log.error("Failed to attach data on subId:{}".format(sub_id))
        return False
    if not verify_http_connection(log, ad):
        log.error("No Internet access after changing Data SIM.")
        return False
    return True
