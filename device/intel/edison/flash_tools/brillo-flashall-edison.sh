#!/bin/bash

# Only execute this script on a Brillo provisioned Edison.
# See your Brillo-Edison online information for initial provisioning and recovery.

set -e

function dir_with_file() {
    local file=${1}; shift
    local dir;
    for dir; do
        if [ -z "${dir}" ]; then continue; fi
        if [ -r "${dir}/${file}" ]; then
            echo ${dir}
            return
        fi
    done
    echo "Could not find ${file}, looked in $@" >&2
    return 1
}

LOCAL_DIR=$(dirname "${0}")

# Location of where the Brillo OS image is built.
OS=$(dir_with_file boot.img \
    "${ANDROID_PROVISION_OS_PARTITIONS}" \
    "${LOCAL_DIR}" \
    "${BRILLO_OUT_DIR}" \
    "${ANDROID_PRODUCT_OUT}")

# Location of binary blobs supplied by the vendor.
VENDOR=$(dir_with_file u-boot-edison.bin \
    "${ANDROID_PROVISION_VENDOR_PARTITIONS}" \
    "${LOCAL_DIR}" \
    "${ANDROID_BUILD_TOP}/vendor/bsp/intel/edison/uboot_firmware")

fastboot flash gpt     "${VENDOR}"/gpt.bin \
	flash u-boot   "${VENDOR}"/u-boot-edison.bin \
	flash boot_a   "${OS}"/boot.img \
	flash system_a "${OS}"/system.img \
	flash boot_b   "${OS}"/boot.img \
	flash system_b "${OS}"/system.img \
	flash userdata "${OS}"/userdata.img \
	oem set_active 0 "$@"

