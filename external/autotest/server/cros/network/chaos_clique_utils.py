# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time
import re

import common
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.server import hosts
from autotest_lib.server import frontend
from autotest_lib.server import site_utils
from autotest_lib.server.cros.ap_configurators import ap_configurator
from autotest_lib.server.cros.ap_configurators import ap_cartridge
from autotest_lib.server.cros.ap_configurators import ap_spec as ap_spec_module


def allocate_packet_capturer(lock_manager, hostname):
    """Allocates a machine to capture packets.

    Locks the allocated machine if the machine was discovered via AFE
    to prevent tests stomping on each other.

    @param lock_manager HostLockManager object.
    @param hostname string optional hostname of a packet capture machine.

    @return: An SSHHost object representing a locked packet_capture machine.
    """
    if hostname is not None:
        return hosts.SSHHost(hostname)

    afe = frontend.AFE(debug=True,
                       server=site_utils.get_global_afe_hostname())
    return hosts.SSHHost(site_utils.lock_host_with_labels(
            afe, lock_manager, labels=['packet_capture']) + '.cros')


def allocate_webdriver_instance(lock_manager):
    """Allocates a machine to capture webdriver instance.

    Locks the allocated machine if the machine was discovered via AFE
    to prevent tests stomping on each other.

    @param lock_manager HostLockManager object.

    @return string hostname of locked webdriver instance
    """
    afe = frontend.AFE(debug=True,
                       server=site_utils.get_global_afe_hostname())
    webdriver_hostname = site_utils.lock_host_with_labels(afe, lock_manager,
                                    labels=['webdriver'])
    if webdriver_hostname is not None:
        return webdriver_hostname
    logging.error("Unable to allocate VM instance")
    return None


def power_on_VM(master, instance):
    """Power on VM

    @param master: chaosvmmaster SSHHost
    @param instance: locked webdriver instance

    """
    logging.debug('Powering on %s VM', instance)
    power_on_cmd = 'VBoxManage startvm %s' % instance
    master.run(power_on_cmd)


def power_off_VM(master, instance):
    """Power off VM

    @param master: chaosvmmaster SSHHost
    @param instance: locked webdriver instance

    """
    logging.debug('Powering off %s VM', instance)
    power_off_cmd = 'VBoxManage controlvm %s poweroff' % instance
    master.run(power_off_cmd)


def power_down_aps(aps, broken_pdus=[]):
     """Powers down a list of aps.

     @param aps: a list of APConfigurator objects.
     @param broken_pdus: a list of broken PDUs identified.
     """
     cartridge = ap_cartridge.APCartridge()
     for ap in aps:
         ap.power_down_router()
         cartridge.push_configurator(ap)
     cartridge.run_configurators(broken_pdus)


def configure_aps(aps, ap_spec, broken_pdus=[]):
    """Configures a given list of APs.

    @param aps: a list of APConfigurator objects.
    @param ap_spec: APSpec object corresponding to the AP configuration.
    @param broken_pdus: a list of broken PDUs identified.
    """
    cartridge = ap_cartridge.APCartridge()
    for ap in aps:
        ap.set_using_ap_spec(ap_spec)
        cartridge.push_configurator(ap)
    cartridge.run_configurators(broken_pdus)


def is_dut_healthy(client, ap):
    """Returns if iw scan is working properly.

    Sometimes iw scan will die, especially on the Atheros chips.
    This works around that bug.  See crbug.com/358716.

    @param client: a wifi_client for the DUT
    @param ap: ap_configurator object

    @returns True if the DUT is healthy (iw scan works); False otherwise.
    """
    # The SSID doesn't matter, all that needs to be verified is that iw
    # works.
    networks = client.iw_runner.wait_for_scan_result(
            client.wifi_if, ssids=[ap.ssid])
    if networks == None:
        return False
    return True


def is_conn_worker_healthy(conn_worker, ap, assoc_params, job):
    """Returns if the connection worker is working properly.

    From time to time the connection worker will fail to establish a
    connection to the APs.

    @param conn_worker: conn_worker object
    @param ap: an ap_configurator object
    @param assoc_params: the connection association parameters
    @param job: the Autotest job object

    @returns True if the worker is healthy; False otherwise
    """
    if conn_worker is None:
        return True
    conn_status = conn_worker.connect_work_client(assoc_params)
    if not conn_status:
        job.run_test('network_WiFi_ChaosConfigFailure', ap=ap,
                     error_string=ap_constants.WORK_CLI_CONNECT_FAIL,
                     tag=ap.ssid)
        # Obtain the logs from the worker
        log_dir_name = str('worker_client_logs_%s' % ap.ssid)
        log_dir = os.path.join(job.resultdir, log_dir_name)
        conn_worker.host.collect_logs(
                '/var/log', log_dir, ignore_errors=True)
        return False
    return True


def release_ap(ap, batch_locker, broken_pdus=[]):
    """Powers down and unlocks the given AP.

    @param ap: the APConfigurator under test.
    @param batch_locker: the batch locker object.
    @param broken_pdus: a list of broken PDUs identified.
    """
    ap.power_down_router()
    try:
        ap.apply_settings()
    except ap_configurator.PduNotResponding as e:
        if ap.pdu not in broken_pdus:
            broken_pdus.append(ap.pdu)
    batch_locker.unlock_one_ap(ap.host_name)


def filter_quarantined_and_config_failed_aps(aps, batch_locker, job,
                                             broken_pdus=[]):
    """Filter out all PDU quarantined and config failed APs.

    @param aps: the list of ap_configurator objects to filter
    @param batch_locker: the batch_locker object
    @param job: an Autotest job object
    @param broken_pdus: a list of broken PDUs identified.

    @returns a list of ap_configuration objects.
    """
    aps_to_remove = list()
    for ap in aps:
        failed_ap = False
        if ap.pdu in broken_pdus:
            ap.configuration_success = ap_constants.PDU_FAIL
        if (ap.configuration_success == ap_constants.PDU_FAIL):
            failed_ap = True
            error_string = ap_constants.AP_PDU_DOWN
            tag = ap.host_name + '_PDU'
        elif (ap.configuration_success == ap_constants.CONFIG_FAIL):
            failed_ap = True
            error_string = ap_constants.AP_CONFIG_FAIL
            tag = ap.host_name
        if failed_ap:
            tag += '_' + str(int(round(time.time())))
            job.run_test('network_WiFi_ChaosConfigFailure',
                         ap=ap,
                         error_string=error_string,
                         tag=tag)
            aps_to_remove.append(ap)
            if error_string == ap_constants.AP_CONFIG_FAIL:
                release_ap(ap, batch_locker, broken_pdus)
            else:
                # Cannot use _release_ap, since power_down will fail
                batch_locker.unlock_one_ap(ap.host_name)
    return list(set(aps) - set(aps_to_remove))


def get_security_from_scan(ap, networks, job):
    """Returns a list of securities determined from the scan result.

    @param ap: the APConfigurator being testing against.
    @param networks: List of matching networks returned from scan.
    @param job: an Autotest job object

    @returns a list of possible securities for the given network.
    """
    securities = list()
    # Sanitize MIXED security setting for both Static and Dynamic
    # configurators before doing the comparison.
    security = networks[0].security
    if (security == iw_runner.SECURITY_MIXED and
        ap.configurator_type == ap_spec_module.CONFIGURATOR_STATIC):
        securities = [iw_runner.SECURITY_WPA, iw_runner.SECURITY_WPA2]
        # We have only seen WPA2 be backwards compatible, and we want
        # to verify the configurator did the right thing. So we
        # promote this to WPA2 only.
    elif (security == iw_runner.SECURITY_MIXED and
          ap.configurator_type == ap_spec_module.CONFIGURATOR_DYNAMIC):
        securities = [iw_runner.SECURITY_WPA2]
    else:
        securities = [security]
    return securities


def scan_for_networks(ssid, capturer, ap_spec):
    """Returns a list of matching networks after running iw scan.

    @param ssid: the SSID string to look for in scan.
    @param capturer: a packet capture device.
    @param ap_spec: APSpec object corresponding to the AP configuration.

    @returns a list of the matching networks; if no networks are found at
             all, returns None.
    """
    # Setup a managed interface to perform scanning on the
    # packet capture device.
    freq = ap_spec_module.FREQUENCY_TABLE[ap_spec.channel]
    wifi_if = capturer.get_wlanif(freq, 'managed')
    capturer.host.run('%s link set %s up' % (capturer.cmd_ip, wifi_if))
    # We have some APs that need a while to come on-line
    networks = capturer.iw_runner.wait_for_scan_result(
            wifi_if, ssids=[ssid], timeout_seconds=300)
    capturer.remove_interface(wifi_if)
    return networks


def return_available_networks(ap, capturer, job, ap_spec):
    """Returns a list of networks configured as described by an APSpec.

    @param ap: the APConfigurator being testing against.
    @param capturer: a packet capture device
    @param job: an Autotest job object.
    @param ap_spec: APSpec object corresponding to the AP configuration.

    @returns a list of networks returned from _scan_for_networks().
    """
    for i in range(2):
        networks = scan_for_networks(ap.ssid, capturer, ap_spec)
        if networks is None:
            return None
        if len(networks) == 0:
            # The SSID wasn't even found, abort
            logging.error('The ssid %s was not found in the scan', ap.ssid)
            job.run_test('network_WiFi_ChaosConfigFailure', ap=ap,
                         error_string=ap_constants.AP_SSID_NOTFOUND,
                         tag=ap.ssid)
            return list()
        security = get_security_from_scan(ap, networks, job)
        if ap_spec.security in security:
            return networks
        if i == 0:
            # The SSID exists but the security is wrong, give the AP time
            # to possible update it.
            time.sleep(60)
    if ap_spec.security not in security:
        logging.error('%s was the expected security but got %s: %s',
                      ap_spec.security,
                      str(security).strip('[]'),
                      networks)
        job.run_test('network_WiFi_ChaosConfigFailure',
                     ap=ap,
                     error_string=ap_constants.AP_SECURITY_MISMATCH,
                     tag=ap.ssid)
        networks = list()
    return networks


def sanitize_client(host):
    """Clean up logs and reboot the DUT.

    @param host: the cros host object to use for RPC calls.
    """
    host.run('rm -rf /var/log')
    host.reboot()


def get_firmware_ver(host):
    """Get firmware version of DUT from /var/log/messages.

    WiFi firmware version is matched against list of known firmware versions
    from ToT.

    @param host: the cros host object to use for RPC calls.

    @returns the WiFi firmware version as a string, None if the version
             cannot be found.
    """
    # TODO(rpius): Need to find someway to get this info for Android/Brillo.
    if host.get_os_type() != 'cros':
        return None

    # Firmware versions manually aggregated by installing ToT on each device
    known_firmware_ver = ['Atheros', 'mwifiex', 'loaded firmware version',
                          'brcmf_c_preinit_dcmds']
    # Find and return firmware version in logs
    for firmware_ver in known_firmware_ver:
        result_str = host.run(
            'awk "/%s/ {print}" /var/log/messages' % firmware_ver).stdout
        if not result_str:
            continue
        else:
            if 'Atheros' in result_str:
                pattern = '%s \w+ Rev:\d' % firmware_ver
            elif 'mwifiex' in result_str:
                pattern = '%s [\d.]+ \([\w.]+\)' % firmware_ver
            elif 'loaded firmware version' in result_str:
                pattern = '(\d+\.\d+\.\d+)'
            elif 'Firmware version' in result_str:
                pattern = '\d+\.\d+\.\d+ \([\w.]+\)'
            else:
                logging.info('%s does not match known firmware versions.',
                             result_str)
                return None
            result = re.search(pattern, result_str)
            if result:
                return result.group(0)
    return None
