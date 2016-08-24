// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_OPENSSL_UTILS_H_
#define LIBWEAVE_SRC_PRIVET_OPENSSL_UTILS_H_

#include <string>
#include <vector>

namespace weave {
namespace privet {

const size_t kSha256OutputSize = 32;

std::vector<uint8_t> HmacSha256(const std::vector<uint8_t>& key,
                                const std::vector<uint8_t>& data);
}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_OPENSSL_UTILS_H_
