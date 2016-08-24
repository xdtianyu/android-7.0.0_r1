#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Instead of this script, try running "make all -j".
# TODO: Delete this file after 15-feb-2016.

DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(cd -P -- "$(dirname -- "$0")/.." && pwd -P)

sudo apt-get update && sudo apt-get install ${APT_GET_OPTS} \
  autoconf \
  automake \
  binutils \
  cmake \
  g++ \
  hostapd \
  libavahi-client-dev \
  libcurl4-openssl-dev \
  libevent-dev \
  libexpat1-dev \
  libnl-3-dev \
  libnl-route-3-dev \
  libssl-dev \
  libtool \
  || exit 1
