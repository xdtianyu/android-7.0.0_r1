#!/bin/bash

BACKUP_IFS=$IFS
IFS=$(echo -en "\n\b")

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
UBOOT_DIR=$(dir_with_file u-boot-edison.img \
    "${LOCAL_DIR}"/uboot_firmware \
    "${LOCAL_DIR}")
IFWI_DIR=$(dir_with_file edison_dnx_fwr.bin \
    "${LOCAL_DIR}"/ifwi_firmware \
    "${LOCAL_DIR}")

if [ $? -ne 0 ]; then
    exit 1
fi

GETOPTS="$(which getopt)"
if [[ "$OSTYPE" == "darwin"* ]] ; then READLINK=greadlink; GETOPTS="$(brew list gnu-getopt | grep bin/getopt)"; else READLINK=readlink;fi;

if [[ "$OSTYPE" == "cygwin" ]] ;
then
	TEMP_DIR="$(dirname $($READLINK -f "$0"))"
	UBOOT_DIR="$(cygpath -m ${UBOOT_DIR})"
	IFWI_DIR="$(cygpath -m ${IFWI_DIR})"
else
	UBOOT_DIR=${UBOOT_DIR//' '/'\ '}
	IFWI_DIR=${IFWI_DIR//' '/'\ '}
fi;

LOG_FILENAME="flash.log"

function print-usage {
	cat << EOF
Usage: ${0##*/} [-h][--help]
Update all software and restore board to its initial state.
 -h,--help     display this help and exit.
EOF
	exit -5
}

function flash-debug {
	echo "DEBUG: lsusb"
	lsusb
}

function flash-ifwi {
	if [ -x "$(which xfstk-dldr-solo)" ]; then
		flash-ifwi-xfstk
	else
		echo "!!! You should install xfstk tools, please visit http://xfstk.sourceforge.net/"
		echo "!!! Alternatively, see the Edison-Brillo web for information on using Phone Flash Tool Lite"
		exit -1
	fi
}

function flash-ifwi-xfstk {
	XFSTK_PARAMS=" --gpflags 0x80000007 --osimage ${UBOOT_DIR}/u-boot-edison.img"
	XFSTK_PARAMS="${XFSTK_PARAMS} --fwdnx ${IFWI_DIR}/edison_dnx_fwr.bin"
	XFSTK_PARAMS="${XFSTK_PARAMS} --fwimage ${IFWI_DIR}/edison_ifwi-dbg-00.bin"
	XFSTK_PARAMS="${XFSTK_PARAMS} --osdnx ${IFWI_DIR}/edison_dnx_osr.bin"

	eval xfstk-dldr-solo ${XFSTK_PARAMS}
	if [ $? -ne 0 ];
	then
		echo "Xfstk tool error"
		flash-debug
		exit -1
	fi
}

# Execute old getopt to have long options support
ARGS=$($GETOPTS -o hv -l "recovery,help" -n "${0##*/}" -- "$@");
#Bad arguments
if [ $? -ne 0 ]; then print-usage ; fi;
eval set -- "$ARGS";

while true; do
	case "$1" in
		-h|--help) shift; print-usage;;
		--) shift; break;;
	esac
done

echo "** Flashing Edison Board $(date) **" >> ${LOG_FILENAME}


if [[ "$OSTYPE" == "darwin"* ]] ; then
	echo "Recovery mode is only available on windows and linux";
	exit -3
fi

echo "Starting Recovery mode"
echo "Please plug and reboot the board"
echo "Flashing IFWI"
flash-ifwi
echo "Recovery Success..."
echo "You can now try a regular flash"

IFS=${BACKUP_IFS}
