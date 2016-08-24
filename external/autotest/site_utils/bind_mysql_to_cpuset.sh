#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script creates a CPU set and binds the MySQL process to the
# set. It restarts MySQL if specified in the option. This script needs
# to be run with "sudo".

set -e # Exit if any function returns a non-zero value.
set -o nounset # Exit if any variable is used unset.

MYSQL_PATH='/etc/init.d/mysql.server'
MYSQL_PID_PATH='/var/lib/mysql/atlantis1.mtv.corp.google.com.pid'
MYSQL_PROC_NAME='mysqld'

# The directory where we mount the cpuset virutal file system to.
MOUNT_DIR='/dev/cpuset'
# The base cpuset directory for mysql.
MYSQL_CPUSET='mysql'
# CPUs in MySQL cpuset. E.g. 0-2,7,12-14.
MYSQL_DEFAULT_CPUS=10-15


# Display usage.
function usage() {
  echo -e "Usage: $0 [-c <CPUs>] [-p <PID>] [-r] [-d]\n"
  echo -e "Create and bind the MySQL process to a specified CPU set.\n"
  echo -e "Options:"
  echo -e "  -c <CPUs>  Specify a list of CPUs to be used. E.g. 0-2,7,12-14"
  echo -e "             (Default: $MYSQL_DEFAULT_CPUS)."
  echo -e "  -d         Delete the CPU set. This option kills the current"
  echo -e "             MySQL process and delete the CPU set. It does not"
  echo -e "             restart MySQL nor create a new CPU set. (Default:"
  echo -e "             disabled)."
  echo -e "  -p <PID>   Bind <PID> to the cpuset (Default: the script searches"
  echo -e "             for the MySQL PID automatically)."
  echo -e "  -r         Restart MySQL (Default: disabled)."
  echo -e "  -h         Display this usage information."
  echo -e "\n"
}

function print_info() {
  msg=$1
  echo "INFO:  "$msg
}

function print_error() {
  msg=$1
  echo "ERROR: "$msg
}

# Run and print out the command if the silent flag is not set (the default).
# Usage: run_cmd <cmd> [slient]
function run_cmd() {
  cmd=""
  slient=0

  if [ $# -gt 0 ]; then
    cmd=$1
  else
    print_error "Empty command!"
    return 1
  fi

  if [ $# -gt 1 ]; then
    silent=$2
  fi

  if [ $slient -eq 0 ]; then
    print_info "Running \"${1}\""
  fi

  # Print an error message if the command failed.
  eval "$1" || { print_error "Failed to execute \"${cmd}\""; return 1; }
}

# Get the PID of the MySQL.
function get_mysql_pid() {
  local pid=""

  if [ $# -gt 0 ] && [ ! -z "$1" ]; then
    # Use user-provided PID.
    pid=$1
  elif [ ! -z ${MYSQL_PID_PATH} -a -f ${MYSQL_PID_PATH} ]; then
    # Get PID from MySQL PID file if it is set.
    print_info "Getting MySQL PID from ${MYSQL_PID_PATH}..."
    pid=$(cat $MYSQL_PID_PATH) || \
      { print_error "No MySQL process found."; return 1; }
  else
    # Get PID of process named mysqld.
    print_info "Searching for MySQL PID..."
    # Ignore the return code to print out an error message.
    pid=$(pidof $MYSQL_PROC_NAME) || \
      { print_error "No MySQL process found."; return 1; }
  fi

  # Test if the PID is an integer
  if [[ $pid != [0-9]* ]]; then
    print_error "No MySQL process found."
    return 1
  fi

  # Check if the PID is a running process.
  if [ ! -d "/proc/${pid}" ]; then
    print_error "No running MySQL process is found."
    return 1
  fi

  _RET="$pid"
  print_info "MySQL PID is ${pid}."
}

# Mount the cpuset virtual file system.
function mount_cpuset() {
  if (mount | grep "on ${MOUNT_DIR} type" > /dev/null)
  then
    print_info "${MOUNT_DIR} already mounted."
  else
    print_info "Mounting cpuset to $MOUNT_DIR."
    run_cmd "mkdir -p ${MOUNT_DIR}"
    run_cmd "mount -t cpuset none ${MOUNT_DIR}"
  fi
}


function clean_all() {
  local delete_msg="No"
  print_info "Will Delete existing CPU set..."
  echo -ne "WARNING: This operation will kill all running "
  echo "processes in the CPU set."
  echo -ne "Are you sure you want to proceed "
  echo -ne "(type \"yes\" or \"Yes\" to proceed)? "
  read delete_msg

  mount_cpuset

  local proc_list=""
  local proc=""

  if [ "$delete_msg" = "yes" -o "$delete_msg" = "Yes" ]; then
    if [ -d "${MOUNT_DIR}/${MYSQL_CPUSET}" ]; then
      proc_list=$(cat ${MOUNT_DIR}/${MYSQL_CPUSET}/cgroup.procs)
      for proc in $proc_list; do
        run_cmd "kill -9 ${proc}"
      done
      # Remove the CPU set directory.
      run_cmd "rmdir ${MOUNT_DIR}/${MYSQL_CPUSET}"
      # Unmount the cpuset virtual file system.
      run_cmd "umount ${MOUNT_DIR}"
    else
      print_info "The CPU set does not exist."
      return 1
    fi
    print_info "Done!"
  else
    # User does not wish to continue.
    print_info "Aborting program."
  fi
}


function main() {

  local MYSQL_CPUS=$MYSQL_DEFAULT_CPUS
  local RESTART_MYSQL_FLAG=0
  local DELETE_CPUSET_FLAG=0
  local MYSQL_PID=""

  # Parse command-line arguments.
  while getopts ":c:dhp:r" opt; do
    case $opt in
      c)
        MYSQL_CPUS=$OPTARG
        ;;
      d)
        DELETE_CPUSET_FLAG=1
        ;;
      h)
        usage
        return 0
        ;;
      p)
        MYSQL_PID=$OPTARG
        ;;
      r)
        RESTART_MYSQL_FLAG=1
        ;;
      \?)
        echo "Invalid option: -$OPTARG" >&2
        usage
        return 1
        ;;
      :)
        echo "Option -$OPTARG requires an argument." >&2
        usage
        return 1
        ;;
    esac
  done


  # Clean up and exit if the flag is set.
  if [ $DELETE_CPUSET_FLAG -eq 1 ]; then
    clean_all
    return 0
  fi

  # Restart MySQL.
  if [ $RESTART_MYSQL_FLAG -eq 1 ]; then
    print_info "Restarting MySQL..."
    $MYSQL_PATH restart
  fi


  # Get PID of MySQL.
  get_mysql_pid "$MYSQL_PID"
  MYSQL_PID=$_RET

  mount_cpuset

  # Make directory for MySql.
  print_info "Making a cpuset for MySQL..."
  run_cmd "mkdir -p ${MOUNT_DIR}/${MYSQL_CPUSET}"

  # Change working directory.
  run_cmd "cd ${MOUNT_DIR}/${MYSQL_CPUSET}"

  # Update the CPUs to use in the CPU set. Note that we use /bin/echo
  # explicitly (instead of "echo") because it displays write errors.
  print_info "Updating CPUs in the cpuset..."
  run_cmd "bash -c \"/bin/echo ${MYSQL_CPUS} > cpus\""

  # Attach/Rebind MySQL process to the cpuset. Note that this command
  # can only attach one PID at a time. This needs to be run every time
  # after the CPU set is modified.
  print_info "Bind MySQL process to the cpuset..."
  run_cmd "bash -c \"/bin/echo ${MYSQL_PID} > tasks\""

  print_info "Done!"
}

main "$@"
