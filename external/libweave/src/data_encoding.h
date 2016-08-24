// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_DATA_ENCODING_H_
#define LIBWEAVE_SRC_DATA_ENCODING_H_

#include <string>
#include <utility>
#include <vector>

namespace weave {

using WebParamList = std::vector<std::pair<std::string, std::string>>;

// Encode/escape string to be used in the query portion of a URL.
// If |encodeSpaceAsPlus| is set to true, spaces are encoded as '+' instead
// of "%20"
std::string UrlEncode(const char* data, bool encodeSpaceAsPlus);

inline std::string UrlEncode(const char* data) {
  return UrlEncode(data, true);
}

// Decodes/unescapes a URL. Replaces all %XX sequences with actual characters.
// Also replaces '+' with spaces.
std::string UrlDecode(const char* data);

// Converts a list of key-value pairs into a string compatible with
// 'application/x-www-form-urlencoded' content encoding.
std::string WebParamsEncode(const WebParamList& params, bool encodeSpaceAsPlus);

inline std::string WebParamsEncode(const WebParamList& params) {
  return WebParamsEncode(params, true);
}

// Parses a string of '&'-delimited key-value pairs (separated by '=') and
// encoded in a way compatible with 'application/x-www-form-urlencoded'
// content encoding.
WebParamList WebParamsDecode(const std::string& data);

// Encodes binary data using base64-encoding.
std::string Base64Encode(const void* data, size_t size);

// Encodes binary data using base64-encoding and wraps lines at 64 character
// boundary using LF as required by PEM (RFC 1421) specification.
std::string Base64EncodeWrapLines(const void* data, size_t size);

// Decodes the input string from Base64.
bool Base64Decode(const std::string& input, std::vector<uint8_t>* output);

// Helper wrappers to use std::string and std::vector<uint8_t> as binary data
// containers.
inline std::string Base64Encode(const std::vector<uint8_t>& input) {
  return Base64Encode(input.data(), input.size());
}
inline std::string Base64EncodeWrapLines(const std::vector<uint8_t>& input) {
  return Base64EncodeWrapLines(input.data(), input.size());
}
inline std::string Base64Encode(const std::string& input) {
  return Base64Encode(input.data(), input.size());
}
inline std::string Base64EncodeWrapLines(const std::string& input) {
  return Base64EncodeWrapLines(input.data(), input.size());
}
inline bool Base64Decode(const std::string& input, std::string* output) {
  std::vector<uint8_t> blob;
  if (!Base64Decode(input, &blob))
    return false;
  *output = std::string{blob.begin(), blob.end()};
  return true;
}

}  // namespace weave

#endif  // LIBWEAVE_SRC_DATA_ENCODING_H_
