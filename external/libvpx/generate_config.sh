#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to generate files in the <platform> directories needed to
# build libvpx. Every time libvpx source code is updated run this script.
#
# For example:
# $ ./generate_config.sh
#
# And this will update all the config files needed.

export LC_ALL=C
BASE_DIR=$(pwd)
LIBVPX_SRC_DIR="libvpx"
LIBVPX_CONFIG_DIR="config"

# Clean files from previous make.
function make_clean {
  make clean > /dev/null
  rm -f libvpx_srcs.txt
}

# Lint a pair of vpx_config.h and vpx_config.asm to make sure they match.
# $1 - Header file directory.
function lint_config {
  # mips does not contain any assembly so the header does not need to be
  # compared to the asm.
  if [[ "$1" != *mips* ]]; then
    $BASE_DIR/lint_config.sh \
      -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
      -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
  fi
}

# Print the configuration.
# $1 - Header file directory.
function print_config {
  $BASE_DIR/lint_config.sh -p \
    -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
    -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
}

# Print the configuration from Header file.
# This function is an abridged version of print_config which does not use
# lint_config and it does not require existence of vpx_config.asm.
# $1 - Header file directory.
function print_config_basic {
  combined_config="$(cat $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
                   | grep -E ' +[01] *$')"
  combined_config="$(echo "$combined_config" | grep -v DO1STROUNDING)"
  combined_config="$(echo "$combined_config" | sed 's/[ \t]//g')"
  combined_config="$(echo "$combined_config" | sed 's/.*define//')"
  combined_config="$(echo "$combined_config" | sed 's/0$/=no/')"
  combined_config="$(echo "$combined_config" | sed 's/1$/=yes/')"
  echo "$combined_config" | sort | uniq
}

# Generate *_rtcd.h files.
# $1 - Header file directory.
# $2 - Architecture.
function gen_rtcd_header {
  echo "Generate $LIBVPX_CONFIG_DIR/$1/*_rtcd.h files."

  # We don't properly persist the config options specificed on the configure
  # line. Until that is fixed, force them here.
  DISABLE_CONFIG="--disable-sse4_1 --disable-avx --disable-avx2"

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
  if [[ "$2" == *mips* ]]; then
    print_config_basic $1 > $BASE_DIR/$TEMP_DIR/libvpx.config
  else
    $BASE_DIR/lint_config.sh -p \
      -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
      -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm \
      -o $BASE_DIR/$TEMP_DIR/libvpx.config
  fi

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vp8_rtcd \
    $DISABLE_CONFIG \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp8/common/rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp8_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vp9_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $DISABLE_CONFIG \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp9/common/vp9_rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp9_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vpx_scale_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $DISABLE_CONFIG \
    $BASE_DIR/$LIBVPX_SRC_DIR/vpx_scale/vpx_scale_rtcd.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_scale_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.pl \
    --arch=$2 \
    --sym=vpx_dsp_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $DISABLE_CONFIG \
    $BASE_DIR/$LIBVPX_SRC_DIR/vpx_dsp/vpx_dsp_rtcd_defs.pl \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_dsp_rtcd.h

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
}

# Generate Config files. "--enable-external-build" must be set to skip
# detection of capabilities on specific targets.
# $1 - Header file directory.
# $2 - Config command line.
function gen_config_files {
  ./configure $2 > /dev/null

  # Generate vpx_config.asm. Do not create one for mips.
  if [[ "$1" != *mips* ]]; then
    if [[ "$1" == *x86* ]]; then
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h \
        | awk '{print "%define " $2 " " $3}' > vpx_config.asm
    else
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h \
        | awk '{print $2 " EQU " $3}' \
        | perl $BASE_DIR/$LIBVPX_SRC_DIR/build/make/ads2gas.pl > vpx_config.asm
    fi
  fi

  # Generate vpx_version.h
  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/version.sh "$BASE_DIR/$LIBVPX_SRC_DIR" vpx_version.h

  cp vpx_config.* vpx_version.h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1
  make_clean
  rm -rf vpx_config.* vpx_version.h
}

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

echo "Generate config files."
all_platforms="--enable-external-build --enable-realtime-only --enable-pic --disable-runtime-cpu-detect"
intel="--disable-sse4_1 --disable-avx --disable-avx2 --as=yasm"
gen_config_files x86 "--target=x86-linux-gcc ${intel} ${all_platforms}"
gen_config_files x86_64 "--target=x86_64-linux-gcc ${intel} ${all_platforms}"
gen_config_files arm "--target=armv6-linux-gcc  ${all_platforms}"
gen_config_files arm-neon "--target=armv7-linux-gcc ${all_platforms}"
gen_config_files arm64 "--force-target=armv8-linux-gcc ${all_platforms}"
gen_config_files mips32 "--target=mips32-linux-gcc --disable-dspr2 ${all_platforms}"
gen_config_files mips32-dspr2 "--target=mips32-linux-gcc --enable-dspr2 ${all_platforms}"
gen_config_files mips64 "--target=mips64-linux-gcc ${all_platforms}"
gen_config_files generic "--target=generic-gnu ${all_platforms}"

echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR

echo "Lint libvpx configuration."
lint_config x86
lint_config x86_64
lint_config arm
lint_config arm-neon
lint_config arm64
lint_config mips32
lint_config mips32-dspr2
lint_config mips64
lint_config generic

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

gen_rtcd_header x86 x86
gen_rtcd_header x86_64 x86_64
gen_rtcd_header arm armv6
gen_rtcd_header arm-neon armv7
gen_rtcd_header arm64 armv8
gen_rtcd_header mips32 mips32
gen_rtcd_header mips32-dspr2 mips32
gen_rtcd_header mips64 mips64
gen_rtcd_header generic generic

echo "Prepare Makefile."
./configure --target=generic-gnu > /dev/null
make_clean

echo "Generate X86 source list."
config=$(print_config x86)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/x86/

echo "Generate X86_64 source list."
config=$(print_config x86_64)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/x86_64/

echo "Generate ARM source list."
config=$(print_config arm)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/arm/

echo "Generate ARM NEON source list."
config=$(print_config arm-neon)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/arm-neon/

echo "Generate ARM64 source list."
config=$(print_config arm64)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/arm64/

echo "Generate MIPS source list."
config=$(print_config_basic mips32)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/mips32/

echo "Generate MIPS DSPR2 source list."
config=$(print_config_basic mips32-dspr2)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/mips32-dspr2/

echo "Generate MIPS64 source list."
config=$(print_config_basic mips64)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/mips64/

echo "Generate GENERIC source list."
config=$(print_config_basic generic)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
cp libvpx_srcs.txt $BASE_DIR/$LIBVPX_CONFIG_DIR/generic/


echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR
