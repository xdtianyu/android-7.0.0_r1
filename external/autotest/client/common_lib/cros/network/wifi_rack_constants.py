# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import namedtuple

# Used in network_RackWiFiConnect
PASSWORD = 'chromeos'

SCAN_RETRY_TIMEOUT = 180

NetworkServices = namedtuple('NetworkServices',
                             ['testname', 'user', 'ssid', 'url', 'pattern'])

HIDDEN_WPA = NetworkServices('hiddenWPA',
                             'networktest01@croste.tv',
                             'CrOS_WPA_LinksysWRT54GL',
                             'www.openvpn.com',
                             'certs')

PROXY_NON_AUTH = NetworkServices('proxyNonAuth',
                                 'networktest01@croste.tv',
                                 'CrOS_WPA2_Airport_Xtreme_5GHz',
                                 'www.openvpn.com',
                                 'certs')

GOOGLE_GUEST = NetworkServices('googleGuest',
                               'networktest01@croste.tv',
                               'GoogleGuest',
                               'www.google.com',
                               'www.google.com')

WEP = NetworkServices('WEP',
                      'networktest01@croste.tv',
                      'CrOS_WEP_DLink_Dir601',
                      'www.openvpn.com',
                      'certs')

PEAP = NetworkServices('PEAP',
                       'networktest01@croste.tv',
                       'CrOS_WPA2_LinksysE3000_2.4GHz',
                       'www.openvpn.com',
                       'certs')

HIDDEN_WEP = NetworkServices('hiddenWEP',
                             'networktest02@croste.tv',
                             'CrOS_WEP_ddwrt_54GL',
                             'www.openvpn.com',
                             'certs')

WPA2 = NetworkServices('WPA2',
                       'networktest02@croste.tv',
                       'CrOS_WPA2_LinksysE3000N_5GHz',
                       'www.openvpn.com',
                       'certs')

EAP_TTLS = NetworkServices('EAP_TTLS',
                           'networktest03@croste.tv',
                           'CrOS_WPA2_LinksysE3000_2.4GHz',
                           'www.openvpn.com',
                           'certs')

NETWORK_SERVICES_TESTS = [HIDDEN_WPA, HIDDEN_WEP, PROXY_NON_AUTH, GOOGLE_GUEST,
                          WEP, PEAP, WPA2, EAP_TTLS]
