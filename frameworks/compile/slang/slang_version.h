/*
 * Copyright 2011-2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_VERSION_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_VERSION_H_

#include <climits>

#define RS_DEVELOPMENT_API UINT_MAX

// API levels used by the standard Android SDK.
// MR -> Maintenance Release
// HC -> Honeycomb
// ICS -> Ice Cream Sandwich
// JB -> Jelly Bean
// KK -> KitKat
// M -> Marshmallow
enum SlangTargetAPI {
  SLANG_MINIMUM_TARGET_API = 11,
  SLANG_HC_TARGET_API = 11,
  SLANG_HC_MR1_TARGET_API = 12,
  SLANG_HC_MR2_TARGET_API = 13,
  SLANG_ICS_TARGET_API = 14,
  SLANG_ICS_MR1_TARGET_API = 15,
  SLANG_JB_TARGET_API = 16,
  SLANG_JB_MR1_TARGET_API = 17,
  SLANG_JB_MR2_TARGET_API = 18,
  SLANG_KK_TARGET_API = 19,
  SLANG_M_TARGET_API = 23,
  SLANG_N_TARGET_API = 24,
  SLANG_MAXIMUM_TARGET_API = RS_VERSION,
  SLANG_DEVELOPMENT_TARGET_API = RS_DEVELOPMENT_API
};
// Note that RS_VERSION is defined at build time (see Android.mk for details).

// API levels where particular features exist.
// . Open (feature appears at a particular level and continues to exist):
//     SLANG_FEAT_FOO_API
// . Closed (feature exists only through a particular range of API levels):
//     SLANG_FEAT_BAR_API_MIN, SLANG_FEAT_BAR_API_MAX
enum SlangFeatureAPI {
  SLANG_FEATURE_GENERAL_REDUCTION_API = SLANG_N_TARGET_API,
  SLANG_FEATURE_GENERAL_REDUCTION_HALTER_API = SLANG_DEVELOPMENT_TARGET_API,
  SLANG_FEATURE_SINGLE_SOURCE_API = SLANG_N_TARGET_API,
};

// SlangVersion refers to the released compiler version (for which certain
// behaviors could change - i.e. critical bugs fixed that may require
// additional workarounds in the backend compiler).
namespace SlangVersion {
enum {
  LEGACY = 0,
  ICS = 1400,
  JB = 1600,
  JB_MR1 = 1700,
  JB_MR2 = 1800,
  KK = 1900,
  KK_P1 = 1901,
  L = 2100,
  M = 2300,
  M_RS_OBJECT = 2310,
  N = 2400,
  CURRENT = N
};
}  // namespace SlangVersion

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_VERSION_H_  NOLINT
