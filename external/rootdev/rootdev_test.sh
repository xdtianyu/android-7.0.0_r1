#!/bin/bash
# Copyright 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Simple functional test harness for rootdev
# TODO(wad) rootdev should be rewritten in C++ and gmocked.

set -u

warn () {
  echo "WARN: $@" 1>&2
}

error () {
  echo "ERROR: $@" 1>&2
  exit 1
}

PASS_COUNT=0
pass () {
  echo "PASS:$1" 1>&2
  PASS_COUNT=$((PASS_COUNT + 1))
  return 0
}

FAIL_COUNT=0
fail () {
  echo "FAIL:$1" 1>&2
  FAIL_COUNT=$((FAIL_COUNT + 1))
  return 0
}

WORKDIR=
cleanup () {
  if [ -n "$WORKDIR" ]; then
    rm -rf "$WORKDIR"
  fi
  trap - EXIT
}

setup () {
  WORKDIR=$(mktemp -d rootdev_test.XXXXXXX)
  if [ ! -d "$WORKDIR" ]; then
    error "Failed to create temporary work dir"
  fi
  trap cleanup EXIT
}

run_test () {
  setup
  echo "RUN:$1" 1>&2
  eval $1
  ret=$?
  cleanup
  if [ $ret -eq 0 ]; then
    pass $1
  else
    fail $1
  fi
}

expect () {
  cond="$1"
  eval test $1
  if [ $? -ne 0 ]; then
    warn "expect: $1"
    return 1
  fi
  return 0
}

ROOTDEV=${1:-./rootdev}
if [[ ! -e ${ROOTDEV} ]]; then
  error "could not find rootdev '${ROOTDEV}'"
fi

if [ "${USER:-}" != "root" ]; then
  error "Must be run as root to use mknod (${USER:-})"
fi

t00_bad_sys_dir () {
  out=$("${ROOTDEV}" --block $WORKDIR 2>/dev/null)
  expect "$? -ne 0" || return 1
  expect "-z '$out'" || return 1
}
run_test t00_bad_sys_dir

h00_setup_sda_tree() {
  local block=$1
  local dev=$2
  mkdir -p $block
  mkdir -p $dev
  mkdir -p $block/sda/sda1
  mkdir -p $block/sda/sda2
  echo "10:0" > $block/sda/dev
  echo "10:1" > $block/sda/sda1/dev
  echo "10:2" > $block/sda/sda2/dev
  mknod $dev/sda1 b 10 1
  mknod $dev/sda2 b 10 2
  mknod $dev/sda b 10 0
}

t01_sys_dev_match () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev

  out=$("${ROOTDEV}" --dev $dev --block $block --major 10 --minor 1 2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda1' = '$out'" || return 1
}
run_test t01_sys_dev_match

t02_sys_dev_match_block () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev

  out=$("${ROOTDEV}" --dev $dev --block $block --major 10 --minor 0 2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda' = '$out'" || return 1
}
run_test t02_sys_dev_match_block

t03_sys_dev_match_block_no_dev () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  rm $dev/sda

  out=$("${ROOTDEV}" --dev $dev --block $block --major 10 --minor 0 2>/dev/null)
  expect "$? -eq 1" || return 1
  expect "'$dev/sda' = '$out'" || return 1
}
run_test t03_sys_dev_match_block_no_dev

t04_sys_dev_match_block_no_dev_ignore () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  rm $dev/sda

  out=$("${ROOTDEV}" -i --dev $dev --block $block --major 10 --minor 0 2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda' = '$out'" || return 1
}
run_test t04_sys_dev_match_block_no_dev_ignore


h01_setup_dm_tree() {
  local block=$1
  local dev=$2
  mkdir -p $block
  mkdir -p $dev
  mkdir -p $block/dm-0
  mkdir -p $block/dm-0/slaves/sda1
  echo "254:0" > $block/dm-0/dev
  echo "10:1" > $block/dm-0/slaves/sda1/dev
  mknod $dev/dm-0 b 254 0
}

t05_match_dm () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  h01_setup_dm_tree $block $dev

  out=$("${ROOTDEV}" --dev $dev --block $block --major 254 --minor 0 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/dm-0' = '$out'" || return 1
}
run_test t05_match_dm

t06_match_dm_slave () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  h01_setup_dm_tree $block $dev

  out=$("${ROOTDEV}" -s --dev $dev --block $block --major 254 --minor 0 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda1' = '$out'" || return 1
}
run_test t06_match_dm_slave

t07_safe_fail_on_no_slave () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  h01_setup_dm_tree $block $dev

  out=$("${ROOTDEV}" -s --dev $dev --block $block --major 10 --minor 1 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda1' = '$out'" || return 1
}
run_test t07_safe_fail_on_no_slave

t08_safe_fail_on_no_slave_dev () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  h01_setup_dm_tree $block $dev
  # If the matching dev node is missing, an error code will be returned
  # but the path will still represent the slave.
  rm $dev/sda1

  out=$("${ROOTDEV}" -s --dev $dev --block $block --major 254 --minor 0 \
        2>/dev/null)
  expect "$? -eq 1" || return 1
  expect "'$dev/sda1' = '$out'" || return 1
}
run_test t08_safe_fail_on_no_slave_dev

t09_safe_fail_on_no_slave_dev_ignore () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  h01_setup_dm_tree $block $dev
  # If the matching dev node is missing, an error code will be returned
  # but the path will still represent the slave.
  rm $dev/sda1

  out=$("${ROOTDEV}" -i -s --dev $dev --block $block --major 254 --minor 0 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda1' = '$out'" || return 1
}
run_test t09_safe_fail_on_no_slave_dev_ignore

h02_setup_mmc_tree() {
  local block=$1
  local dev=$2
  mkdir -p $block
  mkdir -p $dev
  mkdir -p $block/mmcblk0/mmcblk0p1
  mkdir -p $block/mmcblk0/mmcblk0p2
  echo "11:0" > $block/mmcblk0/dev
  echo "11:1" > $block/mmcblk0/mmcblk0p1/dev
  echo "11:2" > $block/mmcblk0/mmcblk0p2/dev
  mknod $dev/mmcblk0 b 11 0
  mknod $dev/mmcblk0p1 b 11 1
  mknod $dev/mmcblk0p2 b 11 2
}

t10_mmcdev () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h02_setup_mmc_tree $block $dev
  out=$("${ROOTDEV}" --dev $dev --block $block --major 11 --minor 2 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/mmcblk0p2' = '$out'" || return 1
}
run_test t10_mmcdev

t11_mmcdev_strip () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h02_setup_mmc_tree $block $dev
  out=$("${ROOTDEV}" -d --dev $dev --block $block --major 11 --minor 2 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/mmcblk0' = '$out'" || return 1
}
run_test t11_mmcdev_strip

t12_sda_strip () {
  local block=$WORKDIR/sys/block
  local dev=$WORKDIR/dev
  h00_setup_sda_tree $block $dev
  out=$("${ROOTDEV}" -d --dev $dev --block $block --major 10 --minor 2 \
        2>/dev/null)
  expect "$? -eq 0" || return 1
  expect "'$dev/sda' = '$out'" || return 1
}
run_test t12_sda_strip

# TODO(wad) add node creation tests

TEST_COUNT=$((PASS_COUNT + FAIL_COUNT))

echo "----"
echo "Test passed:  $PASS_COUNT / $TEST_COUNT"
echo "Test failed:  $FAIL_COUNT / $TEST_COUNT"

if [ $FAIL_COUNT -ne 0 ]; then
  exit 1
fi
