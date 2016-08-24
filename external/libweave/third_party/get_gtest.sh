#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Make gtest and gmock.
cd $(dirname "$0")
THIRD_PARTY=$(pwd)

mkdir -p include lib

rm -rf $THIRD_PARTY/googletest
git clone https://github.com/google/googletest.git || exit 1
cd googletest

# gtest is in process of changing of dir structure and it has broken build
# files. So this is temporarily workaround to fix that.
git reset --hard 82b11b8cfcca464c2ac74b623d04e74452e74f32
mv googletest googlemock/gtest

cd $THIRD_PARTY/googletest/googlemock/gtest/make || exit 1
make gtest.a || exit 1
cp -rf ../include/* $THIRD_PARTY/include/ || exit 1
cp -rf gtest.a $THIRD_PARTY/lib/ || exit 1

cd $THIRD_PARTY/googletest/googlemock/make || exit 1
make gmock.a || exit 1
cp -rf ../include/* $THIRD_PARTY/include/ || exit 1
cp -rf gmock.a $THIRD_PARTY/lib/ || exit 1

rm -rf $THIRD_PARTY/googletest
