#!/bin/bash
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

USAGE='Usage: deploy_puppet.sh [-rsndh]'
HELP="${USAGE}\n\n\
Force puppet deployment.\n\
The script will first fetch server hostnames from server db,\n\
given the server role and status and/or server name.\n\
And then force puppet deployment on the selected servers.\n\n\
Requirement:
 - Run on the machine that has access to server db and
 - Run it as chromeos-test.\n\
 - The server must exist in server db, even if -n is used.

It should be safe to rerun the script multiple time, \n\
as it doesn't hurt to deploy puppet multiple times.\n\n\
Options:\n\
  -r server role as in server db, e.g. 'drone'.\n\
  -s server status as in server db, e.g. 'primary'.\n\
  -n server hostname.\n\
  -d dryrun.\n\
  -h help."

ROLE=
ROLE_OPT=
STATUS=
STATUS_OPT=
HOSTNAME=
DRYRUN="FALSE"
AUTOTEST_ROOT="/usr/local/autotest"
while getopts ":s:r:n:dh" opt; do
  case $opt in
    r)
      ROLE=$OPTARG
      ;;
    s)
      STATUS=$OPTARG
      ;;
    n)
      HOSTNAME=$OPTARG
      ;;
    d)
      DRYRUN="TRUE"
      ;;
    h)
      echo -e "${HELP}" >&2
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
  esac
done

if [ -n "${ROLE}" ]; then
  ROLE_OPT="-r ${ROLE}"
fi

if [ -n "${STATUS}" ]; then
  STATUS_OPT="-s ${STATUS}"
fi

if [ -z "${ROLE}" ] && [ -z "${STATUS}" ] && [ -z "${HOSTNAME}"]; then
  echo "You must specify at least one of -r, -s or -n"
  exit 1
fi

hosts="$(${AUTOTEST_ROOT}/cli/atest server list ${STATUS_OPT} ${ROLE_OPT} ${HOSTNAME}| grep Hostname| awk {'print $3'})"

echo -e "\n******* Will update the following servers ********\n "

for host in ${hosts}; do
  echo ${host}
done

echo -e "\n**************************************************\n"

for host in ${hosts}; do
  git_pull="ssh -t ${host} -- 'sudo git --work-tree=/root/chromeos-admin --git-dir=/root/chromeos-admin/.git pull'"
  run_puppet="ssh ${host} -- 'sudo /root/chromeos-admin/puppet/run_puppet'"
  echo -e "\n********** Processing ${host} ****************\n"
  echo "[Running] ${git_pull}"
  if [ "${DRYRUN}" != "TRUE" ]; then
    eval ${git_pull}
  fi
  echo "[Running] ${run_puppet}"
  if [ "${DRYRUN}" != "TRUE" ]; then
    eval ${run_puppet}
  fi
  echo -e "\n********* Finished processing ${host} *******\n"
done
