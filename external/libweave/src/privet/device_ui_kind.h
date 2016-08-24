// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_DEVICE_UI_KIND_H_
#define LIBWEAVE_SRC_PRIVET_DEVICE_UI_KIND_H_

#include <string>

namespace weave {
namespace privet {

std::string GetDeviceUiKind(const std::string& manifest_id);

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_DEVICE_UI_KIND_H_
