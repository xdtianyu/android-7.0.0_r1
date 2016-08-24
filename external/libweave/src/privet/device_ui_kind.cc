// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/device_ui_kind.h"

#include <unordered_map>

#include <base/logging.h>

namespace weave {
namespace privet {

std::string GetDeviceUiKind(const std::string& manifest_id) {
  // Map of device short id to ui device kind
  static const std::unordered_map<std::string, std::string> device_kind_map = {
    // clang-format off
    {"AC", "accessPoint"},
    {"AK", "aggregator"},
    {"AM", "camera"},
    {"AB", "developmentBoard"},
    {"AH", "acHeating"},
    {"AI", "light"},
    {"AO", "lock"},
    {"AE", "printer"},
    {"AF", "scanner"},
    {"AD", "speaker"},
    {"AL", "storage"},
    {"AJ", "toy"},
    {"AA", "vendor"},
    {"AN", "video"},
    // clang-format on
  };

  CHECK_EQ(5u, manifest_id.size());
  std::string short_id = manifest_id.substr(0, 2);

  auto iter = device_kind_map.find(short_id);
  if (iter != device_kind_map.end())
    return iter->second;

  LOG(FATAL) << "Invalid model id: " << manifest_id;
  return std::string();
}

}  // namespace privet
}  // namespace weave
