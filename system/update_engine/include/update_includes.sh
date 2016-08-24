#!/bin/bash

#
# Copyright (C) 2016 The Android Open Source Project
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
#

DBUS_GENERATOR=$(which dbus-binding-generator)
MY_DIR=$(dirname "$0")

if [[ -z "${ANDROID_HOST_OUT}" ]]; then
  echo "You must run envsetup.sh and lunch first." >&2
  exit 1
fi

if [[ -z "${DBUS_GENERATOR}" ]]; then
  echo "DBus bindings generator not found." >&2
  exit 1
fi

set -e

# generate <kind> <dir> <xml> [xml ...]
# Generate a DBus proxy and/or proxy mock in the passed |dir| for the provided
# |xml| service files.
# The parameter |kind| determines whether it should generate the mock only
# (mock), the proxy only (proxy) or both (both).
generate() {
  local kind="$1"
  local dir="$2"
  local xmls=("${@:3}")

  mkdir -p "${MY_DIR}/${dir}"
  local outdir=$(realpath "${MY_DIR}/${dir}")
  local proxyh="${outdir}/dbus-proxies.h"
  local mockh="${outdir}/dbus-proxy-mocks.h"

  ${DBUS_GENERATOR} "${xmls[@]}" --mock="${mockh}" --proxy="${proxyh}"

  # Fix the include path to the dbus-proxies.h to include ${dir}.
  sed "s,include \"dbus-proxies.h\",include \"${dir}/dbus-proxies.h\"," \
    -i "${mockh}"

  # Fix the header guards to be independent from the checkout location.
  local guard=$(realpath "${MY_DIR}/../.." | tr '[:lower:]/ ' '[:upper:]__')
  for header in "${mockh}" "${proxyh}"; do
    sed "s,___CHROMEOS_DBUS_BINDING__${guard},___CHROMEOS_DBUS_BINDING__," \
      -i "${header}"
  done

  # Remove the files not requested.
  if [[ "${kind}" ==  "mock" ]]; then
    rm -f "${proxyh}"
  elif [[ "${kind}" == "proxy" ]]; then
    rm -f "${mockh}"
  fi
}

UE_DIR=$(realpath "${MY_DIR}/..")
SHILL_DIR=$(realpath "${UE_DIR}/../connectivity/shill")

generate mock "libcros" \
  "${UE_DIR}/dbus_bindings/org.chromium.LibCrosService.dbus-xml"

generate mock "shill" \
  "${SHILL_DIR}"/dbus_bindings/org.chromium.flimflam.{Manager,Service}.dbus-xml

echo "Done."
