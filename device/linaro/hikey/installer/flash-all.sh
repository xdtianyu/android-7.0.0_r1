#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "Provide the right /dev/ttyUSBX specific to recovery device"
    exit
fi

if [ ! -e $1 ]
  then
    echo "device: $1 does not exist"
    exit
fi
DEVICE_PORT=${1}
PTABLE=ptable-aosp-8g.img
if [ $# -gt 1 ]
  then
    if [ $2 == '4g' ]
      then
        PTABLE=ptable-aosp-4g.img
    fi
fi

INSTALLER_DIR="`dirname $0`"
ANDROID_TOP=${INSTALLER_DIR}/../../../../

#get out directory path
while [ $# -ne 0 ]; do
    case "$1" in
        --out) OUT_IMGDIR=$2;shift;
    esac
    shift
done

if [ -z $OUT_IMGDIR ]; then
    if [ ! -z $ANDROID_PRODUCT_OUT ]; then
        OUT_IMGDIR=${ANDROID_PRODUCT_OUT}
    else
        OUT_IMGDIR="${ANDROID_TOP}/out/target/product/hikey"
    fi
fi

if [ ! -d $OUT_IMGDIR ]; then
    echo "error in locating out directory, check if it exist"
    exit
fi
echo "android out dir:$OUT_IMGDIR"

python ${INSTALLER_DIR}/hisi-idt.py --img1=${INSTALLER_DIR}/l-loader.bin -d ${DEVICE_PORT}
fastboot flash ptable ${INSTALLER_DIR}/${PTABLE}
fastboot flash fastboot ${INSTALLER_DIR}/fip.bin
fastboot flash nvme ${INSTALLER_DIR}/nvme.img
fastboot flash boot ${OUT_IMGDIR}/boot_fat.uefi.img
fastboot flash system ${OUT_IMGDIR}/system.img
fastboot flash cache ${OUT_IMGDIR}/cache.img
fastboot flash userdata ${OUT_IMGDIR}/userdata.img
