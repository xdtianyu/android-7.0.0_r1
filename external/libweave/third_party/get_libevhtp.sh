#!/bin/bash
# Copyright 2016 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Make libevhtp.
# Example uses libevhtp to implement HTTPS server. This step could be
# replaced with apt-get in future (Debian jessie, Ubuntu vivid).
cd $(dirname "$0")
THIRD_PARTY=$(pwd)

LIBEVHTP_VERSION=1.2.11n

mkdir -p include lib

rm -rf $THIRD_PARTY/libevhtp
curl -L https://github.com/ellzey/libevhtp/archive/$LIBEVHTP_VERSION.tar.gz | tar xz || exit 1
mv libevhtp-$LIBEVHTP_VERSION $THIRD_PARTY/libevhtp || exit 1
cd $THIRD_PARTY/libevhtp || exit 1

cmake -D EVHTP_DISABLE_REGEX:BOOL=ON . || exit 1
make evhtp || exit 1

cp -rf *.h $THIRD_PARTY/include/ || exit 1
cp -f libevhtp.a $THIRD_PARTY/lib/ || exit 1

rm -rf $THIRD_PARTY/libevhtp
