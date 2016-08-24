#!/bin/bash
FROM_HOST=cautotest
TO_HOST=cautotest-cq
POOL=pool:cq

function quiet() {
  $@ > /dev/null
}

function silent() {
  $@ > /dev/null 2>&1
}

function host_labels() {
  ./cli/atest host list --web=$FROM_HOST --parse $1 | awk -F '|' '{ print $5 }' | sed 's/Labels=//' | sed 's/, /,/g'
}

function host_platform() {
  ./cli/atest host list --web=$FROM_HOST $1 | sed 1d | awk '{ print $4; }'
}

function lock_host() {
  ./cli/atest host mod --web=$FROM_HOST -l $1
}

function create_labels() {
  ./cli/atest label create --web=$TO_HOST $1
}

function create_platform() {
  ./cli/atest label create -t --web=$TO_HOST $1
}

function create_host() {
  ./cli/atest host create --web=$TO_HOST -b $2 $1
}

function remove_host() {
  ./cli/atest host delete --web=$FROM_HOST $1
}

HOSTS_TO_MOVE=$(./cli/atest host list --web=$FROM_HOST -b $POOL | sed 1d | awk '{ print $1 }')

for host in $HOSTS_TO_MOVE
do
  # if ! silent lock_host $host; then echo $host already handled; continue; fi
  LABELS=$(host_labels $host)
  PLATFORM=$(host_platform $host)
  silent create_labels $LABELS
  silent create_platform $PLATFORM
  if create_host $host $LABELS
  then
    silent remove_host $host
    echo $host migrated
  else
    echo $host failed
  fi
done
