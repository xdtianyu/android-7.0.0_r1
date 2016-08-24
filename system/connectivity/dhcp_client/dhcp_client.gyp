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
        'libchrome-<(libbase_ver)',
        'libbrillo-<(libbase_ver)',
        'libshill-net-<(libbase_ver)',
      ],
    },
    'cflags': [
      '-Wextra',
      '-Wno-unused-parameter',  # base/lazy_instance.h, etc.
    ],
    'cflags_cc': [
      '-fno-strict-aliasing',
      '-Wno-missing-field-initializers', # for LAZY_INSTANCE_INITIALIZER
      '-Wno-unused-const-variable',
    ],
    'include_dirs': [
      # We need this include dir because we include all the local code as
      # "dhcp_client/...".
      '<(platform2_root)/../aosp/system/connectivity',
    ],
  },

  'targets': [
    {
      'target_name': 'libdhcp_client',
      'type': 'static_library',
      'variables': {
        'exported_deps': [
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
        'daemon.cc',
        'device_info.cc',
        'dhcp_message.cc',
        'dhcp_options_parser.cc',
        'dhcp_options_writer.cc',
        'dhcpv4.cc',
        'message_loop_event_dispatcher.cc',
        'manager.cc',
        'service.cc',
      ],
    },
    {
      'target_name': 'dhcp_client',
      'type': 'executable',
      'dependencies': ['libdhcp_client'],
      'sources': [
        'main.cc',
      ],
    },
  ],
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'dhcp_client_testrunner',
          'type': 'executable',
          'dependencies': ['libdhcp_client'],
          'includes': ['../../../../platform2/common-mk/common_test.gypi'],
          'sources': [
            'device_info_unittest.cc',
            'dhcp_message_unittest.cc',
            'dhcp_options_parser_unittest.cc',
            'dhcp_options_writer_unittest.cc',
            'testrunner.cc',
          ],
        },
      ],
    }],
  ],
}

