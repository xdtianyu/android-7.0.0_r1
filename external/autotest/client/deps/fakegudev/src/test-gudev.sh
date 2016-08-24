#!/bin/bash
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

null_output=`mktemp`
actual_output=`mktemp`
expected_output=`mktemp`
failed=0
export FAKEGUDEV_DEVICES

function test_output() {
  diff -u --ignore-all-space --ignore-blank-lines ${expected_output} \
    ${actual_output}
  if [ $? -ne 0 ]
  then
    echo FAILED
    failed=$(( failed + 1 ))
  fi
}

function run_test() {
  if [ $# -eq 1 ]
  then
    FAKEGUDEV_DEVICES=
  else
    FAKEGUDEV_DEVICES=$2
  fi
  LD_PRELOAD=./libfakegudev.so ./gudev-exercise $1 > ${actual_output}
  test_output
}

function generate_output_file() {
  # If two arguments supplied, second device is the parent of the first.
  cat $1 > ${expected_output}
  if [ $# -eq 2 ]
  then
    echo Parent device:>> ${expected_output}
    cat $2 >> ${expected_output}
  fi
}

./gudev-exercise /dev/null > ${null_output}


echo "TEST: /dev/fake does not appear in test program output"
generate_output_file test_files/empty.output
run_test /dev/fake

echo "TEST: /dev/null appears in test program output"
generate_output_file ${null_output}
run_test /dev/null

echo "TEST: =mem,null finds /dev/null "
generate_output_file ${null_output}
run_test =mem,null

echo "TEST: /sys/devices/virtual/mem/null appears in test program output"
generate_output_file ${null_output}
run_test /sys/devices/virtual/mem/null


echo "TEST: /dev/fake appears when specified in FAKEGUDEV_DEVICES"
generate_output_file test_files/fake.output
run_test /dev/fake test_files/fake.dat

echo "TEST: /dev/null appears when /dev/fake is specified in FAKEGUDEV_DEVICES"
generate_output_file ${null_output}
run_test /dev/null test_files/fake.dat

echo "TEST: Device name appears"
generate_output_file test_files/fake_name.output
run_test /dev/fake test_files/fake_name.dat

echo "TEST: Driver appears"
generate_output_file test_files/fake_driver.output
run_test /dev/fake test_files/fake_driver.dat

echo "TEST: Subsystem appears"
generate_output_file test_files/fake_subsystem.output
run_test /dev/fake test_files/fake_subsystem.dat

echo "TEST: Property appears"
generate_output_file test_files/fake_property_foo.output
run_test /dev/fake test_files/fake_property_foo.dat

echo "TEST: Several properties appear"
generate_output_file test_files/fake_properties.output
run_test /dev/fake test_files/fake_properties.dat

echo "TEST: /dev/fake2 does not appear when only /dev/fake is specified"
generate_output_file test_files/empty.output
run_test /dev/fake2 test_files/fake.dat

echo "TEST: /dev/fake2 and /dev/fake both appear when specified"
generate_output_file test_files/fake.output
run_test /dev/fake test_files/fake_and_fake2.dat
generate_output_file test_files/fake2.output
run_test /dev/fake2 test_files/fake_and_fake2.dat

echo "TEST: /dev/fake appears as parent of /dev/fake2"
generate_output_file test_files/fake2.output test_files/fake.output
run_test /dev/fake2 test_files/fake2_parent_fake.dat

echo "TEST: Real device /dev/null appears as parent of /dev/fake"
generate_output_file test_files/fake.output ${null_output}
run_test /dev/fake test_files/fake_parent_null.dat

echo "TEST: /sys/devices/fake fully fledged"
generate_output_file test_files/fake_full.output ${null_output}
run_test /dev/fake test_files/fake_full.dat

echo "TEST: Search for fake device by subsystem and name works"
generate_output_file test_files/fake_full.output ${null_output}
run_test '=fakesub,fakedevice' test_files/fake_full.dat

echo "TEST: Search for fake device by sysfs path works"
generate_output_file test_files/fake_full.output ${null_output}
run_test /sys/devices/virtual/fake test_files/fake_full.dat

# Can't use handy functions for this test :(
echo "TEST: Property appears when queried repeatedly (test caching)"
cat test_files/fake.output > ${expected_output}
cat test_files/fake.output >> ${expected_output}
cat test_files/fake.output >> ${expected_output}
FAKEGUDEV_DEVICES=test_files/fake.dat
LD_PRELOAD=./libfakegudev.so ./gudev-exercise /dev/fake /dev/fake /dev/fake > \
  ${actual_output}
test_output


echo
echo
echo Total errors: ${failed}
