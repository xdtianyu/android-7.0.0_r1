# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# Job keyvals for finding debug symbols when processing crash dumps.
JOB_BUILD_KEY = 'build'
JOB_SUITE_KEY = 'suite'

# Job keyvals for builds to be installed in dut and source of server-side tests.
JOB_BUILDS_KEY = 'builds'
JOB_TEST_SOURCE_BUILD_KEY = 'test_source_build'

# Job keyval indicating whether a job is for an experimental test.
JOB_EXPERIMENTAL_KEY = 'experimental'
RETRY_ORIGINAL_JOB_ID = 'retry_original_job_id'
# Job keyval indicating the minimum duts required by the suite
SUITE_MIN_DUTS_KEY = 'suite_min_duts'

# Job attribute and label names
EXPERIMENTAL_PREFIX = 'experimental_'
FWRW_BUILD = 'fwrw_build'
FWRO_BUILD = 'fwro_build'
JOB_REPO_URL = 'job_repo_url'
VERSION_PREFIX = 'cros-version:'
BOARD_PREFIX = 'board:'

# Bug filing
ISSUE_OPEN = 'open'
ISSUE_CLOSED = 'closed'
ISSUE_DUPLICATE = 'Duplicate'
ISSUE_MERGEDINTO = 'mergedInto'
ISSUE_STATE = 'state'
ISSUE_STATUS = 'status'

# Timings
ARTIFACT_FINISHED_TIME = 'artifact_finished_time'
DOWNLOAD_STARTED_TIME = 'download_started_time'
PAYLOAD_FINISHED_TIME = 'payload_finished_time'

# Reimage type names
# Please be very careful in changing or adding to these, as one needs to
# maintain backwards compatibility.
REIMAGE_TYPE_OS = 'os'
REIMAGE_TYPE_FIRMWARE = 'firmware'
LATEST_BUILD_URL = 'gs://chromeos-image-archive/master-paladin/LATEST-master'

JOB_OFFLOAD_FAILURES_KEY = 'offload_failures_only'

GS_OFFLOADER_INSTRUCTIONS = '.GS_OFFLOADER_INSTRUCTIONS'
GS_OFFLOADER_NO_OFFLOAD = 'no_offload'

PARENT_JOB_ID = 'parent_job_id'
