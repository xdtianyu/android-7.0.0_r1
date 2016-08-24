//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/payload_consumer/payload_constants.h"

namespace chromeos_update_engine {

const uint64_t kChromeOSMajorPayloadVersion = 1;
const uint64_t kBrilloMajorPayloadVersion = 2;

const uint32_t kFullPayloadMinorVersion = 0;
const uint32_t kInPlaceMinorPayloadVersion = 1;
const uint32_t kSourceMinorPayloadVersion = 2;
const uint32_t kOpSrcHashMinorPayloadVersion = 3;
const uint32_t kImgdiffMinorPayloadVersion = 4;

const char kLegacyPartitionNameKernel[] = "boot";
const char kLegacyPartitionNameRoot[] = "system";

const char kDeltaMagic[4] = {'C', 'r', 'A', 'U'};
const char kBspatchPath[] = "bspatch";

// The zlib in Android and Chrome OS are currently compatible with each other,
// so they are sharing the same array, but if in the future they are no longer
// compatible with each other, we coule make the same change on the other one to
// make them compatible again or use ifdef here.
const char kCompatibleZlibFingerprint[][65] = {
    "ea973605ccbbdb24f59f449c5f65861a1a9bc7a4353377aaaa06cb3e0f1cfbd7",
    "3747fa404cceb00a5ec3606fc779510aaa784d5864ab1d5c28b9e267c40aad5c",
};

const char* InstallOperationTypeName(InstallOperation_Type op_type) {
  switch (op_type) {
    case InstallOperation::BSDIFF:
      return "BSDIFF";
    case InstallOperation::MOVE:
      return "MOVE";
    case InstallOperation::REPLACE:
      return "REPLACE";
    case InstallOperation::REPLACE_BZ:
      return "REPLACE_BZ";
    case InstallOperation::SOURCE_COPY:
      return "SOURCE_COPY";
    case InstallOperation::SOURCE_BSDIFF:
      return "SOURCE_BSDIFF";
    case InstallOperation::ZERO:
      return "ZERO";
    case InstallOperation::DISCARD:
      return "DISCARD";
    case InstallOperation::REPLACE_XZ:
      return "REPLACE_XZ";
    case InstallOperation::IMGDIFF:
      return "IMGDIFF";
  }
  return "<unknown_op>";
}

};  // namespace chromeos_update_engine
