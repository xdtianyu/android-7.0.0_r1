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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_CONSTANTS_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_CONSTANTS_H_

#include <stdint.h>

#include <limits>

#include "update_engine/update_metadata.pb.h"

namespace chromeos_update_engine {

// The major version used by Chrome OS.
extern const uint64_t kChromeOSMajorPayloadVersion;

// The major version used by Brillo.
extern const uint64_t kBrilloMajorPayloadVersion;

// The minor version used for all full payloads.
extern const uint32_t kFullPayloadMinorVersion;

// The minor version used by the in-place delta generator algorithm.
extern const uint32_t kInPlaceMinorPayloadVersion;

// The minor version used by the A to B delta generator algorithm.
extern const uint32_t kSourceMinorPayloadVersion;

// The minor version that allows per-operation source hash.
extern const uint32_t kOpSrcHashMinorPayloadVersion;

// The minor version that allows IMGDIFF operation.
extern const uint32_t kImgdiffMinorPayloadVersion;


// The kernel and rootfs partition names used by the BootControlInterface when
// handling update payloads with a major version 1. The names of the updated
// partitions are include in the payload itself for major version 2.
extern const char kLegacyPartitionNameKernel[];
extern const char kLegacyPartitionNameRoot[];

extern const char kBspatchPath[];
extern const char kDeltaMagic[4];

// The list of compatible SHA256 hashes of zlib source code.
// This is used to check if the source image have a compatible zlib (produce
// same compressed result given the same input).
// When a new fingerprint is found, please examine the changes in zlib source
// carefully and determine if it's still compatible with previous version, if
// yes then add the new fingerprint to this array, otherwise remove all previous
// fingerprints in the array first, and only include the new fingerprint.
extern const char kCompatibleZlibFingerprint[2][65];

// A block number denoting a hole on a sparse file. Used on Extents to refer to
// section of blocks not present on disk on a sparse file.
const uint64_t kSparseHole = std::numeric_limits<uint64_t>::max();

// Return the name of the operation type.
const char* InstallOperationTypeName(InstallOperation_Type op_type);

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_CONSTANTS_H_
