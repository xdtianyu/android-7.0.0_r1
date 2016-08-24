#!/bin/bash
#
# Copyright 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

OUT=$1
v=$2

deps=$(<"${OUT}"/gen/libwebserv-${v}-deps.txt)
sed \
  -e "s/@BSLOT@/${v}/g" \
  -e "s/@PRIVATE_PC@/${deps}/g" \
  "libwebserv/libwebserv.pc.in" > "${OUT}/lib/libwebserv-${v}.pc"
