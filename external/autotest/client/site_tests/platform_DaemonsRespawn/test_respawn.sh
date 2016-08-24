#!/bin/sh

# This script will kill some important system daemons and make sure
# that they have been automatically restarted.

OLD_PID=0
PID=0

# Time to wait for upstart to restart a daemon in seconds
RESTART_TIMEOUT=5

# Verifies that the given job is running and returns the pid.
get_job_pid() {
  local upstart_job=$1
  local daemon=$2
  local status=""
  local upstart_pid=""
  local pgrep_pid=""

  PID=0

  # First make sure that upstart thinks it is running.
  status="$(initctl status "$upstart_job")"
  echo "$status" | grep "start/running"
  if [ $? -ne 0 ] ; then
    echo "Job $upstart_job is not running."
    return 1
  fi

  # Now make sure that upstart has the pid that we would expect for this job
  local upstart_pid=$(echo $status | awk '{ print $NF }')
  if [ -z "$upstart_pid" ] ; then
    echo "Upstart not able to track pid for job: $upstart_job"
    return 1
  fi
  local pgrep_pid=$(pgrep -o $daemon)
  if [ -z "$pgrep_pid" ] ; then
    echo "Unable to find running job for daemon: $daemon"
    return 1
  fi
  if [ "$upstart_pid" != "$pgrep_pid" ] ; then
    echo "Upstart and daemon pids don't match: $upstart_pid vs $pgrep_pid"
    return 1
  fi

  # Everything checks out.
  PID=$upstart_pid
}

# The set of jobs (and corresponding daemon names) to test.
# TODO: Test more jobs that have the respawn stanza
UPSTART_JOBS_TO_TEST="udev:udevd"

for job in $UPSTART_JOBS_TO_TEST ; do

  JOB=$(echo "$job" | awk -F':' '{ print $1 }')
  DAEMON=$(echo "$job" | awk -F':' '{ print $2 }')

  get_job_pid "$JOB" "$DAEMON"
  if [ $PID -le 0 ] ; then
    echo "Error: It looks like job '$JOB' is not running."
    exit 255
  fi

  OLD_PID=$PID
  kill -KILL $PID

  for x in $(seq ${RESTART_TIMEOUT}); do
    sleep 1

    get_job_pid "$JOB" "$DAEMON"
    if [ $PID -gt 0 ] ; then
      break
    fi
  done

  if [ $PID -le 0 ] ; then
    echo "Error: Job '$JOB' was not respawned properly."
    exit 255
  fi
  if [ $PID -eq $OLD_PID ] ; then
    echo "Error: Job '$JOB' retained the same pid; something went wrong."
    exit 255
  fi

done

exit 0
