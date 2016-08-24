#!/bin/env bash
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

FAKE_SYSCALLS_LIB=`pwd`/libfakesyscalls.so
FAKE_SYSCALLS_DBG=/tmp/fake_syscalls.dbg
TEST_EXE=`pwd`/fakesyscalls-exercise

# build up the redirection envrionment variable
FILE_REDIRECTIONS_PRELOAD=:
FILE_REDIRECTIONS_PRELOAD=${FILE_REDIRECTIONS_PRELOAD}:/tmp/foo1=/tmp/foo2
FILE_REDIRECTIONS_PRELOAD=${FILE_REDIRECTIONS_PRELOAD}:/tmp/foo3
FILE_REDIRECTIONS_PRELOAD=${FILE_REDIRECTIONS_PRELOAD}:/tmp/foo4=.
FILE_REDIRECTIONS_PRELOAD=${FILE_REDIRECTIONS_PRELOAD}:/tmp/foo5=/tmp/foo6
export FILE_REDIRECTIONS_PRELOAD=$FILE_REDIRECTIONS_PRELOAD

rm -f /tmp/foo1 /tmp/foo2 /tmp/foo3 /tmp/foo4 /tmp/foo5 /tmp/foo6 /tmp/foo99
rm -f ${FAKE_SYSCALLS_DBG}

# Successful redirection
echo LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo1 /tmp/foo2
LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo1 /tmp/foo2
echo
echo ${FAKE_SYSCALLS_DBG}
cat ${FAKE_SYSCALLS_DBG}
echo

# Malformed map --> unsuccessful redirection
echo LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo3 /tmp/foo3
LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo3 /tmp/foo3
echo
echo ${FAKE_SYSCALLS_DBG}
cat ${FAKE_SYSCALLS_DBG}
echo

# Relative path in map --> unsuccessful redirection
echo LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo4 /tmp/foo4
LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo4 /tmp/foo4
echo
echo ${FAKE_SYSCALLS_DBG}
cat ${FAKE_SYSCALLS_DBG}
echo

# Does not exist in map --> no redirection.
echo LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo99 /tmp/foo99
LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} /tmp/foo99 /tmp/foo99
echo
echo ${FAKE_SYSCALLS_DBG}
cat ${FAKE_SYSCALLS_DBG}
echo


pushd . >/dev/null
cd /tmp >/dev/null
# Relative path in open() --> unsuccessful redirection
echo LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} foo5 foo5
LD_PRELOAD=${FAKE_SYSCALLS_LIB} ${TEST_EXE} foo5 foo5
if test -e /tmp/foo6
then
  echo Fail: /tmp/foo6 should not have been created. foo6:
  cat /tmp/foo6
fi
echo
echo ${FAKE_SYSCALLS_DBG}
cat ${FAKE_SYSCALLS_DBG}
echo
popd >/dev/null
