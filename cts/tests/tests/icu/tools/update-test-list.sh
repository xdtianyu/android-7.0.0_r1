#!/bin/bash

# Extract a flat list of tests from a CTSv2 generated host log for use when generating the CTSv1
# required list of tests.
ZIP=$1

TMP_DIR=/tmp/update-test-list-$$
mkdir -p $TMP_DIR

unzip $ZIP host_log.txt -d $TMP_DIR

HOST_LOG=$TMP_DIR/host_log.txt

ICU_DIR=$(dirname $0)/../

grep "ModuleListener.testStarted" $HOST_LOG \
    | sed 's%.*(\([^#]\+\)#\([^)]\+\))%\1#\2%' | sort > $ICU_DIR/test-list.txt
