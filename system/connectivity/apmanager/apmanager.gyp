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
        'libbrillo-<(libbase_ver)',
        'libchrome-<(libbase_ver)',
      ],
    },
    'cflags': [
      '-Wextra',
      '-Wno-unused-parameter',  # base/lazy_instance.h, etc.
    ],
    'cflags_cc': [
      '-Wno-missing-field-initializers', # for LAZY_INSTANCE_INITIALIZER
    ],
    'include_dirs': [
      # We need this include dir because we include all the local code as
      # "apmanager/...".
      '<(platform2_root)/../aosp/system/connectivity',
    ],
  },
  'targets': [
    {
      'target_name': 'apmanager-adaptors',
      'type': 'none',
      'variables': {
        'dbus_adaptors_out_dir': 'include/dbus_bindings',
        'dbus_xml_extension': 'dbus-xml',
      },
      'sources': [
        'dbus_bindings/org.chromium.apmanager.Config.dbus-xml',
        'dbus_bindings/org.chromium.apmanager.Device.dbus-xml',
        'dbus_bindings/org.chromium.apmanager.Manager.dbus-xml',
        'dbus_bindings/org.chromium.apmanager.Service.dbus-xml',
      ],
      'includes': ['../../../../platform2/common-mk/generate-dbus-adaptors.gypi'],
    },
    {
      'target_name': 'libapmanager',
      'type': 'static_library',
      'dependencies': [
        'apmanager-adaptors',
      ],
      'variables': {
        'exported_deps': [
          'libpermission_broker-client',
          'libshill-client',
          'libshill-net-<(libbase_ver)',
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
        'config.cc',
        'daemon.cc',
        'dbus/config_dbus_adaptor.cc',
        'dbus/dbus_control.cc',
        'dbus/device_dbus_adaptor.cc',
        'dbus/manager_dbus_adaptor.cc',
        'dbus/permission_broker_dbus_proxy.cc',
        'dbus/service_dbus_adaptor.cc',
        'dbus/shill_dbus_proxy.cc',
        'device.cc',
        'device_info.cc',
        'dhcp_server.cc',
        'dhcp_server_factory.cc',
        'error.cc',
        'event_dispatcher.cc',
        'file_writer.cc',
        'firewall_manager.cc',
        'hostapd_monitor.cc',
        'manager.cc',
        'process_factory.cc',
        'service.cc',
        'shill_manager.cc',
      ],
    },
    {
      'target_name': 'apmanager',
      'type': 'executable',
      'dependencies': ['libapmanager'],
      'variables': {
        'deps': [
          'libminijail',
        ],
      },
      'sources': [
        'main.cc',
      ],
    },
    # apmanager client library generated headers. Used by other daemons to
    # interact with apmanager.
    {
      'target_name': 'libapmanager-client-headers',
      'type': 'none',
      'actions': [
        {
          'action_name': 'libapmanager-client-dbus-proxies',
          'variables': {
            'dbus_service_config': 'dbus_bindings/dbus-service-config.json',
            'proxy_output_file': 'include/apmanager/dbus-proxies.h',
            'mock_output_file': 'include/apmanager/dbus-proxy-mocks.h',
            'proxy_path_in_mocks': 'apmanager/dbus-proxies.h',
          },
          'sources': [
            'dbus_bindings/org.chromium.apmanager.Config.dbus-xml',
            'dbus_bindings/org.chromium.apmanager.Device.dbus-xml',
            'dbus_bindings/org.chromium.apmanager.Manager.dbus-xml',
            'dbus_bindings/org.chromium.apmanager.Service.dbus-xml',
          ],
          'includes': ['../../../../platform2/common-mk/generate-dbus-proxies.gypi'],
        },
      ]
    },
  ],
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'apmanager_testrunner',
          'type': 'executable',
          'dependencies': ['libapmanager'],
          'includes': ['../../../../platform2/common-mk/common_test.gypi'],
          'sources': [
            'config_unittest.cc',
            'device_info_unittest.cc',
            'device_unittest.cc',
            'dhcp_server_unittest.cc',
            'error_unittest.cc',
            'fake_config_adaptor.cc',
            'fake_device_adaptor.cc',
            'hostapd_monitor_unittest.cc',
            'manager_unittest.cc',
            'mock_config.cc',
            'mock_control.cc',
            'mock_device.cc',
            'mock_dhcp_server.cc',
            'mock_dhcp_server_factory.cc',
            'mock_event_dispatcher.cc',
            'mock_file_writer.cc',
            'mock_hostapd_monitor.cc',
            'mock_manager.cc',
            'mock_process_factory.cc',
            'mock_service.cc',
            'mock_service_adaptor.cc',
            'service_unittest.cc',
            'testrunner.cc',
          ],
        },
      ],
    }],
  ],
}
