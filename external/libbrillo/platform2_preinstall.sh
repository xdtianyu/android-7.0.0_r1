#!/bin/bash

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

OUT=$1
shift
for v; do
  # Extract all the libbrillo sublibs from 'dependencies' section of
  # 'libbrillo-<(libbase_ver)' target in libbrillo.gypi and convert them
  # into an array of "-lbrillo-<sublib>-<v>" flags.
  sublibs=($(sed -n "
     /'target_name': 'libbrillo-<(libbase_ver)'/,/target_name/ {
       /dependencies/,/],/ {
         /libbrillo/ {
           s:[',]::g
           s:<(libbase_ver):${v}:g
           s:libbrillo:-lbrillo:
           p
         }
       }
     }" libbrillo.gypi))

  echo "GROUP ( AS_NEEDED ( ${sublibs[@]} ) )" > "${OUT}"/lib/libbrillo-${v}.so

  deps=$(<"${OUT}"/gen/libbrillo-${v}-deps.txt)
  pc="${OUT}"/lib/libbrillo-${v}.pc

  sed \
    -e "s/@BSLOT@/${v}/g" \
    -e "s/@PRIVATE_PC@/${deps}/g" \
    "libbrillo.pc.in" > "${pc}"

  deps_test=$(<"${OUT}"/gen/libbrillo-test-${v}-deps.txt)
  deps_test+=" libbrillo-${v}"
  sed \
    -e "s/@BSLOT@/${v}/g" \
    -e "s/@PRIVATE_PC@/${deps_test}/g" \
    "libbrillo-test.pc.in" > "${OUT}/lib/libbrillo-test-${v}.pc"


  deps_glib=$(<"${OUT}"/gen/libbrillo-glib-${v}-deps.txt)
  pc_glib="${OUT}"/lib/libbrillo-glib-${v}.pc

  sed \
    -e "s/@BSLOT@/${v}/g" \
    -e "s/@PRIVATE_PC@/${deps_glib}/g" \
    "libbrillo-glib.pc.in" > "${pc_glib}"
done
