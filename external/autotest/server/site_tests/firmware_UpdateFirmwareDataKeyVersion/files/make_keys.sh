#!/bin/bash

. "$(dirname "$0")/common.sh"

FKEY_VERSION=$1

pushd /var/tmp/faft/autest/keys

make_pair "firmware_data_key" $FIRMWARE_DATAKEY_ALGOID $FKEY_VERSION
make_pair "dev_firmware_data_key" $DEV_FIRMWARE_DATAKEY_ALGOID $FKEY_VERSION

make_keyblock "firmware" $FIRMWARE_KEYBLOCK_MODE "firmware_data_key" "root_key"
make_keyblock "dev_firmware" $DEV_FIRMWARE_KEYBLOCK_MODE \
  "dev_firmware_data_key" "root_key"

popd
