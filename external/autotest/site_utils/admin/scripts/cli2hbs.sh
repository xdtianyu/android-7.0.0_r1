#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Author: ericli@google.com (Eric Li)
#
# This script copies needed files from the repo to allow Autotest CLI access 
# from /h/b/s.


SCRIPT_DIR=$(cd $(dirname $0);pwd)
AUTOTEST_TOOLS_DIR=$(cd ${SCRIPT_DIR}/../..;pwd)
REPO_DIR=$(cd ${AUTOTEST_TOOLS_DIR}/../../..;pwd)
AUTOTEST_DIR="${REPO_DIR}/src/third_party/autotest/files"

DATESTAMP=$(date '+%Y%m%d')
TARGET_DIR="/home/build/static/projects-rw/chromeos/autotest.${DATESTAMP}"

cp -fpruv ${AUTOTEST_DIR}/cli ${TARGET_DIR}

mkdir -p ${TARGET_DIR}/client
touch ${TARGET_DIR}/client/__init__.py
cp -uv ${AUTOTEST_DIR}/client/setup_modules.py ${TARGET_DIR}/client
cp -uv ${AUTOTEST_TOOLS_DIR}/autotest/global_config.ini ${TARGET_DIR}/client
cp -fpruv ${AUTOTEST_DIR}/client/common_lib ${TARGET_DIR}/client

mkdir -p ${TARGET_DIR}/frontend/afe
touch ${TARGET_DIR}/frontend/__init__.py
touch ${TARGET_DIR}/frontend/afe/__init__.py
cp -uv ${AUTOTEST_DIR}/frontend/common.py \
    ${TARGET_DIR}/frontend
cp -fpruv ${AUTOTEST_DIR}/frontend/afe/json_rpc \
    ${TARGET_DIR}/frontend/afe
cp -uv ${AUTOTEST_DIR}/frontend/afe/rpc_client_lib.py \
    ${TARGET_DIR}/frontend/afe
cp -uv \
    ${AUTOTEST_TOOLS_DIR}/autotest/syncfiles/frontend/afe/site_rpc_client_lib.py \
    ${TARGET_DIR}/frontend/afe

# update autotest symlink
cd $(dirname ${TARGET_DIR})
unlink autotest
ln -s $(basename ${TARGET_DIR}) autotest
