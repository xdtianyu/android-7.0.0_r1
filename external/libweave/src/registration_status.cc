// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/device.h>
#include <weave/enum_to_string.h>
#include <weave/export.h>

namespace weave {

namespace {

const EnumToStringMap<GcdState>::Map kMap[] = {
    {GcdState::kUnconfigured, "unconfigured"},
    {GcdState::kConnecting, "connecting"},
    {GcdState::kConnected, "connected"},
    {GcdState::kInvalidCredentials, "invalid_credentials"},
};

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<GcdState>::EnumToStringMap()
    : EnumToStringMap(kMap) {}

}  // namespace weave
