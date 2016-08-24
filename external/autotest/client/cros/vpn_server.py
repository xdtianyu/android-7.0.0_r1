# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.cros import network_chroot
from autotest_lib.client.common_lib.cros import site_eap_certs

class VPNServer(object):
    """Context enclosing the use of a VPN server instance."""

    def __enter__(self):
        self.start_server()
        return self


    def __exit__(self, exception, value, traceback):
        logging.info('Log contents: %s', self.get_log_contents())
        self.stop_server()


class L2TPIPSecVPNServer(VPNServer):
    """Implementation of an L2TP/IPSec VPN.  Uses ipsec starter and xl2tpd."""
    PRELOAD_MODULES = ('af_key', 'ah4', 'esp4', 'ipcomp', 'xfrm_user',
                       'xfrm4_tunnel')
    ROOT_DIRECTORIES = ('etc/ipsec.d', 'etc/ipsec.d/cacerts',
                        'etc/ipsec.d/certs', 'etc/ipsec.d/crls',
                        'etc/ipsec.d/private', 'etc/ppp', 'etc/xl2tpd')
    CHAP_USER = 'chapuser'
    CHAP_SECRET = 'chapsecret'
    IPSEC_COMMAND = '/usr/sbin/ipsec'
    IPSEC_LOGFILE = 'var/log/charon.log'
    IPSEC_PRESHARED_KEY = 'preshared-key'
    IPSEC_CA_CERTIFICATE = 'etc/ipsec.d/cacerts/ca.cert'
    IPSEC_SERVER_CERTIFICATE = 'etc/ipsec.d/certs/server.cert'
    PPPD_PID_FILE = 'var/run/ppp0.pid'
    XAUTH_USER = 'xauth_user'
    XAUTH_PASSWORD = 'xauth_password'
    XAUTH_SECONDARY_AUTHENTICATION_STANZA = 'rightauth2=xauth'
    XL2TPD_COMMAND = '/usr/sbin/xl2tpd'
    XL2TPD_CONFIG_FILE = 'etc/xl2tpd/xl2tpd.conf'
    XL2TPD_PID_FILE = 'var/run/xl2tpd.pid'
    SERVER_IP_ADDRESS = '192.168.1.99'
    IPSEC_COMMON_CONFIGS = {
        'etc/strongswan.conf' :
            'charon {\n'
            '  filelog {\n'
            '    %(charon-logfile)s {\n'
            '      time_format = %%b %%e %%T\n'
            '      default = 3\n'
            '    }\n'
            '  }\n'
            '  install_routes = no\n'
            '  ignore_routing_tables = 0\n'
            '  routing_table = 0\n'
            '}\n',

        'etc/passwd' :
            'root:x:0:0:root:/root:/bin/bash\n'
            'ipsec:*:212:212::/dev/null:/bin/false\n',

        'etc/group' :
            'ipsec:x:212:\n',

        XL2TPD_CONFIG_FILE :
            '[global]\n'
            '\n'
            '[lns default]\n'
            '  ip range = 192.168.1.128-192.168.1.254\n'
            '  local ip = 192.168.1.99\n'
            '  require chap = yes\n'
            '  refuse pap = yes\n'
            '  require authentication = yes\n'
            '  name = LinuxVPNserver\n'
            '  ppp debug = yes\n'
            '  pppoptfile = /etc/ppp/options.xl2tpd\n'
            '  length bit = yes\n',

        'etc/xl2tpd/l2tp-secrets' :
            '*      them    l2tp-secret',

        'etc/ppp/chap-secrets' :
            '%(chap-user)s        *       %(chap-secret)s      *',

        'etc/ppp/options.xl2tpd' :
            'ipcp-accept-local\n'
            'ipcp-accept-remote\n'
            'noccp\n'
            'auth\n'
            'crtscts\n'
            'idle 1800\n'
            'mtu 1410\n'
            'mru 1410\n'
            'nodefaultroute\n'
            'debug\n'
            'lock\n'
            'proxyarp\n'
    }
    IPSEC_TYPED_CONFIGS = {
        'psk': {
            'etc/ipsec.conf' :
                'config setup\n'
                '  charondebug="%(charon-debug-flags)s"\n'
                'conn L2TP\n'
                '  keyexchange=ikev1\n'
                '  authby=psk\n'
                '  %(xauth-stanza)s\n'
                '  rekey=no\n'
                '  left=%(local-ip)s\n'
                '  leftprotoport=17/1701\n'
                '  right=%%any\n'
                '  rightprotoport=17/%%any\n'
                '  auto=add\n',

            'etc/ipsec.secrets' :
              '%(local-ip)s %%any : PSK "%(preshared-key)s"\n'
              '%(xauth-user)s : XAUTH "%(xauth-password)s"\n',
        },
        'cert': {
            'etc/ipsec.conf' :
                'config setup\n'
                '  charondebug="%(charon-debug-flags)s"\n'
                'conn L2TP\n'
                '  keyexchange=ikev1\n'
                '  left=%(local-ip)s\n'
                '  leftcert=server.cert\n'
                '  leftid="C=US, ST=California, L=Mountain View, '
                'CN=chromelab-wifi-testbed-server.mtv.google.com"\n'
                '  leftprotoport=17/1701\n'
                '  right=%%any\n'
                '  rightca="C=US, ST=California, L=Mountain View, '
                'CN=chromelab-wifi-testbed-root.mtv.google.com"\n'
                '  rightprotoport=17/%%any\n'
                '  auto=add\n',

            'etc/ipsec.secrets' : ': RSA server.key ""\n',

            IPSEC_SERVER_CERTIFICATE : site_eap_certs.server_cert_1,
            IPSEC_CA_CERTIFICATE : site_eap_certs.ca_cert_1,
            'etc/ipsec.d/private/server.key' :
                site_eap_certs.server_private_key_1,
        },
    }

    """Implementation of an L2TP/IPSec server instance."""
    def __init__(self, auth_type, interface_name, address, network_prefix,
                 perform_xauth_authentication=False):
        self._auth_type = auth_type
        self._chroot = network_chroot.NetworkChroot(interface_name,
                                                    address, network_prefix)
        self._perform_xauth_authentication = perform_xauth_authentication


    def start_server(self):
        """Start VPN server instance"""
        if self._auth_type not in self.IPSEC_TYPED_CONFIGS:
            raise RuntimeError('L2TP/IPSec type %s is not define' %
                               self._auth_type)
        chroot = self._chroot
        chroot.add_root_directories(self.ROOT_DIRECTORIES)
        chroot.add_config_templates(self.IPSEC_COMMON_CONFIGS)
        chroot.add_config_templates(self.IPSEC_TYPED_CONFIGS[self._auth_type])
        chroot.add_config_values({
            'chap-user': self.CHAP_USER,
            'chap-secret': self.CHAP_SECRET,
            'charon-debug-flags': 'dmn 2, mgr 2, ike 2, net 2',
            'charon-logfile': self.IPSEC_LOGFILE,
            'preshared-key': self.IPSEC_PRESHARED_KEY,
            'xauth-user': self.XAUTH_USER,
            'xauth-password': self.XAUTH_PASSWORD,
            'xauth-stanza': self.XAUTH_SECONDARY_AUTHENTICATION_STANZA
                    if self._perform_xauth_authentication else '',
        })
        chroot.add_startup_command('%s start' % self.IPSEC_COMMAND)
        chroot.add_startup_command('%s -c /%s -C /tmp/l2tpd.control' %
                                   (self.XL2TPD_COMMAND,
                                    self.XL2TPD_CONFIG_FILE))
        self.preload_modules()
        chroot.startup()


    def stop_server(self):
        """Start VPN server instance"""
        chroot = self._chroot
        chroot.run([self.IPSEC_COMMAND, 'stop'], ignore_status=True)
        chroot.kill_pid_file(self.XL2TPD_PID_FILE, missing_ok=True)
        chroot.kill_pid_file(self.PPPD_PID_FILE, missing_ok=True)
        chroot.shutdown()


    def get_log_contents(self):
        """Return all logs related to the chroot."""
        return self._chroot.get_log_contents()


    def preload_modules(self):
        """Pre-load ipsec modules since they can't be loaded from chroot."""
        for module in self.PRELOAD_MODULES:
            utils.system('modprobe %s' % module)


class OpenVPNServer(VPNServer):
    """Implementation of an OpenVPN service."""
    PRELOAD_MODULES = ('tun',)
    ROOT_DIRECTORIES = ('etc/openvpn', 'etc/ssl')
    CA_CERTIFICATE_FILE = 'etc/openvpn/ca.crt'
    SERVER_CERTIFICATE_FILE = 'etc/openvpn/server.crt'
    SERVER_KEY_FILE = 'etc/openvpn/server.key'
    DIFFIE_HELLMAN_FILE = 'etc/openvpn/diffie-hellman.pem'
    OPENVPN_COMMAND = '/usr/sbin/openvpn'
    OPENVPN_CONFIG_FILE = 'etc/openvpn/openvpn.conf'
    OPENVPN_PID_FILE = 'var/run/openvpn.pid'
    OPENVPN_STATUS_FILE = 'tmp/openvpn.status'
    AUTHENTICATION_SCRIPT = 'etc/openvpn_authentication_script.sh'
    EXPECTED_AUTHENTICATION_FILE = 'etc/openvpn_expected_authentication.txt'
    PASSWORD = 'password'
    USERNAME = 'username'
    SERVER_IP_ADDRESS = '10.11.12.1'
    CONFIGURATION = {
        'etc/ssl/blacklist' : '',
        CA_CERTIFICATE_FILE : site_eap_certs.ca_cert_1,
        SERVER_CERTIFICATE_FILE : site_eap_certs.server_cert_1,
        SERVER_KEY_FILE : site_eap_certs.server_private_key_1,
        DIFFIE_HELLMAN_FILE : site_eap_certs.dh1024_pem_key_1,
        AUTHENTICATION_SCRIPT :
            '#!/bin/bash\n'
            'diff -q $1 %(expected-authentication-file)s\n',
        EXPECTED_AUTHENTICATION_FILE : '%(username)s\n%(password)s\n',
        OPENVPN_CONFIG_FILE :
            'ca /%(ca-cert)s\n'
            'cert /%(server-cert)s\n'
            'dev tun\n'
            'dh /%(diffie-hellman-params-file)s\n'
            'keepalive 10 120\n'
            'local %(local-ip)s\n'
            'log /var/log/openvpn.log\n'
            'ifconfig-pool-persist /tmp/ipp.txt\n'
            'key /%(server-key)s\n'
            'persist-key\n'
            'persist-tun\n'
            'port 1194\n'
            'proto udp\n'
            'server 10.11.12.0 255.255.255.0\n'
            'status /%(status-file)s\n'
            'verb 5\n'
            'writepid /%(pid-file)s\n'
            '%(optional-user-verification)s\n'
    }

    def __init__(self, interface_name, address, network_prefix,
                 perform_username_authentication=False):
        self._chroot = network_chroot.NetworkChroot(interface_name,
                                                    address, network_prefix)
        self._perform_username_authentication = perform_username_authentication


    def start_server(self):
        """Start VPN server instance"""
        chroot = self._chroot
        chroot.add_root_directories(self.ROOT_DIRECTORIES)
        # Create a configuration template from the key-value pairs.
        chroot.add_config_templates(self.CONFIGURATION)
        config_values = {
            'ca-cert': self.CA_CERTIFICATE_FILE,
            'diffie-hellman-params-file': self.DIFFIE_HELLMAN_FILE,
            'expected-authentication-file': self.EXPECTED_AUTHENTICATION_FILE,
            'optional-user-verification': '',
            'password': self.PASSWORD,
            'pid-file': self.OPENVPN_PID_FILE,
            'server-cert': self.SERVER_CERTIFICATE_FILE,
            'server-key': self.SERVER_KEY_FILE,
            'status-file': self.OPENVPN_STATUS_FILE,
            'username': self.USERNAME,
        }
        if self._perform_username_authentication:
            config_values['optional-user-verification'] = (
                    'auth-user-pass-verify /%s via-file\nscript-security 2' %
                    self.AUTHENTICATION_SCRIPT)
        chroot.add_config_values(config_values)
        chroot.add_startup_command('chmod 755 %s' % self.AUTHENTICATION_SCRIPT)
        chroot.add_startup_command('%s --config /%s &' %
                                   (self.OPENVPN_COMMAND,
                                    self.OPENVPN_CONFIG_FILE))
        self.preload_modules()
        chroot.startup()


    def preload_modules(self):
        """Pre-load modules since they can't be loaded from chroot."""
        for module in self.PRELOAD_MODULES:
            utils.system('modprobe %s' % module)


    def get_log_contents(self):
        """Return all logs related to the chroot."""
        return self._chroot.get_log_contents()


    def stop_server(self):
        """Start VPN server instance"""
        chroot = self._chroot
        chroot.kill_pid_file(self.OPENVPN_PID_FILE, missing_ok=True)
        chroot.shutdown()
