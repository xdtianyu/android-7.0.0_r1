#!/bin/bash
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Author: Dale Curtis (dalecurtis@google.com)
#
# Small cron script which uses a PID file to check if Dev Server is running. If
# not, the Dev Server is started and the PID is recorded.
#
# Script is written with the layout of Kirkland test lab's Dev Server in mind.
#


# Path to Dev Server source code.
declare -r DEV_SERVER_PATH="/usr/local/google/chromeos/src/platform/dev/"

# Path to Dev Server images directory.
declare -r IMAGES_PATH="/usr/local/google/images"

# PID file location.
declare -r PID_FILE="/tmp/dev_server.pid"


function start_dev_server {
  echo "Starting a new Dev Server instance..."
  cd ${DEV_SERVER_PATH}
  python devserver.py 8080 --archive_dir ${IMAGES_PATH} -t &>/dev/null&
  echo $!>${PID_FILE}
}


# First check for PID file, then check if PID is valid. If either are false,
# start a new Dev Server instance.
if [ -f ${PID_FILE} ]; then
  ps $(cat ${PID_FILE}) | grep -q devserver.py || start_dev_server
else
  start_dev_server
fi
