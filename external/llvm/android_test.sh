#!/bin/bash

# Android-specific configuration details are kept in test/lit.site.cfg

# Set resource limits
ulimit -t 600
ulimit -d 512000
ulimit -m 512000
ulimit -s 8192

if [ -z $ANDROID_BUILD_TOP ]; then
# Use this script's location to determine the actual top-level directory.
export ANDROID_BUILD_TOP="$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ && pwd )"
fi

if [ ! -d $ANDROID_BUILD_TOP/out/test/host/linux-x86/obj/test_llvm ]; then
  mkdir -p $ANDROID_BUILD_TOP/out/test/host/linux-x86/obj/test_llvm
fi

python ./utils/lit/lit.py -s -v ./test
