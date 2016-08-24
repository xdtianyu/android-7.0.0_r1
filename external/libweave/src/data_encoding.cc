// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/data_encoding.h"

#include <memory>

#include <base/logging.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "src/string_utils.h"
#include "third_party/modp_b64/modp_b64/modp_b64.h"

namespace weave {

namespace {

inline int HexToDec(int hex) {
  int dec = -1;
  if (hex >= '0' && hex <= '9') {
    dec = hex - '0';
  } else if (hex >= 'A' && hex <= 'F') {
    dec = hex - 'A' + 10;
  } else if (hex >= 'a' && hex <= 'f') {
    dec = hex - 'a' + 10;
  }
  return dec;
}

// Helper for Base64Encode() and Base64EncodeWrapLines().
std::string Base64EncodeHelper(const void* data, size_t size) {
  std::vector<char> buffer;
  buffer.resize(modp_b64_encode_len(size));
  size_t out_size =
      modp_b64_encode(buffer.data(), static_cast<const char*>(data), size);
  return std::string{buffer.begin(), buffer.begin() + out_size};
}

}  // namespace

std::string UrlEncode(const char* data, bool encodeSpaceAsPlus) {
  std::string result;

  while (*data) {
    char c = *data++;
    // According to RFC3986 (http://www.faqs.org/rfcs/rfc3986.html),
    // section 2.3. - Unreserved Characters
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') || c == '-' || c == '.' || c == '_' ||
        c == '~') {
      result += c;
    } else if (c == ' ' && encodeSpaceAsPlus) {
      // For historical reasons, some URLs have spaces encoded as '+',
      // this also applies to form data encoded as
      // 'application/x-www-form-urlencoded'
      result += '+';
    } else {
      base::StringAppendF(&result, "%%%02X",
                          static_cast<unsigned char>(c));  // Encode as %NN
    }
  }
  return result;
}

std::string UrlDecode(const char* data) {
  std::string result;
  while (*data) {
    char c = *data++;
    int part1 = 0, part2 = 0;
    // HexToDec would return -1 even for character 0 (end of string),
    // so it is safe to access data[0] and data[1] without overrunning the buf.
    if (c == '%' && (part1 = HexToDec(data[0])) >= 0 &&
        (part2 = HexToDec(data[1])) >= 0) {
      c = static_cast<char>((part1 << 4) | part2);
      data += 2;
    } else if (c == '+') {
      c = ' ';
    }
    result += c;
  }
  return result;
}

std::string WebParamsEncode(const WebParamList& params,
                            bool encodeSpaceAsPlus) {
  std::vector<std::string> pairs;
  pairs.reserve(params.size());
  for (const auto& p : params) {
    std::string key = UrlEncode(p.first.c_str(), encodeSpaceAsPlus);
    std::string value = UrlEncode(p.second.c_str(), encodeSpaceAsPlus);
    pairs.push_back(Join("=", key, value));
  }

  return Join("&", pairs);
}

WebParamList WebParamsDecode(const std::string& data) {
  WebParamList result;
  for (const auto& p : Split(data, "&", true, true)) {
    auto pair = SplitAtFirst(p, "=", true);
    result.emplace_back(UrlDecode(pair.first.c_str()),
                        UrlDecode(pair.second.c_str()));
  }
  return result;
}

std::string Base64Encode(const void* data, size_t size) {
  return Base64EncodeHelper(data, size);
}

std::string Base64EncodeWrapLines(const void* data, size_t size) {
  std::string unwrapped = Base64EncodeHelper(data, size);
  std::string wrapped;

  for (size_t i = 0; i < unwrapped.size(); i += 64) {
    wrapped.append(unwrapped, i, 64);
    wrapped.append("\n");
  }
  return wrapped;
}

bool Base64Decode(const std::string& input, std::vector<uint8_t>* output) {
  std::string temp_buffer;
  const std::string* data = &input;
  if (input.find_first_of("\r\n") != std::string::npos) {
    base::ReplaceChars(input, "\n", "", &temp_buffer);
    base::ReplaceChars(temp_buffer, "\r", "", &temp_buffer);
    data = &temp_buffer;
  }
  // base64 decoded data has 25% fewer bytes than the original (since every
  // 3 source octets are encoded as 4 characters in base64).
  // modp_b64_decode_len provides an upper estimate of the size of the output
  // data.
  output->resize(modp_b64_decode_len(data->size()));

  size_t size_read = modp_b64_decode(reinterpret_cast<char*>(output->data()),
                                     data->data(), data->size());
  if (size_read == MODP_B64_ERROR) {
    output->resize(0);
    return false;
  }
  output->resize(size_read);

  return true;
}

}  // namespace weave
