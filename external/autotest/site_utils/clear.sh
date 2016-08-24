#!/bin/bash
#
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The purpose of this script is to be able to reset an autotest instance.
# This means cleaning up the database and all log and results files.
# The main use case for this is if the master ever fails and all shards need to
# be reset.

declare -a SERVICES=("apache2" "scheduler" "host-scheduler" "shard-client"
                     "gs_offloader" "gs_offloader_s")
AUTOTEST_DIR=$(dirname $(dirname $0))

function service_action {
  local s
  for s in "${SERVICES[@]}"; do
    if [[ -e "/etc/init/$s.conf" || -e "/etc/init.d/$s" ]]; then
      sudo service $s $1
    fi
  done
}

service_action stop

${AUTOTEST_DIR}/frontend/manage.py dbshell <<END
DROP DATABASE chromeos_autotest_db;
CREATE DATABASE chromeos_autotest_db;
END

${AUTOTEST_DIR}/database/migrate.py sync -f
${AUTOTEST_DIR}/frontend/manage.py syncdb --noinput
${AUTOTEST_DIR}/frontend/manage.py syncdb --noinput

sudo rm -rf ${AUTOTEST_DIR}/results/*
sudo rm -rf ${AUTOTEST_DIR}/logs/*

service_action start
