// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_HTTP_CONSTANTS_H_
#define LIBWEAVE_SRC_HTTP_CONSTANTS_H_

namespace weave {
namespace http {

const int kContinue = 100;
const int kOk = 200;
const int kBadRequest = 400;
const int kDenied = 401;
const int kForbidden = 403;
const int kNotFound = 404;
const int kInternalServerError = 500;
const int kServiceUnavailable = 503;
const int kNotSupported = 501;

extern const char kAuthorization[];
extern const char kContentType[];

extern const char kJson[];
extern const char kJsonUtf8[];
extern const char kPlain[];
extern const char kWwwFormUrlEncoded[];

}  // namespace http
}  // namespace weave

#endif  // LIBWEAVE_SRC_HTTP_CONSTANTS_H_
