#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Configuration for cell emulator tests."""
import copy, unittest

CELLS = {}

# TODO(rochberg):  Need some way to subset this list for long/short tests

LTE_TECHNOLOGIES = ['LTE']
GENERIC_GSM_TECHNOLOGIES = ['GPRS', 'EGPRS', 'WCDMA', 'HSDPA', 'HSUPA',
                            'HSDUPA', 'HSPA_PLUS']

ICERA_TECHNOLOGIES = GENERIC_GSM_TECHNOLOGIES[:]
ICERA_TECHNOLOGIES.remove('HSPA_PLUS')

GOBI_3000_TECHNOLOGIES = GENERIC_GSM_TECHNOLOGIES + ['CDMA_2000', 'EVDO_1X']

GOBI_2000_TECHNOLOGIES = GOBI_3000_TECHNOLOGIES[:]
GOBI_2000_TECHNOLOGIES.remove('HSPA_PLUS')

# TODO(thieule): Make HSPA_PLUS work with autotest (crosbug.com/32621).
GENERIC_GSM_TECHNOLOGIES.remove('HSPA_PLUS')
GOBI_3000_TECHNOLOGIES.remove('HSPA_PLUS')

def combine_trees(a_original, b):
    """Combines two dict-of-dict trees, favoring the second."""
    try:
        a = copy.copy(a_original)
        for (key_b, value_b) in b.iteritems():
            a[key_b] = combine_trees(a.get(key_b, None), value_b)
    except AttributeError:  # one argument wasn't a dict.  B wins.
        return b
    return a


def MakeDefaultCallBoxConfig(specifics):
    base = {
            "type": "8960-prologix",
            # IP addresses and netmask for the air-side of the
            # basestation network.
            "bs_addresses": [
                "192.168.2.2",
                "192.168.2.3"
                ],
            "bs_netmask": "255.255.0.0",

            "gpib_adapter": {
                "gpib_address": 14,
                "ip_port": 1234
                },
            # DNS addresses for the UE.  You do not need a
            # working DNS server at this address, but you must
            # have a machine there to send ICMP Port
            # Unreachable messages, so the DNS lookups will
            # fail quickly)
            "ue_dns_addresses": [
                "192.168.2.254",
                "192.168.2.254"
                ],
            "ue_rf_addresses": [
                "192.168.2.4",
                "192.168.2.5"
                ]
            }
    return combine_trees(base, specifics)

def MakeDefaultPerfServer(specifics):
    rf_address = "192.168.2.254"
    base = {
        "rf_address": rf_address,
        "upload_url": "http://%s/upload" % (rf_address),
        "download_url_format_string": ("http://%s/download?size=%%(size)s" %
                                       rf_address),
        }
    return combine_trees(base, specifics)


CELLS['cam'] = {
    "basestations": [
        MakeDefaultCallBoxConfig({
            "gpib_adapter": {
                "address": "172.31.206.171",
                },
            })
        ],
    "duts": [
        {
            "address": "172.31.206.145",
            "name": "ad-hoc-usb",
            "technologies": GOBI_2000_TECHNOLOGIES,
            "rf_switch_port": 3,
            },
        {
            "address": "172.31.206.146",
            "name": "y3300",
            "technologies": GENERIC_GSM_TECHNOLOGIES,
            "rf_switch_port": 0,
            }
        ],

    "perfserver": MakeDefaultPerfServer({
        "name": "perfserver-cam",
        "address": "172.31.206.153",
        "ethernet_mac": "e8:11:32:cb:bb:95 ",
        }),

    "http_connectivity": {
        # "url" should point to a URL that fetches a page small enough
        # to be comfortably kept in memory.  If
        # "url_required_contents" is present, it points to a string
        # that must be present in the the fetched data.

        "url": "http://192.168.2.254/connectivity/index.html",
        "url_required_contents": "Chromium",
        },
    "rf_switch": {
        "type": "ether_io",
        "address":  "172.31.206.172",
        "ethernet_mac": "00:11:ba:02:12:83",
        }
    }

CELLS['mtv'] = {
    "basestations": [
        MakeDefaultCallBoxConfig({
            "gpib_adapter": {
              "type":'8960',
              "address": "172.22.50.118",
              "ethernet_mac": "00:21:69:01:06:46",
              }
            }),
        MakeDefaultCallBoxConfig({
          "type":'pxt',
          "gpib_adapter": {
              "address": "172.22.50.244",
              "ethernet_mac": "00:21:69:01:0a:11",
              # ddns-hostname "chromeos1-rack1-pxt-gpib";
            }
        })
        ],


#chromeos1-rack1-pxt / 172.22.50.243
#chromeos1-rack2-rfswitch2 / 172.22.50.229
#pixel 172.22.50.86 chromeos1-rack2-host6

    "duts": [
         {
             "address": "172.22.50.86",
             "ethernet_mac": "00:0e:c6:89:9d:18",
             "name": "link-lte",
             "technologies": LTE_TECHNOLOGIES,
             "location": "rack2-host6",
             "rf_switch_port": 1,
             },
        {
            "address": "172.22.50.187",
            "ethernet_mac": "00:00:00:00:08:4b",
            "name": "alex-gobi-2000",
            "technologies": GOBI_2000_TECHNOLOGIES,
            "location": "rack2-host0",
            "rf_switch_port": 0,
            },
        {
            "address": "172.22.50.85",
            "ethernet_mac": "00:00:00:00:00:c8",
            "name": "alex-gobi-3000",
            "technologies": GOBI_3000_TECHNOLOGIES,
            "location": "rack2-host4",
            "rf_switch_port": 1,
            },
        {
            "address": "172.22.50.191",
            "ethernet_mac": "c0:c1:c0:4b:d7:4f",
            "name": "alex-y3300",
            "technologies": ICERA_TECHNOLOGIES,
            "location": "rack2-host1",
            "rf_switch_port": 3,
            },
        {
            "address": "172.22.50.89",
            "ethernet_mac": "58:6d:8f:50:ae:55",
            "name": "alex-y3400",
            "technologies": ICERA_TECHNOLOGIES,
            "location": "rack2-host5",
            "rf_switch_port": 2,
            },
        ],

    "perfserver": MakeDefaultPerfServer({
        "name": "perfserver-mtv",
        "address": "172.22.50.246",
        "ethernet_mac": "c4:54:44:2a:1a:8b",
        }),

    # Used for tests that check web connectivity
    "http_connectivity": {
        "url": "http://192.168.2.254/connectivity/index.html",
        "url_required_contents": "Chromium",
        },
    "rf_switch": {
        "type": "ether_io",
        "name": "rf-switch-1-mtv",
        "ethernet_mac": "00:11:BA:02:12:82",
        "address":  "172.22.50.88",
        }
    }


class TestCombineTrees(unittest.TestCase):
    def test_simple(self):
        self.assertEqual({1:2, 3:4, 5:6},
                         combine_trees({1:2, 3:4}, {5:6}))

    def test_override_simple(self):
        self.assertEqual({1:3},
                         combine_trees({1:2},{1:3}))

    def test_join_nested(self):
        self.assertEqual({1:{2:3, 3:4}},
                         combine_trees({1:{2:3}},{1:{3:4}}))

    def test_override_in_nested(self):
        self.assertEqual({1:{2:4}},
                         combine_trees({1:{2:3}},{1:{2:4}}))

    def test_override_different_types(self):
        self.assertEqual({1:{2:4}},
                         combine_trees({1:'rhinoceros'},{1:{2:4}}))
        self.assertEqual({1:'rhinoceros'},
                         combine_trees({1:{2:4}},{1:'rhinoceros'}))

    def test_two_level(self):
        self.assertEqual({1:{2:{3:4, 5:6}}},
                         combine_trees({1:{2:{3:4}}},{1:{2:{5:6}}}))

    def test_none(self):
        self.assertEqual({1:None},
                         combine_trees({1:2}, {1:None}))
        self.assertEqual({1:None},
                         combine_trees({1:None}, {}))
        self.assertEqual({1:2},
                         combine_trees({1:None}, {1:2}))


if __name__ == '__main__':
    unittest.main()
