{
  'targets': [
    {
      'target_name': 'libpolicy-includes',
      'type': 'none',
      'copies': [
        {
          'destination': '<(SHARED_INTERMEDIATE_DIR)/include/policy',
          'files': [
            'policy/device_policy.h',
            'policy/device_policy_impl.h',
            'policy/libpolicy.h',
            'policy/mock_libpolicy.h',
            'policy/mock_device_policy.h',
          ],
        },
      ],
    },
  ],
}
