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
      '-Wno-unused-parameter',  # for scoped_ptr.h, included indirectly
    ],
    'cflags_cc': [
      '-fno-strict-aliasing',
      '-Woverloaded-virtual',
    ],
    'include_dirs': ['..'],
  },
  'targets': [
    {
      'target_name': 'libchromeos-dbus-bindings',
      'type': 'static_library',
      'sources': [
        'adaptor_generator.cc',
        'dbus_signature.cc',
        'header_generator.cc',
        'indented_text.cc',
        'method_name_generator.cc',
        'name_parser.cc',
        'proxy_generator.cc',
        'xml_interface_parser.cc',
      ],
      'variables': {
        'exported_deps': [
          'expat',
        ],
        'deps': [
          'dbus-1',
          '<@(exported_deps)',
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
            'expat',
          ],
        },
      },
    },
    {
      'target_name': 'generate-chromeos-dbus-bindings',
      'type': 'executable',
      'dependencies': ['libchromeos-dbus-bindings'],
      'sources': [
        'generate_chromeos_dbus_bindings.cc',
      ]
    },
  ],
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'chromeos_dbus_bindings_unittest',
          'type': 'executable',
          'dependencies': ['libchromeos-dbus-bindings'],
          'includes': ['../../common-mk/common_test.gypi'],
          'sources': [
            'testrunner.cc',
            'adaptor_generator_unittest.cc',
            'dbus_signature_unittest.cc',
            'indented_text_unittest.cc',
            'method_name_generator_unittest.cc',
            'name_parser_unittest.cc',
            'proxy_generator_mock_unittest.cc',
            'proxy_generator_unittest.cc',
            'test_utils.cc',
            'xml_interface_parser_unittest.cc',
          ],
        },
      ],
    }],
  ],
}
