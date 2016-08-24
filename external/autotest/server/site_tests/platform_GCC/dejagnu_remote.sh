#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
find_common_sh() {
  local common_paths=(/usr/lib/crosutils $(dirname "$0"))
  local path

  SCRIPT_ROOT=
  for path in "${common_paths[@]}"; do
    local common="${path}/common.sh"
    if ([ -r "${common}" ] && . "${common}" && [ -d "${SCRIPTS_DIR}" ]); then
      SCRIPT_ROOT="${path}"
      break
    fi
  done
}
find_common_sh
. "${SCRIPT_ROOT}/common.sh" || ! echo "Unable to load common.sh" || exit 1

DEFAULT_PRIVATE_KEY="${GCLIENT_ROOT}/src/scripts/mod_for_test_scripts/\
ssh_keys/testing_rsa"

TMP="/tmp/dejagnu-tests/"
TMP_PRIVATE_KEY=${TMP}/private_key
TMP_KNOWN_HOSTS=${TMP}/known_hosts
CONTROL_PATH="${TMP}/%r@%h:%p"
SSH_ARGS="-p22 -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=${TMP_KNOWN_HOSTS} -i ${TMP_PRIVATE_KEY}"

dejagnu_init_remote() {
  mkdir -p ${TMP}
  cp ${DEFAULT_PRIVATE_KEY} ${TMP_PRIVATE_KEY}
  chmod 0400 ${TMP_PRIVATE_KEY}
  PS1=. TERM=linux ssh ${SSH_ARGS} -t -t -M -S "${CONTROL_PATH}" root@$1 \
      >/dev/null  2>&1 &
  echo $! > "${TMP}/master-pid"
  dejagnu_ssh root@$1 -- "echo Connection OK."
}

dejagnu_cleanup_remote() {
  set +e
  kill "$(cat ${TMP}/master-pid)"
  set -e
  rm -rf "${TMP}"
}

dejagnu_ssh() {
  COMMAND="ssh ${SSH_ARGS} -t -o ControlPath=${CONTROL_PATH} $@"
  # TODO(raymes): Remove this timeout hack once our tests run without
  # infinite loops.
  TIMEOUT_COMMAND="$(echo "$COMMAND" | sed "s/sh -c '/sh -c 'timeout 5 /g")"
  $TIMEOUT_COMMAND
}

dejagnu_scp() {
  scp ${SSH_ARGS} -o ControlPath="${CONTROL_PATH}" $@
}
