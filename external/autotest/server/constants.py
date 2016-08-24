# Prefix of directory where collected crash-logs are stored.
CRASHLOGS_DEST_DIR_PREFIX = 'crashlogs'

# Attribute of host that needs logs to be collected.
CRASHLOGS_HOST_ATTRIBUTE = 'need_crash_logs'

# Marker file that is left when crash-logs are collected.
CRASHLOGS_MARKER = '.crashjob'

# Flag file to indicate the host is an adb tester.
ANDROID_TESTER_FILEFLAG = '/mnt/stateful_partition/.android_tester'