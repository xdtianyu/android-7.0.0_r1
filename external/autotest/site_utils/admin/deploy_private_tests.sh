#!/bin/bash
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
USAGE="Usage: deploy_private_test.sh -a PATH_TO_PUBLIC_AUTOTEST \
-p PATH_TO_PRIVATE_AUTOTEST"
HELP="${USAGE}
    Link server side tests under PATH_TO_PRIVATE_AUTOTEST \
to PATH_TO_PUBLIC_AUTOTEST

    PATH_TO_PRIVATE_AUTOTEST defaults to \
/usr/local/autotest/site_utils/autotest_private
    PATH_TO_PUBLIC_AUTOTEST defaults to \
/usr/local/autotest"

AUTOTEST_ROOT="/usr/local/autotest"
PRIVATE_AUTOTEST_ROOT="${AUTOTEST_ROOT}/site_utils/autotest_private"

while getopts ":p:a:h" opt; do
  case $opt in
    a)
      AUTOTEST_ROOT=$OPTARG
      ;;
    p)
      PRIVATE_AUTOTEST_ROOT=$OPTARG
      ;;
    h)
      echo "${HELP}" >&2
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo "${USAGE}" >&2
      exit 1
      ;;
  esac
done

if [ ! -d "${AUTOTEST_ROOT}" ]; then
  echo "Invalid public autotest root: ${AUTOTEST_ROOT}" >&2
  exit 1
fi

if [ ! -d "${PRIVATE_AUTOTEST_ROOT}" ]; then
  echo "Invalid private autotest root: ${PRIVATE_AUTOTEST_ROOT}" >&2
  exit 1
fi

PUBLIC_SERVER_TESTS_DIR="${AUTOTEST_ROOT}/server/site_tests"
PUBLIC_SERVER_SUITES_DIR="${PUBLIC_SERVER_TESTS_DIR}/suites"
PUBLIC_CLIENT_TESTS_DIR="${AUTOTEST_ROOT}/client/site_tests"

PRIVATE_SERVER_TESTS_DIR="${PRIVATE_AUTOTEST_ROOT}/server/site_tests"
PRIVATE_SERVER_SUITES_DIR="${PRIVATE_SERVER_TESTS_DIR}/suites"
PRIVATE_CLIENT_TESTS_DIR="${PRIVATE_AUTOTEST_ROOT}/client/site_tests"

echo "Removing existing symbolic links in ${PUBLIC_SERVER_TESTS_DIR}, \
${PUBLIC_CLIENT_TESTS_DIR} and ${PUBLIC_SERVER_SUITES_DIR}"
find ${PUBLIC_SERVER_TESTS_DIR} -type l -exec rm -v {} \;
find ${PUBLIC_CLIENT_TESTS_DIR} -type l -exec rm -v {} \;
find ${PUBLIC_SERVER_SUITES_DIR} -type l -exec rm -v {} \;
echo "Creating links for tests..."
find ${PRIVATE_SERVER_TESTS_DIR} -mindepth 1 -maxdepth 1 \
     -type d ! -path ${PRIVATE_SERVER_SUITES_DIR} \
     -exec ln -v -s {} ${PUBLIC_SERVER_TESTS_DIR} \;
find ${PRIVATE_CLIENT_TESTS_DIR} -mindepth 1 -maxdepth 1 \
     -type d ! -path ${PRIVATE_SERVER_SUITES_DIR} \
     -exec ln -v -s {} ${PUBLIC_CLIENT_TESTS_DIR} \;
echo "Creating links for suites..."
find ${PRIVATE_SERVER_SUITES_DIR} -mindepth 1 -maxdepth 1 -type f \
     -exec ln -v -s {} ${PUBLIC_SERVER_SUITES_DIR} \;
