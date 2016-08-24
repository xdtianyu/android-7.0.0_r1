#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(cd -P -- "$(dirname -- "$0")/../.." && pwd -P)

cd $ROOT_DIR

git subtree add --prefix third_party/temp_libuweave \
    https://weave.googlesource.com/weave/libuweave master --squash || exit 1

mkdir -p third_party/libuweave/src
pushd third_party
git mv -kf temp_libuweave/LICENSE libuweave/
git mv -kf temp_libuweave/src/crypto_hmac.h libuweave/src/crypto_hmac.h
git mv -kf temp_libuweave/src/macaroon* libuweave/src/
git mv -kf temp_libuweave/src/crypto_utils.* libuweave/src/
popd

git rm -rf third_party/temp_libuweave
git reset --soft weave/master
git commit -av
