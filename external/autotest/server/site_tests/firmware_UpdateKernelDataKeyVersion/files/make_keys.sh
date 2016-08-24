#!/bin/bash

. "$(dirname "$0")/common.sh"

KDATAKEY_VERSION=$1

pushd /var/tmp/faft/autest/keys

make_pair "kernel_data_key" $KERNEL_DATAKEY_ALGOID $KDATAKEY_VERSION
make_keyblock "kernel" $KERNEL_KEYBLOCK_MODE "kernel_data_key" "kernel_subkey"

popd
