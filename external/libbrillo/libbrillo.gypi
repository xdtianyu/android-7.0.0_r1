{
  'target_defaults': {
    'variables': {
      'deps': [
        'libchrome-<(libbase_ver)'
      ],
      'USE_dbus%': '1',
    },
    'include_dirs': [
      '../libbrillo',
    ],
    'defines': [
      'USE_DBUS=<(USE_dbus)',
      'USE_RTTI_FOR_TYPE_TAGS',
    ],
  },
  'targets': [
    {
      'target_name': 'libbrillo-<(libbase_ver)',
      'type': 'none',
      'dependencies': [
        'libbrillo-core-<(libbase_ver)',
        'libbrillo-cryptohome-<(libbase_ver)',
        'libbrillo-http-<(libbase_ver)',
        'libbrillo-minijail-<(libbase_ver)',
        'libbrillo-streams-<(libbase_ver)',
        'libpolicy-<(libbase_ver)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../libbrillo',
        ],
      },
      'includes': ['../common-mk/deps.gypi'],
    },
    {
      'target_name': 'libbrillo-core-<(libbase_ver)',
      'type': 'shared_library',
      'variables': {
        'exported_deps': [
          'dbus-1',
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
      'libraries': ['-lmodp_b64'],
      #TODO(deymo): Split DBus code from libbrillo-core the same way is split in
      # the Android.mk, based on the <(USE_dbus) variable.
      'sources': [
        'brillo/any.cc',
        'brillo/asynchronous_signal_handler.cc',
        'brillo/backoff_entry.cc',
        'brillo/daemons/dbus_daemon.cc',
        'brillo/daemons/daemon.cc',
        'brillo/data_encoding.cc',
        'brillo/dbus/async_event_sequencer.cc',
        'brillo/dbus/data_serialization.cc',
        'brillo/dbus/dbus_connection.cc',
        'brillo/dbus/dbus_method_invoker.cc',
        'brillo/dbus/dbus_method_response.cc',
        'brillo/dbus/dbus_object.cc',
        'brillo/dbus/dbus_service_watcher.cc',
        'brillo/dbus/dbus_signal.cc',
        'brillo/dbus/exported_object_manager.cc',
        'brillo/dbus/exported_property_set.cc',
        'brillo/dbus/utils.cc',
        'brillo/errors/error.cc',
        'brillo/errors/error_codes.cc',
        'brillo/file_utils.cc',
        'brillo/flag_helper.cc',
        'brillo/key_value_store.cc',
        'brillo/message_loops/base_message_loop.cc',
        'brillo/message_loops/message_loop.cc',
        'brillo/message_loops/message_loop_utils.cc',
        'brillo/mime_utils.cc',
        'brillo/osrelease_reader.cc',
        'brillo/process.cc',
        'brillo/process_reaper.cc',
        'brillo/process_information.cc',
        'brillo/secure_blob.cc',
        'brillo/strings/string_utils.cc',
        'brillo/syslog_logging.cc',
        'brillo/type_name_undecorate.cc',
        'brillo/url_utils.cc',
        'brillo/userdb_utils.cc',
        'brillo/value_conversion.cc',
      ],
    },
    {
      'target_name': 'libbrillo-http-<(libbase_ver)',
      'type': 'shared_library',
      'dependencies': [
        'libbrillo-core-<(libbase_ver)',
        'libbrillo-streams-<(libbase_ver)',
      ],
      'variables': {
        'exported_deps': [
          'libcurl',
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
        'brillo/http/curl_api.cc',
        'brillo/http/http_connection_curl.cc',
        'brillo/http/http_form_data.cc',
        'brillo/http/http_request.cc',
        'brillo/http/http_transport.cc',
        'brillo/http/http_transport_curl.cc',
        'brillo/http/http_utils.cc',
      ],
    },
    {
      'target_name': 'libbrillo-streams-<(libbase_ver)',
      'type': 'shared_library',
      'dependencies': [
        'libbrillo-core-<(libbase_ver)',
      ],
      'variables': {
        'exported_deps': [
          'openssl',
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
        'brillo/streams/file_stream.cc',
        'brillo/streams/input_stream_set.cc',
        'brillo/streams/memory_containers.cc',
        'brillo/streams/memory_stream.cc',
        'brillo/streams/openssl_stream_bio.cc',
        'brillo/streams/stream.cc',
        'brillo/streams/stream_errors.cc',
        'brillo/streams/stream_utils.cc',
        'brillo/streams/tls_stream.cc',
      ],
    },
    {
      'target_name': 'libbrillo-test-<(libbase_ver)',
      'type': 'static_library',
      'standalone_static_library': 1,
      'dependencies': [
        'libbrillo-http-<(libbase_ver)',
      ],
      'sources': [
        'brillo/http/http_connection_fake.cc',
        'brillo/http/http_transport_fake.cc',
        'brillo/message_loops/fake_message_loop.cc',
        'brillo/streams/fake_stream.cc',
      ],
      'includes': ['../common-mk/deps.gypi'],
    },
    {
      'target_name': 'libbrillo-cryptohome-<(libbase_ver)',
      'type': 'shared_library',
      'variables': {
        'exported_deps': [
          'openssl',
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
        'brillo/cryptohome.cc',
      ],
    },
    {
      'target_name': 'libbrillo-minijail-<(libbase_ver)',
      'type': 'shared_library',
      'variables': {
        'exported_deps': [
          'libminijail',
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
      'cflags': [
        '-fvisibility=default',
      ],
      'sources': [
        'brillo/minijail/minijail.cc',
      ],
    },
    {
      'target_name': 'libpolicy-<(libbase_ver)',
      'type': 'shared_library',
      'dependencies': [
        'libpolicy-includes',
        '../common-mk/external_dependencies.gyp:policy-protos',
      ],
      'variables': {
        'exported_deps': [
          'openssl',
          'protobuf-lite',
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
      'ldflags': [
        '-Wl,--version-script,<(platform2_root)/libbrillo/libpolicy.ver',
      ],
      'sources': [
        'policy/device_policy.cc',
        'policy/device_policy_impl.cc',
        'policy/libpolicy.cc',
      ],
    },
    {
      'target_name': 'libbrillo-glib-<(libbase_ver)',
      'type': 'shared_library',
      'dependencies': [
          'libbrillo-<(libbase_ver)',
      ],
      'variables': {
        'exported_deps': [
          'dbus-1',
          'dbus-glib-1',
          'glib-2.0',
          'gobject-2.0',
        ],
        'deps': ['<@(exported_deps)'],
      },
      'cflags': [
        # glib uses the deprecated "register" attribute in some header files.
        '-Wno-deprecated-register',
      ],
      'all_dependent_settings': {
        'variables': {
          'deps': [
            '<@(exported_deps)',
          ],
        },
      },
      'sources': [
        'brillo/glib/abstract_dbus_service.cc',
        'brillo/glib/dbus.cc',
        'brillo/message_loops/glib_message_loop.cc',
      ],
      'includes': ['../common-mk/deps.gypi'],
    },
  ],
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'libbrillo-<(libbase_ver)_unittests',
          'type': 'executable',
          'dependencies': [
            'libbrillo-<(libbase_ver)',
            'libbrillo-test-<(libbase_ver)',
            'libbrillo-glib-<(libbase_ver)',
          ],
          'variables': {
            'deps': [
              'libchrome-test-<(libbase_ver)',
            ],
            'proto_in_dir': 'brillo/dbus',
            'proto_out_dir': 'include/brillo/dbus',
          },
          'includes': [
            '../common-mk/common_test.gypi',
            '../common-mk/protoc.gypi',
          ],
          'cflags': [
            '-Wno-format-zero-length',
          ],
          'conditions': [
            ['debug == 1', {
              'cflags': [
                '-fprofile-arcs',
                '-ftest-coverage',
                '-fno-inline',
              ],
              'libraries': [
                '-lgcov',
              ],
            }],
          ],
          'sources': [
            'brillo/any_unittest.cc',
            'brillo/any_internal_impl_unittest.cc',
            'brillo/asynchronous_signal_handler_unittest.cc',
            'brillo/backoff_entry_unittest.cc',
            'brillo/data_encoding_unittest.cc',
            'brillo/dbus/async_event_sequencer_unittest.cc',
            'brillo/dbus/data_serialization_unittest.cc',
            'brillo/dbus/dbus_method_invoker_unittest.cc',
            'brillo/dbus/dbus_object_unittest.cc',
            'brillo/dbus/dbus_param_reader_unittest.cc',
            'brillo/dbus/dbus_param_writer_unittest.cc',
            'brillo/dbus/dbus_signal_handler_unittest.cc',
            'brillo/dbus/exported_object_manager_unittest.cc',
            'brillo/dbus/exported_property_set_unittest.cc',
            'brillo/errors/error_codes_unittest.cc',
            'brillo/errors/error_unittest.cc',
            'brillo/file_utils_unittest.cc',
            'brillo/flag_helper_unittest.cc',
            'brillo/glib/object_unittest.cc',
            'brillo/http/http_connection_curl_unittest.cc',
            'brillo/http/http_form_data_unittest.cc',
            'brillo/http/http_request_unittest.cc',
            'brillo/http/http_transport_curl_unittest.cc',
            'brillo/http/http_utils_unittest.cc',
            'brillo/key_value_store_unittest.cc',
            'brillo/map_utils_unittest.cc',
            'brillo/message_loops/base_message_loop_unittest.cc',
            'brillo/message_loops/fake_message_loop_unittest.cc',
            'brillo/message_loops/glib_message_loop_unittest.cc',
            'brillo/message_loops/message_loop_unittest.cc',
            'brillo/mime_utils_unittest.cc',
            'brillo/osrelease_reader_unittest.cc',
            'brillo/process_reaper_unittest.cc',
            'brillo/process_unittest.cc',
            'brillo/secure_blob_unittest.cc',
            'brillo/streams/fake_stream_unittest.cc',
            'brillo/streams/file_stream_unittest.cc',
            'brillo/streams/input_stream_set_unittest.cc',
            'brillo/streams/memory_containers_unittest.cc',
            'brillo/streams/memory_stream_unittest.cc',
            'brillo/streams/openssl_stream_bio_unittests.cc',
            'brillo/streams/stream_unittest.cc',
            'brillo/streams/stream_utils_unittest.cc',
            'brillo/strings/string_utils_unittest.cc',
            'brillo/type_name_undecorate_unittest.cc',
            'brillo/unittest_utils.cc',
            'brillo/url_utils_unittest.cc',
            'brillo/variant_dictionary_unittest.cc',
            'brillo/value_conversion_unittest.cc',
            'testrunner.cc',
            '<(proto_in_dir)/test.proto',
          ]
        },
        {
          'target_name': 'libpolicy-<(libbase_ver)_unittests',
          'type': 'executable',
          'dependencies': ['libpolicy-<(libbase_ver)'],
          'includes': ['../common-mk/common_test.gypi'],
          'sources': [
            'policy/tests/libpolicy_unittest.cc',
          ]
        },
      ],
    }],
  ],
}
