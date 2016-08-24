// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_CONSTANTS_H_
#define LIBWEAVE_SRC_PRIVET_CONSTANTS_H_

namespace weave {
namespace privet {

namespace errors {

extern const char kInvalidClientCommitment[];
extern const char kInvalidFormat[];
extern const char kMissingAuthorization[];
extern const char kInvalidAuthorization[];
extern const char kInvalidAuthorizationScope[];
extern const char kAuthorizationExpired[];
extern const char kCommitmentMismatch[];
extern const char kUnknownSession[];
extern const char kInvalidAuthCode[];
extern const char kInvalidAuthMode[];
extern const char kInvalidRequestedScope[];
extern const char kAccessDenied[];
extern const char kInvalidParams[];
extern const char kSetupUnavailable[];
extern const char kDeviceBusy[];
extern const char kInvalidState[];
extern const char kInvalidSsid[];
extern const char kInvalidPassphrase[];
extern const char kNotFound[];
extern const char kNotImplemented[];
extern const char kAlreadyClaimed[];
}  // namespace errors
}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_CONSTANTS_H_
