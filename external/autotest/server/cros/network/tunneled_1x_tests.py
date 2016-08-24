# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros import site_eap_certs
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.network import hostap_config


def get_positive_8021x_test_cases(outer_auth_type, inner_auth_type):
    """Return a test case asserting that outer/inner auth works.

    @param inner_auth_type one of
            xmlrpc_security_types.Tunneled1xConfig.LAYER1_TYPE*
    @param inner_auth_type one of
            xmlrpc_security_types.Tunneled1xConfig.LAYER2_TYPE*
    @return list of ap_config, association_params tuples for
            network_WiFi_SimpleConnect.

    """
    eap_config = xmlrpc_security_types.Tunneled1xConfig(
            site_eap_certs.ca_cert_1,
            site_eap_certs.server_cert_1,
            site_eap_certs.server_private_key_1,
            site_eap_certs.ca_cert_1,
            'testuser',
            'password',
            inner_protocol=inner_auth_type,
            outer_protocol=outer_auth_type)
    ap_config = hostap_config.HostapConfig(
            frequency=2412,
            mode=hostap_config.HostapConfig.MODE_11G,
            security_config=eap_config)
    assoc_params = xmlrpc_datatypes.AssociationParameters(
            security_config=eap_config)
    return [(ap_config, assoc_params)]


def get_negative_8021x_test_cases(outer_auth_type, inner_auth_type):
    """Build a set of test cases for TTLS/PEAP authentication.

    @param inner_auth_type one of
            xmlrpc_security_types.Tunneled1xConfig.LAYER1_TYPE*
    @param inner_auth_type one of
            xmlrpc_security_types.Tunneled1xConfig.LAYER2_TYPE*
    @return list of ap_config, association_params tuples for
            network_WiFi_SimpleConnect.

    """
    configurations = []
    # Bad passwords won't work.
    eap_config = xmlrpc_security_types.Tunneled1xConfig(
            site_eap_certs.ca_cert_1,
            site_eap_certs.server_cert_1,
            site_eap_certs.server_private_key_1,
            site_eap_certs.ca_cert_1,
            'testuser',
            'password',
            inner_protocol=inner_auth_type,
            outer_protocol=outer_auth_type,
            client_password='wrongpassword')
    ap_config = hostap_config.HostapConfig(
            frequency=2412,
            mode=hostap_config.HostapConfig.MODE_11G,
            security_config=eap_config)
    assoc_params = xmlrpc_datatypes.AssociationParameters(
            security_config=eap_config,
            expect_failure=True)
    configurations.append((ap_config, assoc_params))
    # If use the wrong CA on the client, it won't trust the server credentials.
    eap_config = xmlrpc_security_types.Tunneled1xConfig(
            site_eap_certs.ca_cert_1,
            site_eap_certs.server_cert_1,
            site_eap_certs.server_private_key_1,
            site_eap_certs.ca_cert_2,
            'testuser',
            'password',
            inner_protocol=inner_auth_type,
            outer_protocol=outer_auth_type)
    ap_config = hostap_config.HostapConfig(
            frequency=2412,
            mode=hostap_config.HostapConfig.MODE_11G,
            security_config=eap_config)
    assoc_params = xmlrpc_datatypes.AssociationParameters(
            security_config=eap_config,
            expect_failure=True)
    configurations.append((ap_config, assoc_params))
    # And if the server's credentials are good but expired, we also reject it.
    eap_config = xmlrpc_security_types.Tunneled1xConfig(
            site_eap_certs.ca_cert_1,
            site_eap_certs.server_expired_cert,
            site_eap_certs.server_expired_key,
            site_eap_certs.ca_cert_1,
            'testuser',
            'password',
            inner_protocol=inner_auth_type,
            outer_protocol=outer_auth_type)
    ap_config = hostap_config.HostapConfig(
            frequency=2412,
            mode=hostap_config.HostapConfig.MODE_11G,
            security_config=eap_config)
    assoc_params = xmlrpc_datatypes.AssociationParameters(
            security_config=eap_config,
            expect_failure=True)
    configurations.append((ap_config, assoc_params))
    return configurations
