#!/bin/bash

. "$(dirname "$0")/common.sh"

KSUBKEY_VERSION=$1

pushd /var/tmp/faft/autest/keys

make_pair "kernel_subkey" $KERNEL_SUBKEY_ALGOID $KSUBKEY_VERSION

popd
