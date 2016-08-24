#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

{
  'target_defaults': {
    'variables': {
      'deps': [
        'dbus-1',
        'libbrillo-<(libbase_ver)',
        'libchrome-<(libbase_ver)',
      ],
      'enable_exceptions': 1,
    },
    'cflags': [
      '-Wextra',
      '-Werror',
      '-Wno-unused-parameter',  # for pppd_plugin.c, base/tuple.h
    ],
    'cflags_cc': [
      '-fno-strict-aliasing',
      '-Woverloaded-virtual',
      '-Wno-missing-field-initializers',  # for LAZY_INSTANCE_INITIALIZER
    ],
    'defines': [
      'ENABLE_CHROMEOS_DBUS',
      'RUNDIR="/var/run/shill"',
      'SHIMDIR="<(libdir)/shill/shims"',
    ],
    'conditions': [
      ['USE_cellular == 0', {
        'defines': [
          'DISABLE_CELLULAR',
        ],
      }],
      ['USE_dhcpv6 == 0', {
        'defines': [
          'DISABLE_DHCPV6',
        ],
      }],
      ['USE_json_store == 1', {
        'defines': [
          'ENABLE_JSON_STORE',
        ],
      }],
      ['USE_pppoe == 0', {
        'defines': [
          'DISABLE_PPPOE',
        ],
      }],
      ['USE_vpn == 0', {
        'defines': [
          'DISABLE_VPN',
        ],
      }],
      ['USE_wake_on_wifi == 0', {
        'defines': [
          'DISABLE_WAKE_ON_WIFI',
        ],
      }],
      ['USE_wifi == 0', {
        'defines': [
          'DISABLE_WIFI',
        ],
      }],
      ['USE_wimax == 0', {
        'defines': [
          'DISABLE_WIMAX',
        ],
      }],
      ['USE_wired_8021x == 0', {
        'defines': [
          'DISABLE_WIRED_8021X',
        ],
      }],
    ],
    'include_dirs': [
      # We need this include dir because we include all the local code as
      # # "shill/...".
      '<(platform2_root)/../aosp/system/connectivity',
    ],
  },
  'includes': [
    'shill.gypi',
  ],
  'targets': [
    {
      'target_name': 'mobile_operator_db-protos',
      'type': 'static_library',
      'variables': {
        'proto_in_dir': 'mobile_operator_db',
        'proto_out_dir':
            'include/shill/mobile_operator_db'
      },
      'sources': [
        '<(proto_in_dir)/mobile_operator_db.proto'
      ],
      'includes': ['../../../../platform2/common-mk/protoc.gypi'],
    },
    {
      'target_name': 'mobile_operator_db-db',
      'type': 'none',
      'variables' : {
        'protoc_proto_dir': 'mobile_operator_db',
        'protoc_proto_def': 'mobile_operator_db.proto',
        'protoc_text_dir': 'mobile_operator_db',
        'protoc_bin_dir': '<(PRODUCT_DIR)',
        'protoc_message_name': 'shill.mobile_operator_db.MobileOperatorDB',
      },
      'sources': [
        '<(protoc_text_dir)/serviceproviders.prototxt',
      ],
      'includes': ['../../../../platform2/common-mk/protoctxt.gypi'],
    },
    {
      'target_name': 'mobile_operator_db',
      'type': 'static_library',
      'dependencies': [
        'mobile_operator_db-protos',
        'mobile_operator_db-db',
      ],
    },
    {
      'target_name': 'shill-chromeos-dbus-adaptors',
      'type': 'none',
      'variables': {
        'dbus_adaptors_out_dir': 'include/dbus_bindings',
        'dbus_xml_extension': 'dbus-xml',
      },
      'sources': [
        'dbus_bindings/org.chromium.flimflam.Device.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.IPConfig.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.Manager.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.Profile.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.Service.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.Task.dbus-xml',
        'dbus_bindings/org.chromium.flimflam.ThirdPartyVpn.dbus-xml',
      ],
      'includes': ['../../../../platform2/common-mk/generate-dbus-adaptors.gypi'],
    },
    {
      'target_name': 'shim-protos',
      'type': 'static_library',
      'variables': {
        'proto_in_dir': 'shims/protos',
        'proto_out_dir': 'include/shill/shims/protos',
      },
      'sources': [
        '<(proto_in_dir)/crypto_util.proto',
      ],
      'includes': ['../../../../platform2/common-mk/protoc.gypi'],
    },
    {
      'target_name': 'crypto_util',
      'type': 'executable',
      'dependencies': ['shim-protos'],
      'variables': {
        'deps': [
          'openssl',
          'protobuf-lite',
        ]
      },
      'sources': [
        'shims/crypto_util.cc',
      ],
    },
    {
      'target_name': 'libshill-net-<(libbase_ver)',
      'type': 'shared_library',
      'sources': [
        'net/attribute_list.cc',
        'net/byte_string.cc',
        'net/control_netlink_attribute.cc',
        'net/event_history.cc',
        'net/generic_netlink_message.cc',
        'net/io_handler_factory.cc',
        'net/io_handler_factory_container.cc',
        'net/io_input_handler.cc',
        'net/io_ready_handler.cc',
        'net/ip_address.cc',
        'net/netlink_attribute.cc',
        'net/netlink_manager.cc',
        'net/netlink_message.cc',
        'net/netlink_packet.cc',
        'net/netlink_socket.cc',
        'net/nl80211_attribute.cc',
        'net/nl80211_message.cc',
        'net/rtnl_handler.cc',
        'net/rtnl_listener.cc',
        'net/rtnl_message.cc',
        'net/shill_time.cc',
        'net/sockets.cc',
      ],
      'includes': ['../../../../platform2/common-mk/deps.gypi'],
    },
    {
      'target_name': 'libshill',
      'type': 'static_library',
      'dependencies': [
        'mobile_operator_db',
        'shill-chromeos-dbus-adaptors',
        'shim-protos',
        'libshill-net-<(libbase_ver)',
      ],
      'variables': {
        'exported_deps': [
          'libcares',
          'libmetrics-<(libbase_ver)',
          'libpermission_broker-client',
          'libpower_manager-client',
          'protobuf-lite',
        ],
        'deps': [
          '<@(exported_deps)',
          'libshill-client',
        ],
      },
      'all_dependent_settings': {
        'variables': {
          'deps': [
            '<@(exported_deps)',
          ],
        },
      },
      'link_settings': {
        'variables': {
          'deps': [
            'libcares',
            # system_api depends on protobuf (or protobuf-lite). It must appear
            # before protobuf here or the linker flags won't be in the right
            # order.
            'system_api',
            'protobuf-lite',
          ],
        },
        'libraries': [
          '-lbootstat',
          '-lrootdev',
          '-lrt'
        ],
      },
      'conditions': [
        ['USE_cellular == 1', {
          'dependencies': [
            '../../../../platform2/common-mk/external_dependencies.gyp:modemmanager-dbus-proxies',
          ],
          'variables': {
            'deps': [
              'ModemManager',
            ],
          },
          'sources': [
            'cellular/active_passive_out_of_credits_detector.cc',
            'cellular/cellular.cc',
            'cellular/cellular_bearer.cc',
            'cellular/cellular_capability.cc',
            'cellular/cellular_capability_cdma.cc',
            'cellular/cellular_capability_classic.cc',
            'cellular/cellular_capability_gsm.cc',
            'cellular/cellular_capability_universal.cc',
            'cellular/cellular_capability_universal_cdma.cc',
            'cellular/cellular_error.cc',
            'cellular/cellular_error_mm1.cc',
            'cellular/cellular_service.cc',
            'cellular/mobile_operator_info.cc',
            'cellular/mobile_operator_info_impl.cc',
            'cellular/modem.cc',
            'cellular/modem_1.cc',
            'cellular/modem_classic.cc',
            'cellular/modem_info.cc',
            'cellular/modem_manager.cc',
            'cellular/modem_manager_1.cc',
            'cellular/out_of_credits_detector.cc',
            'cellular/subscription_state_out_of_credits_detector.cc',
            'dbus/chromeos_dbus_objectmanager_proxy.cc',
            'dbus/chromeos_dbus_properties_proxy.cc',
            'dbus/chromeos_mm1_modem_modem3gpp_proxy.cc',
            'dbus/chromeos_mm1_modem_modemcdma_proxy.cc',
            'dbus/chromeos_mm1_modem_proxy.cc',
            'dbus/chromeos_mm1_modem_simple_proxy.cc',
            'dbus/chromeos_mm1_sim_proxy.cc',
            'dbus/chromeos_modem_cdma_proxy.cc',
            'dbus/chromeos_modem_gobi_proxy.cc',
            'dbus/chromeos_modem_gsm_card_proxy.cc',
            'dbus/chromeos_modem_gsm_network_proxy.cc',
            'dbus/chromeos_modem_manager_proxy.cc',
            'dbus/chromeos_modem_proxy.cc',
            'dbus/chromeos_modem_simple_proxy.cc',
            'protobuf_lite_streams.cc',
          ],
          'actions': [
            {
              'action_name': 'generate-cellular-proxies',
              'variables': {
                'proxy_output_file': 'include/cellular/dbus-proxies.h',
                'modemmanager_in_dir': '<(sysroot)/usr/share/dbus-1/interfaces/',
              },
              'sources': [
                'dbus_bindings/dbus-objectmanager.dbus-xml',
                'dbus_bindings/dbus-properties.dbus-xml',
                'dbus_bindings/modem-gobi.dbus-xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.Modem.Cdma.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.Modem.Gsm.Card.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.Modem.Gsm.Network.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.Modem.Simple.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.Modem.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager1.Modem.Modem3gpp.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager1.Modem.ModemCdma.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager1.Modem.Simple.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager1.Modem.xml',
                '<(modemmanager_in_dir)/org.freedesktop.ModemManager1.Sim.xml',
              ],
              'includes': ['../../../../platform2/common-mk/generate-dbus-proxies.gypi'],
            },
          ],
        }],
        ['USE_json_store == 0', {
          'link_settings': {
            'variables': {
              'deps': [
                'gio-2.0',  # for g_type_init()
                'glib-2.0',  # for g_key_*(), etc.
              ],
            },
          },
          'sources': [
            'key_file_store.cc',
          ],
          'variables': {
            'exported_deps': [
              'gio-2.0',  # for g_type_init()
              'glib-2.0',  # for g_key_*(), etc.
            ],
            'deps': ['<@(exported_deps)'],
          },
        }],
        ['USE_json_store == 1', {
          'sources': [
            'json_store.cc',
          ],
        }],
        ['USE_vpn == 1', {
          'sources': [
            'vpn/l2tp_ipsec_driver.cc',
            'vpn/openvpn_driver.cc',
            'vpn/openvpn_management_server.cc',
            'vpn/third_party_vpn_driver.cc',
          ],
        }],
        ['USE_wifi == 1', {
          'sources': [
            'wifi/callback80211_metrics.cc',
            'wifi/mac80211_monitor.cc',
            'wifi/scan_session.cc',
            'wifi/tdls_manager.cc',
            'wifi/wake_on_wifi.cc',
            'wifi/wifi.cc',
            'wifi/wifi_endpoint.cc',
            'wifi/wifi_provider.cc',
            'wifi/wifi_service.cc',
          ],
        }],
        ['USE_wifi == 1 or USE_wired_8021x == 1', {
          'sources': [
            'dbus/chromeos_supplicant_bss_proxy.cc',
            'dbus/chromeos_supplicant_interface_proxy.cc',
            'dbus/chromeos_supplicant_network_proxy.cc',
            'dbus/chromeos_supplicant_process_proxy.cc',
            'eap_credentials.cc',
            'eap_listener.cc',
            'supplicant/supplicant_eap_state_handler.cc',
            'supplicant/wpa_supplicant.cc',
          ],
          'actions': [
            {
              'action_name': 'generate-supplicant-proxies',
              'variables': {
                'proxy_output_file': 'include/supplicant/dbus-proxies.h',
              },
              'sources': [
                'dbus_bindings/supplicant-bss.dbus-xml',
                'dbus_bindings/supplicant-interface.dbus-xml',
                'dbus_bindings/supplicant-network.dbus-xml',
                'dbus_bindings/supplicant-process.dbus-xml',
              ],
              'includes': ['../../../../platform2/common-mk/generate-dbus-proxies.gypi'],
            },
          ],
        }],
        ['USE_wimax == 1', {
          'variables': {
            'exported_deps': [
              'libwimax_manager-client',
            ],
            'deps': ['<@(exported_deps)'],
          },
          'all_dependent_settings': {
            'variables': {
              'deps': [
                '<@(exported_deps)',
              ],
            },
          },
          'sources': [
            'dbus/chromeos_wimax_device_proxy.cc',
            'dbus/chromeos_wimax_manager_proxy.cc',
            'dbus/chromeos_wimax_network_proxy.cc',
            'wimax/wimax.cc',
            'wimax/wimax_provider.cc',
            'wimax/wimax_service.cc',
          ],
        }],
        ['USE_wired_8021x == 1', {
          'sources': [
            'ethernet/ethernet_eap_provider.cc',
            'ethernet/ethernet_eap_service.cc',
          ],
        }],
        ['USE_dhcpv6 == 1', {
          'sources': [
            'dhcp/dhcpv6_config.cc',
          ],
        }],
      ],
      'sources': [
        'active_link_monitor.cc',
        'arp_client.cc',
        'arp_packet.cc',
        'async_connection.cc',
        'certificate_file.cc',
        'connection.cc',
        'connection_diagnostics.cc',
        'connection_health_checker.cc',
        'connection_info.cc',
        'connection_info_reader.cc',
        'connection_tester.cc',
        'connectivity_trial.cc',
        'crypto_des_cbc.cc',
        'crypto_provider.cc',
        'crypto_rot47.cc',
        'crypto_util_proxy.cc',
        'daemon_task.cc',
        'dbus/chromeos_dbus_adaptor.cc',
        'dbus/chromeos_dbus_control.cc',
        'dbus/chromeos_dbus_service_watcher.cc',
        'dbus/chromeos_device_dbus_adaptor.cc',
        'dbus/chromeos_dhcpcd_listener.cc',
        'dbus/chromeos_dhcpcd_proxy.cc',
        'dbus/chromeos_ipconfig_dbus_adaptor.cc',
        'dbus/chromeos_manager_dbus_adaptor.cc',
        'dbus/chromeos_permission_broker_proxy.cc',
        'dbus/chromeos_power_manager_proxy.cc',
        'dbus/chromeos_profile_dbus_adaptor.cc',
        'dbus/chromeos_rpc_task_dbus_adaptor.cc',
        'dbus/chromeos_service_dbus_adaptor.cc',
        'dbus/chromeos_third_party_vpn_dbus_adaptor.cc',
        'dbus/chromeos_upstart_proxy.cc',
        'dbus/dbus_service_watcher_factory.cc',
        'default_profile.cc',
        'device.cc',
        'device_claimer.cc',
        'device_info.cc',
        'dhcp_properties.cc',
        'dhcp/dhcp_config.cc',
        'dhcp/dhcp_provider.cc',
        'dhcp/dhcpv4_config.cc',
        'dns_client.cc',
        'dns_client_factory.cc',
        'dns_server_tester.cc',
        'ephemeral_profile.cc',
        'error.cc',
        'ethernet/ethernet.cc',
        'ethernet/ethernet_service.cc',
        'ethernet/ethernet_temporary_service.cc',
        'ethernet/virtio_ethernet.cc',
        'event_dispatcher.cc',
        'external_task.cc',
        'file_io.cc',
        'file_reader.cc',
        'geolocation_info.cc',
        'hook_table.cc',
        'http_proxy.cc',
        'http_request.cc',
        'http_url.cc',
        'icmp.cc',
        'icmp_session.cc',
        'icmp_session_factory.cc',
        'ip_address_store.cc',
        'ipconfig.cc',
        'key_value_store.cc',
        'link_monitor.cc',
        'logging.cc',
        'manager.cc',
        'metrics.cc',
        'passive_link_monitor.cc',
        'pending_activation_store.cc',
        'portal_detector.cc',
        'power_manager.cc',
        'ppp_daemon.cc',
        'ppp_device.cc',
        'ppp_device_factory.cc',
        'pppoe/pppoe_service.cc',
        'process_manager.cc',
        'profile.cc',
        'property_store.cc',
        'resolver.cc',
        'result_aggregator.cc',
        'routing_table.cc',
        'rpc_task.cc',
        'scope_logger.cc',
        'scoped_umask.cc',
        'service.cc',
        'service_property_change_notifier.cc',
        'shill_ares.cc',
        'shill_config.cc',
        'shill_daemon.cc',
        'shill_test_config.cc',
        'socket_info.cc',
        'socket_info_reader.cc',
        'static_ip_parameters.cc',
        'store_factory.cc',
        'technology.cc',
        'tethering.cc',
        'traffic_monitor.cc',
        'upstart/upstart.cc',
        'virtual_device.cc',
        'vpn/vpn_driver.cc',
        'vpn/vpn_provider.cc',
        'vpn/vpn_service.cc',
      ],
      'actions': [
        {
          'action_name': 'generate-dhcpcd-proxies',
          'variables': {
            'proxy_output_file': 'include/dhcpcd/dbus-proxies.h',
          },
          'sources': [
            'dbus_bindings/dhcpcd.dbus-xml',
          ],
          'includes': ['../../../../platform2/common-mk/generate-dbus-proxies.gypi'],
        },
        {
          'action_name': 'generate-upstart-proxies',
          'variables': {
            'proxy_output_file': 'include/upstart/dbus-proxies.h',
          },
          'sources': [
            'dbus_bindings/upstart.dbus-xml',
          ],
          'includes': ['../../../../platform2/common-mk/generate-dbus-proxies.gypi'],
        },
      ],
    },
    {
      'target_name': 'shill',
      'type': 'executable',
      'dependencies': ['libshill'],
      'sources': [
        'shill_main.cc',
      ]
    },
    {
      'target_name': 'crypto-util',
      'type': 'executable',
      'dependencies': ['shim-protos'],
      'variables': {
        'deps': [
          'openssl',
          'protobuf-lite',
        ],
      },
      'sources': [
        'shims/crypto_util.cc',
      ]
    },
    {
      'target_name': 'netfilter-queue-helper',
      'type': 'executable',
      'variables': {
        'deps': [
          'libnetfilter_queue',
          'libnfnetlink',
        ],
      },
      'sources': [
        'shims/netfilter_queue_helper.cc',
        'shims/netfilter_queue_processor.cc',
      ]
    },
    {
      'target_name': 'openvpn-script',
      'type': 'executable',
      'variables': {
        'deps': [
          'libshill-client',
        ],
      },
      'sources': [
        'shims/environment.cc',
        'shims/openvpn_script.cc',
        'shims/task_proxy.cc',
      ]
    },
  ],
  'conditions': [
    ['USE_cellular == 1', {
      'targets': [
        {
          'target_name': 'set-apn-helper',
          'type': 'executable',
          'variables': {
            'deps': [
              'dbus-glib-1'
            ]
          },
          'sources': [
            'shims/set_apn_helper.c',
          ],
        },
      ],
    }],
    ['USE_cellular == 1 or USE_vpn == 1 or USE_pppoe == 1', {
      'targets': [
        {
          'target_name': 'shill-pppd-plugin',
          'type': 'shared_library',
          'variables': {
            'deps': [
              'libshill-client',
            ],
          },
          'sources': [
            'shims/c_ppp.cc',
            'shims/environment.cc',
            'shims/ppp.cc',
            'shims/pppd_plugin.c',
            'shims/task_proxy.cc',
          ],
        },
      ],
    }],
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'shill_unittest',
          'type': 'executable',
          'dependencies': [
            'libshill',
          ],
          'includes': ['../../../../platform2/common-mk/common_test.gypi'],
          'variables': {
            'deps': [
              'libchrome-test-<(libbase_ver)',
              'libnetfilter_queue',
              'libnfnetlink',
            ],
          },
          'defines': [
            'SYSROOT="<(sysroot)"',
          ],
          'sources': [
            'active_link_monitor_unittest.cc',
            'arp_client_test_helper.cc',
            'arp_client_unittest.cc',
            'arp_packet_unittest.cc',
            'async_connection_unittest.cc',
            'certificate_file_unittest.cc',
            'connection_diagnostics_unittest.cc',
            'connection_health_checker_unittest.cc',
            'connection_info_reader_unittest.cc',
            'connection_info_unittest.cc',
            'connection_tester_unittest.cc',
            'connection_unittest.cc',
            'connectivity_trial_unittest.cc',
            'crypto_des_cbc_unittest.cc',
            'crypto_provider_unittest.cc',
            'crypto_rot47_unittest.cc',
            'crypto_util_proxy_unittest.cc',
            'daemon_task_unittest.cc',
            'dbus/chromeos_dbus_adaptor_unittest.cc',
            'dbus/chromeos_manager_dbus_adaptor_unittest.cc',
            'default_profile_unittest.cc',
            'device_claimer_unittest.cc',
            'device_info_unittest.cc',
            'device_unittest.cc',
            'dhcp/dhcp_config_unittest.cc',
            'dhcp/dhcp_provider_unittest.cc',
            'dhcp/dhcpv4_config_unittest.cc',
            'dhcp/mock_dhcp_config.cc',
            'dhcp/mock_dhcp_provider.cc',
            'dhcp/mock_dhcp_proxy.cc',
            'dhcp_properties_unittest.cc',
            'dns_client_unittest.cc',
            'dns_server_tester_unittest.cc',
            'error_unittest.cc',
            'ethernet/ethernet_service_unittest.cc',
            'ethernet/ethernet_unittest.cc',
            'ethernet/mock_ethernet.cc',
            'ethernet/mock_ethernet_service.cc',
            'external_task_unittest.cc',
            'fake_store.cc',
            'file_reader_unittest.cc',
            'hook_table_unittest.cc',
            'http_proxy_unittest.cc',
            'http_request_unittest.cc',
            'http_url_unittest.cc',
            'icmp_unittest.cc',
            'icmp_session_unittest.cc',
            'ip_address_store_unittest.cc',
            'ipconfig_unittest.cc',
            'key_value_store_unittest.cc',
            'link_monitor_unittest.cc',
            'manager_unittest.cc',
            'metrics_unittest.cc',
            'mock_active_link_monitor.cc',
            'mock_adaptors.cc',
            'mock_ares.cc',
            'mock_arp_client.cc',
            'mock_async_connection.cc',
            'mock_certificate_file.cc',
            'mock_connection.cc',
            'mock_connection_health_checker.cc',
            'mock_connection_info_reader.cc',
            'mock_connectivity_trial.cc',
            'mock_control.cc',
            'mock_crypto_util_proxy.cc',
            'mock_device.cc',
            'mock_device_claimer.cc',
            'mock_device_info.cc',
	    'mock_dhcp_properties.cc',
            'mock_dns_client.cc',
            'mock_dns_client_factory.cc',
            'mock_dns_server_tester.cc',
            'mock_event_dispatcher.cc',
            'mock_external_task.cc',
            'mock_http_request.cc',
            'mock_icmp.cc',
            'mock_icmp_session.cc',
            'mock_icmp_session_factory.cc',
            'mock_ip_address_store.cc',
            'mock_ipconfig.cc',
            'mock_link_monitor.cc',
            'mock_log.cc',
            'mock_log_unittest.cc',
            'mock_manager.cc',
            'mock_metrics.cc',
            'mock_passive_link_monitor.cc',
            'mock_pending_activation_store.cc',
            'mock_portal_detector.cc',
            'mock_power_manager.cc',
            'mock_power_manager_proxy.cc',
            'mock_ppp_device.cc',
            'mock_ppp_device_factory.cc',
            'mock_process_manager.cc',
            'mock_profile.cc',
            'mock_property_store.cc',
            'mock_resolver.cc',
            'mock_routing_table.cc',
            'mock_service.cc',
            'mock_socket_info_reader.cc',
            'mock_store.cc',
            'mock_traffic_monitor.cc',
            'mock_virtual_device.cc',
            'net/attribute_list_unittest.cc',
            'net/byte_string_unittest.cc',
            'net/event_history_unittest.cc',
            'net/ip_address_unittest.cc',
            'net/netlink_attribute_unittest.cc',
            'net/rtnl_handler_unittest.cc',
            'net/rtnl_listener_unittest.cc',
            'net/rtnl_message_unittest.cc',
            'net/shill_time_unittest.cc',
            'nice_mock_control.cc',
            'passive_link_monitor_unittest.cc',
            'pending_activation_store_unittest.cc',
            'portal_detector_unittest.cc',
            'power_manager_unittest.cc',
            'ppp_daemon_unittest.cc',
            'ppp_device_unittest.cc',
            'pppoe/pppoe_service_unittest.cc',
            'process_manager_unittest.cc',
            'profile_unittest.cc',
            'property_accessor_unittest.cc',
            'property_observer_unittest.cc',
            'property_store_unittest.cc',
            'resolver_unittest.cc',
            'result_aggregator_unittest.cc',
            'routing_table_unittest.cc',
            'rpc_task_unittest.cc',
            'scope_logger_unittest.cc',
            'service_property_change_test.cc',
            'service_under_test.cc',
            'service_unittest.cc',
            'shims/netfilter_queue_processor.cc',
            'shims/netfilter_queue_processor_unittest.cc',
            'socket_info_reader_unittest.cc',
            'socket_info_unittest.cc',
            'static_ip_parameters_unittest.cc',
            'technology_unittest.cc',
            'testrunner.cc',
            'traffic_monitor_unittest.cc',
            'upstart/mock_upstart.cc',
            'upstart/mock_upstart_proxy.cc',
            'upstart/upstart_unittest.cc',
            'virtual_device_unittest.cc',
            'vpn/mock_vpn_provider.cc',
          ],
          'conditions': [
            ['USE_cellular == 1', {
              'variables': {
                'deps': [
                  'ModemManager',
                ],
              },
              'sources': [
                'cellular/active_passive_out_of_credits_detector_unittest.cc',
                'cellular/cellular_bearer_unittest.cc',
                'cellular/cellular_capability_cdma_unittest.cc',
                'cellular/cellular_capability_classic_unittest.cc',
                'cellular/cellular_capability_gsm_unittest.cc',
                'cellular/cellular_capability_universal_cdma_unittest.cc',
                'cellular/cellular_capability_universal_unittest.cc',
                'cellular/cellular_error_unittest.cc',
                'cellular/cellular_service_unittest.cc',
                'cellular/cellular_unittest.cc',
                'cellular/mobile_operator_info_unittest.cc',
                'cellular/mock_cellular.cc',
                'cellular/mock_cellular_service.cc',
                'cellular/mock_dbus_objectmanager_proxy.cc',
                'cellular/mock_mm1_modem_modem3gpp_proxy.cc',
                'cellular/mock_mm1_modem_modemcdma_proxy.cc',
                'cellular/mock_mm1_modem_proxy.cc',
                'cellular/mock_mm1_modem_simple_proxy.cc',
                'cellular/mock_mm1_sim_proxy.cc',
                'cellular/mock_mobile_operator_info.cc',
                'cellular/mock_modem.cc',
                'cellular/mock_modem_cdma_proxy.cc',
                'cellular/mock_modem_gobi_proxy.cc',
                'cellular/mock_modem_gsm_card_proxy.cc',
                'cellular/mock_modem_gsm_network_proxy.cc',
                'cellular/mock_modem_info.cc',
                'cellular/mock_modem_manager_proxy.cc',
                'cellular/mock_modem_proxy.cc',
                'cellular/mock_modem_simple_proxy.cc',
                'cellular/mock_out_of_credits_detector.cc',
                'cellular/modem_1_unittest.cc',
                'cellular/modem_info_unittest.cc',
                'cellular/modem_manager_unittest.cc',
                'cellular/modem_unittest.cc',
                'cellular/subscription_state_out_of_credits_detector_unittest.cc',
                'mock_dbus_properties_proxy.cc',
              ],
            }],
            ['USE_dhcpv6 == 1', {
              'sources': [
                'dhcp/dhcpv6_config_unittest.cc',
              ],
            }],
            ['USE_json_store == 0', {
              'sources': [
                'key_file_store_unittest.cc',
              ],
            }],
            ['USE_json_store == 1', {
              'sources': [
                'json_store_unittest.cc',
              ],
            }],
            ['USE_vpn == 1', {
              'sources': [
                'shims/environment.cc',
                'shims/environment_unittest.cc',
                'vpn/l2tp_ipsec_driver_unittest.cc',
                'vpn/mock_openvpn_driver.cc',
                'vpn/mock_openvpn_management_server.cc',
                'vpn/mock_vpn_driver.cc',
                'vpn/mock_vpn_service.cc',
                'vpn/openvpn_driver_unittest.cc',
                'vpn/openvpn_management_server_unittest.cc',
                'vpn/third_party_vpn_driver_unittest.cc',
                'vpn/vpn_driver_unittest.cc',
                'vpn/vpn_provider_unittest.cc',
                'vpn/vpn_service_unittest.cc',
              ],
            }],
            ['USE_wifi == 1', {
              'sources': [
                'net/netlink_manager_unittest.cc',
                'net/netlink_message_unittest.cc',
                'net/netlink_packet_unittest.cc',
                'net/netlink_socket_unittest.cc',
                'net/nl80211_attribute_unittest.cc',
                'supplicant/mock_supplicant_bss_proxy.cc',
                'wifi/callback80211_metrics_unittest.cc',
                'wifi/mac80211_monitor_unittest.cc',
                'wifi/mock_mac80211_monitor.cc',
                'wifi/mock_scan_session.cc',
                'wifi/mock_tdls_manager.cc',
                'wifi/mock_wake_on_wifi.cc',
                'wifi/mock_wifi.cc',
                'wifi/mock_wifi_provider.cc',
                'wifi/mock_wifi_service.cc',
                'wifi/scan_session_unittest.cc',
                'wifi/tdls_manager_unittest.cc',
                'wifi/wake_on_wifi_unittest.cc',
                'wifi/wifi_endpoint_unittest.cc',
                'wifi/wifi_provider_unittest.cc',
                'wifi/wifi_service_unittest.cc',
                'wifi/wifi_unittest.cc',
              ],
            }],
            ['USE_wifi == 1 or USE_wired_8021x == 1', {
              'sources': [
                'eap_credentials_unittest.cc',
                'eap_listener_unittest.cc',
                'mock_eap_credentials.cc',
                'mock_eap_listener.cc',
                'supplicant/mock_supplicant_eap_state_handler.cc',
                'supplicant/mock_supplicant_interface_proxy.cc',
                'supplicant/mock_supplicant_network_proxy.cc',
                'supplicant/mock_supplicant_process_proxy.cc',
                'supplicant/supplicant_eap_state_handler_unittest.cc',
                'supplicant/wpa_supplicant_unittest.cc',
              ],
            }],
            ['USE_wimax == 1', {
              'sources': [
                'wimax/mock_wimax.cc',
                'wimax/mock_wimax_device_proxy.cc',
                'wimax/mock_wimax_manager_proxy.cc',
                'wimax/mock_wimax_network_proxy.cc',
                'wimax/mock_wimax_provider.cc',
                'wimax/mock_wimax_service.cc',
                'wimax/wimax_provider_unittest.cc',
                'wimax/wimax_service_unittest.cc',
                'wimax/wimax_unittest.cc',
              ],
            }],
            ['USE_wired_8021x == 1', {
              'sources': [
                'ethernet/ethernet_eap_provider_unittest.cc',
                'ethernet/ethernet_eap_service_unittest.cc',
                'ethernet/mock_ethernet_eap_provider.cc',
              ],
            }],
          ],
        },
        {
          'target_name': 'shill_setup_wifi',
          'type': 'executable',
          'variables': {
            'deps': [
              'libshill-client',
            ],
          },
          'sources': [
            'setup_wifi/main.cc'
          ],
        },
      ],
    }],
  ],
}
