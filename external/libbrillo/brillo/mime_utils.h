// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MIME_UTILS_H_
#define LIBBRILLO_BRILLO_MIME_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include <base/compiler_specific.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>

namespace brillo {
namespace mime {

namespace types {
// Main MIME type categories
BRILLO_EXPORT extern const char kApplication[];        // application
BRILLO_EXPORT extern const char kAudio[];              // audio
BRILLO_EXPORT extern const char kImage[];              // image
BRILLO_EXPORT extern const char kMessage[];            // message
BRILLO_EXPORT extern const char kMultipart[];          // multipart
BRILLO_EXPORT extern const char kText[];               // test
BRILLO_EXPORT extern const char kVideo[];              // video
}  // namespace types

namespace parameters {
// Common MIME parameters
BRILLO_EXPORT extern const char kCharset[];            // charset=...
}  // namespace parameters

namespace image {
// Common image MIME types
BRILLO_EXPORT extern const char kJpeg[];               // image/jpeg
BRILLO_EXPORT extern const char kPng[];                // image/png
BRILLO_EXPORT extern const char kBmp[];                // image/bmp
BRILLO_EXPORT extern const char kTiff[];               // image/tiff
BRILLO_EXPORT extern const char kGif[];                // image/gif
}  // namespace image

namespace text {
// Common text MIME types
BRILLO_EXPORT extern const char kPlain[];              // text/plain
BRILLO_EXPORT extern const char kHtml[];               // text/html
BRILLO_EXPORT extern const char kXml[];                // text/xml
}  // namespace text

namespace application {
// Common application MIME types
// application/octet-stream
BRILLO_EXPORT extern const char kOctet_stream[];
// application/json
BRILLO_EXPORT extern const char kJson[];
// application/x-www-form-urlencoded
BRILLO_EXPORT extern const char kWwwFormUrlEncoded[];
// application/x-protobuf
BRILLO_EXPORT extern const char kProtobuf[];
}  // namespace application

namespace multipart {
// Common multipart MIME types
// multipart/form-data
BRILLO_EXPORT extern const char kFormData[];
// multipart/mixed
BRILLO_EXPORT extern const char kMixed[];
}  // namespace multipart

using Parameters = std::vector<std::pair<std::string, std::string>>;

// Combine a MIME type, subtype and parameters into a MIME string.
// e.g. Combine("text", "plain", {{"charset", "utf-8"}}) will give:
//      "text/plain; charset=utf-8"
BRILLO_EXPORT std::string Combine(
    const std::string& type,
    const std::string& subtype,
    const Parameters& parameters = {}) WARN_UNUSED_RESULT;

// Splits a MIME string into type and subtype.
// "text/plain;charset=utf-8" => ("text", "plain")
BRILLO_EXPORT bool Split(const std::string& mime_string,
                         std::string* type,
                         std::string* subtype);

// Splits a MIME string into type, subtype, and parameters.
// "text/plain;charset=utf-8" => ("text", "plain", {{"charset","utf-8"}})
BRILLO_EXPORT bool Split(const std::string& mime_string,
                         std::string* type,
                         std::string* subtype,
                         Parameters* parameters);

// Returns the MIME type from MIME string.
// "text/plain;charset=utf-8" => "text"
BRILLO_EXPORT std::string GetType(const std::string& mime_string);

// Returns the MIME sub-type from MIME string.
// "text/plain;charset=utf-8" => "plain"
BRILLO_EXPORT std::string GetSubtype(const std::string& mime_string);

// Returns the MIME parameters from MIME string.
// "text/plain;charset=utf-8" => {{"charset","utf-8"}}
BRILLO_EXPORT Parameters GetParameters(const std::string& mime_string);

// Removes parameters from a MIME string
// "text/plain;charset=utf-8" => "text/plain"
BRILLO_EXPORT std::string RemoveParameters(
    const std::string& mime_string) WARN_UNUSED_RESULT;

// Appends a parameter to a MIME string.
// "text/plain" => "text/plain; charset=utf-8"
BRILLO_EXPORT std::string AppendParameter(
    const std::string& mime_string,
    const std::string& paramName,
    const std::string& paramValue) WARN_UNUSED_RESULT;

// Returns the value of a parameter on a MIME string (empty string if missing).
// ("text/plain;charset=utf-8","charset") => "utf-8"
BRILLO_EXPORT std::string GetParameterValue(const std::string& mime_string,
                                            const std::string& paramName);

}  // namespace mime
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MIME_UTILS_H_
