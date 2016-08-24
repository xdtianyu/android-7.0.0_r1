#!/system/bin/sh
#
# Copyright (C) 2014 The Android Open Source Project
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

UNIT_TEST_DIR="jemalloc_unittests"

UNIT_TESTS=( \
  "atomic" \
  "bitmap" \
  "ckh" \
  "decay" \
  "hash" \
  "junk" \
  "junk_alloc" \
  "junk_free" \
  "lg_chunk" \
  "mallctl" \
  "math" \
  "mq" \
  "mtx" \
  "nstime" \
  "prng" \
  "prof_accum" \
  "prof_active" \
  "prof_gdump" \
  "prof_idump" \
  "prof_reset" \
  "prof_thread_name" \
  "ql" \
  "qr" \
  "quarantine" \
  "rb" \
  "rtree" \
  "run_quantize" \
  "SFMT" \
  "size_classes" \
  "smoothstep" \
  "stats" \
  "ticker" \
  "tsd" \
  "util" \
  "zero" \
)

INTEGRATION_TEST_DIR="jemalloc_integrationtests"

INTEGRATION_TESTS=( \
  "aligned_alloc" \
  "allocated" \
  "chunk" \
  "MALLOCX_ARENA" \
  "mallocx" \
  "overflow" \
  "posix_memalign" \
  "rallocx" \
  "sdallocx" \
  "thread_arena" \
  "thread_tcache_enabled" \
  "xallocx" \
)

TEST_DIRECTORIES=( "/data/nativetest" "/data/nativetest64" )
FAILING_TESTS=()

function run_tests () {
  local test_type=$1
  shift
  local test_dir=$1
  shift
  local test_list=$*
  if [[ -d "${test_dir}" ]]; then
    for test in ${test_list[@]}; do
      echo "Running ${test_type} ${test}"
      ${test_dir}/$test
      local exit_val=$?
      # 0 means all tests passed.
      # 1 means all tests passed but some tests were skipped.
      # 2 means at least one failure.
      if [[ ${exit_val} -ne 0 ]] && [[ ${exit_val} -ne 1 ]]; then
        echo "*** $test failed: ${exit_val}"
        FAILING_TESTS+=("${test}")
        EXIT_CODE=$((EXIT_CODE+1))
      fi
    done
  fi
}

EXIT_CODE=0
for test_dir in ${TEST_DIRECTORIES[@]}; do
  if [[ -d "${test_dir}" ]]; then
    run_tests "unit" "${test_dir}/${UNIT_TEST_DIR}" ${UNIT_TESTS[@]}
    run_tests "integration" "${test_dir}/${INTEGRATION_TEST_DIR}" ${INTEGRATION_TESTS[@]}
  fi
done
if [[ ${EXIT_CODE} -eq 0 ]]; then
  echo "All tests passed"
else
  echo "List of failing tests:"
  for fail in ${FAILING_TESTS[@]}; do
    echo "  $fail"
  done
fi
exit ${EXIT_CODE}
