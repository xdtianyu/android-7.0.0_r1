# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import site_utils
from autotest_lib.client.common_lib.cros import site_eap_certs
from autotest_lib.client.common_lib.cros import virtual_ethernet_pair
from autotest_lib.client.cros import certificate_util
from autotest_lib.client.cros import shill_temporary_profile
from autotest_lib.client.cros import tpm_store
from autotest_lib.client.cros import vpn_server
from autotest_lib.client.cros.networking import shill_proxy

class network_VPNConnect(test.test):
    """The VPN authentication class.

    Starts up a VPN server within a chroot on the other end of a virtual
    ethernet pair and attempts a VPN association using shill.

    """
    CLIENT_INTERFACE_NAME = 'pseudoethernet0'
    SERVER_INTERFACE_NAME = 'serverethernet0'
    TEST_PROFILE_NAME = 'testVPN'
    CONNECT_TIMEOUT_SECONDS = 15
    version = 1
    SERVER_ADDRESS = '10.9.8.1'
    CLIENT_ADDRESS = '10.9.8.2'
    NETWORK_PREFIX = 24

    def get_device(self, interface_name):
        """Finds the corresponding Device object for an ethernet
        interface with the name |interface_name|.

        @param interface_name string The name of the interface to check.

        @return DBus interface object representing the associated device.

        """
        device = self._shill_proxy.find_object('Device',
                                               {'Name': interface_name})
        if device is None:
            raise error.TestFail('Device was not found.')

        return device


    def find_ethernet_service(self, interface_name):
        """Finds the corresponding service object for an ethernet
        interface.

        @param interface_name string The name of the associated interface

        @return Service object representing the associated service.

        """
        device = self.get_device(interface_name)
        device_path = shill_proxy.ShillProxy.dbus2primitive(device.object_path)
        return self._shill_proxy.find_object('Service', {'Device': device_path})


    def configure_static_ip(self, interface_name, address, prefix_len):
        """Configures the Static IP parameters for the Ethernet interface
        |interface_name| and applies those parameters to the interface by
        forcing a re-connect.

        @param interface_name string The name of the associated interface.
        @param address string the IP address this interface should have.
        @param prefix_len string the IP address prefix for the interface.

        """
        service = self.find_ethernet_service(interface_name)
        service.SetProperty('StaticIP.Address', address)
        service.SetProperty('StaticIP.Prefixlen', prefix_len)
        service.Disconnect()
        service.Connect()


    def get_vpn_server(self):
        """Returns a VPN server instance."""
        if self._vpn_type.startswith('l2tpipsec-psk'):
            return vpn_server.L2TPIPSecVPNServer('psk',
                                                 self.SERVER_INTERFACE_NAME,
                                                 self.SERVER_ADDRESS,
                                                 self.NETWORK_PREFIX,
                                                 'xauth' in self._vpn_type)
        elif self._vpn_type.startswith('l2tpipsec-cert'):
            return vpn_server.L2TPIPSecVPNServer('cert',
                                                 self.SERVER_INTERFACE_NAME,
                                                 self.SERVER_ADDRESS,
                                                 self.NETWORK_PREFIX)
        elif self._vpn_type.startswith('openvpn'):
            return vpn_server.OpenVPNServer(self.SERVER_INTERFACE_NAME,
                                            self.SERVER_ADDRESS,
                                            self.NETWORK_PREFIX,
                                            'user_pass' in self._vpn_type)
        else:
            raise error.TestFail('Unknown vpn server type %s' % self._vpn_type)


    def get_vpn_client_properties(self, tpm):
        """Returns VPN configuration properties.

        @param tpm object TPM store instance to add credentials if necessary.

        """
        if self._vpn_type.startswith('l2tpipsec-psk'):
            params = {
                'L2TPIPsec.Password': vpn_server.L2TPIPSecVPNServer.CHAP_SECRET,
                'L2TPIPsec.PSK':
                        vpn_server.L2TPIPSecVPNServer.IPSEC_PRESHARED_KEY,
                'L2TPIPsec.User':vpn_server.L2TPIPSecVPNServer.CHAP_USER,
                'Name': 'test-vpn-l2tp-psk',
                'Provider.Host': self.SERVER_ADDRESS,
                'Provider.Type': 'l2tpipsec',
                'Type': 'vpn',
                'VPN.Domain': 'test-vpn-psk-domain'
            }
            if 'xauth' in self._vpn_type:
                if 'incorrect_user' in self._vpn_type:
                    params['L2TPIPsec.XauthUser'] = 'wrong_user'
                    params['L2TPIPsec.XauthPassword'] = 'wrong_password'
                elif 'incorrect_missing_user' not in self._vpn_type:
                    params['L2TPIPsec.XauthUser'] = (
                            vpn_server.L2TPIPSecVPNServer.XAUTH_USER)
                    params['L2TPIPsec.XauthPassword'] = (
                            vpn_server.L2TPIPSecVPNServer.XAUTH_PASSWORD)
            return params
        elif self._vpn_type == 'l2tpipsec-cert':
            tpm.install_certificate(site_eap_certs.client_cert_1,
                                    site_eap_certs.cert_1_tpm_key_id)
            tpm.install_private_key(site_eap_certs.client_private_key_1,
                                    site_eap_certs.cert_1_tpm_key_id)
            return {
                'L2TPIPsec.CACertPEM': [ site_eap_certs.ca_cert_1 ],
                'L2TPIPsec.ClientCertID': site_eap_certs.cert_1_tpm_key_id,
                'L2TPIPsec.ClientCertSlot': tpm.SLOT_ID,
                'L2TPIPsec.User':vpn_server.L2TPIPSecVPNServer.CHAP_USER,
                'L2TPIPsec.Password': vpn_server.L2TPIPSecVPNServer.CHAP_SECRET,
                'L2TPIPsec.PIN': tpm.PIN,
                'Name': 'test-vpn-l2tp-cert',
                'Provider.Host': self.SERVER_ADDRESS,
                'Provider.Type': 'l2tpipsec',
                'Type': 'vpn',
                'VPN.Domain': 'test-vpn-psk-domain'
            }
        elif self._vpn_type.startswith('openvpn'):
            tpm.install_certificate(site_eap_certs.client_cert_1,
                                    site_eap_certs.cert_1_tpm_key_id)
            tpm.install_private_key(site_eap_certs.client_private_key_1,
                                    site_eap_certs.cert_1_tpm_key_id)
            params = {
                'Name': 'test-vpn-openvpn',
                'Provider.Host': self.SERVER_ADDRESS,
                'Provider.Type': 'openvpn',
                'Type': 'vpn',
                'VPN.Domain': 'test-openvpn-domain',
                'OpenVPN.CACertPEM': [ site_eap_certs.ca_cert_1 ],
                'OpenVPN.Pkcs11.ID': site_eap_certs.cert_1_tpm_key_id,
                'OpenVPN.Pkcs11.PIN': tpm.PIN,
                'OpenVPN.RemoteCertEKU': 'TLS Web Server Authentication',
                'OpenVPN.Verb': '5'
            }
            if 'user_pass' in self._vpn_type:
                params['OpenVPN.User'] = vpn_server.OpenVPNServer.USERNAME
                params['OpenVPN.Password'] = vpn_server.OpenVPNServer.PASSWORD
            if 'cert_verify' in self._vpn_type:
                ca = certificate_util.PEMCertificate(site_eap_certs.ca_cert_1)
                if 'incorrect_hash' in self._vpn_type:
                    bogus_hash = ':'.join(['00'] * 20)
                    params['OpenVPN.VerifyHash'] = bogus_hash
                else:
                    params['OpenVPN.VerifyHash'] = ca.fingerprint
                server = certificate_util.PEMCertificate(
                        site_eap_certs.server_cert_1)
                if 'incorrect_subject' in self._vpn_type:
                    params['OpenVPN.VerifyX509Name'] = 'bogus subject name'
                elif 'incorrect_cn' in self._vpn_type:
                    params['OpenVPN.VerifyX509Name'] = 'bogus cn'
                    params['OpenVPN.VerifyX509Type'] = 'name'
                elif 'cn_only' in self._vpn_type:
                    params['OpenVPN.VerifyX509Name'] = server.subject_dict['CN']
                    params['OpenVPN.VerifyX509Type'] = 'name'
                else:
                    # This is the form OpenVPN expects.
                    params['OpenVPN.VerifyX509Name'] = ', '.join(server.subject)
            return params
        else:
            raise error.TestFail('Unknown vpn client type %s' % self._vpn_type)


    def connect_vpn(self):
        """Connects the client to the VPN server."""
        proxy = self._shill_proxy
        with tpm_store.TPMStore() as tpm:
            service = proxy.get_service(self.get_vpn_client_properties(tpm))
            service.Connect()
            result = proxy.wait_for_property_in(service,
                                                proxy.SERVICE_PROPERTY_STATE,
                                                ('ready', 'online'),
                                                self.CONNECT_TIMEOUT_SECONDS)
        (successful, _, _) = result
        if not successful and self._expect_success:
            raise error.TestFail('VPN connection failed')
        if successful and not self._expect_success:
            raise error.TestFail('VPN connection suceeded '
                                 'when it should have failed')


    def run_once(self, vpn_types=[]):
        """Test main loop."""
        self._shill_proxy = shill_proxy.ShillProxy()
        for vpn_type in vpn_types:
            self.run_vpn_test(vpn_type)


    def run_vpn_test(self, vpn_type):
        """Run a vpn test of |vpn_type|.

        @param vpn_type string type of VPN test to run.

        """
        manager = self._shill_proxy.manager
        server_address_and_prefix = '%s/%d' % (self.SERVER_ADDRESS,
                                               self.NETWORK_PREFIX)
        client_address_and_prefix = '%s/%d' % (self.CLIENT_ADDRESS,
                                               self.NETWORK_PREFIX)
        self._vpn_type = vpn_type
        self._expect_success = 'incorrect' not in vpn_type

        with shill_temporary_profile.ShillTemporaryProfile(
                manager, profile_name=self.TEST_PROFILE_NAME):
            with virtual_ethernet_pair.VirtualEthernetPair(
                    interface_name=self.SERVER_INTERFACE_NAME,
                    peer_interface_name=self.CLIENT_INTERFACE_NAME,
                    peer_interface_ip=client_address_and_prefix,
                    interface_ip=server_address_and_prefix,
                    ignore_shutdown_errors=True) as ethernet_pair:
                if not ethernet_pair.is_healthy:
                    raise error.TestFail('Virtual ethernet pair failed.')

                # When shill finds this ethernet interface, it will reset
                # its IP address and start a DHCP client.  We must configure
                # the static IP address through shill.
                self.configure_static_ip(self.CLIENT_INTERFACE_NAME,
                                         self.CLIENT_ADDRESS,
                                         self.NETWORK_PREFIX)

                with self.get_vpn_server() as server:
                    self.connect_vpn()
                    site_utils.ping(server.SERVER_IP_ADDRESS, tries=3)
