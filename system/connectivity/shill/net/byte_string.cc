//
// Copyright (C) 2011 The Android Open Source Project
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

#include "shill/net/byte_string.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>

#include <base/strings/string_number_conversions.h>

using std::min;
using std::string;
using std::vector;

namespace shill {

ByteString::ByteString(const ByteString& b) {
  data_ = b.data_;
}

ByteString& ByteString::operator=(const ByteString& b) {
  data_ = b.data_;
  return *this;
}

unsigned char* ByteString::GetData() {
  return (GetLength() == 0) ? nullptr : &data_.front();
}

const unsigned char* ByteString::GetConstData() const {
  return (GetLength() == 0) ? nullptr : &data_.front();
}

size_t ByteString::GetLength() const {
  return data_.size();
}

ByteString ByteString::GetSubstring(size_t offset, size_t length) const {
  if (offset > GetLength()) {
    offset = GetLength();
  }
  if (length > GetLength() - offset) {
    length = GetLength() - offset;
  }
  return ByteString(GetConstData() + offset, length);
}

// static
ByteString ByteString::CreateFromCPUUInt32(uint32_t val) {
  return ByteString(reinterpret_cast<unsigned char*>(&val), sizeof(val));
}

// static
ByteString ByteString::CreateFromNetUInt32(uint32_t val) {
  return CreateFromCPUUInt32(ntohl(val));
}

// static
ByteString ByteString::CreateFromHexString(const string& hex_string) {
  vector<uint8_t> bytes;
  if (!base::HexStringToBytes(hex_string, &bytes)) {
    return ByteString();
  }
  return ByteString(&bytes.front(), bytes.size());
}

bool ByteString::ConvertToCPUUInt32(uint32_t* val) const {
  if (val == nullptr || GetLength() != sizeof(*val)) {
    return false;
  }
  memcpy(val, GetConstData(), sizeof(*val));

  return true;
}

bool ByteString::ConvertToNetUInt32(uint32_t* val) const {
  if (!ConvertToCPUUInt32(val)) {
    return false;
  }
  *val = ntohl(*val);
  return true;
}

template <typename T>
bool ByteString::ConvertByteOrderAsUIntArray(T (*converter)(T)) {
  size_t length = GetLength();
  if ((length % sizeof(T)) != 0) {
    return false;
  }
  for (auto i = data_.begin(); i != data_.end(); i += sizeof(T)) {
    // Take care of word alignment.
    T val;
    memcpy(&val, &(*i), sizeof(T));
    val = converter(val);
    memcpy(&(*i), &val, sizeof(T));
  }
  return true;
}

bool ByteString::ConvertFromNetToCPUUInt32Array() {
  return ConvertByteOrderAsUIntArray(ntohl);
}

bool ByteString::ConvertFromCPUToNetUInt32Array() {
  return ConvertByteOrderAsUIntArray(htonl);
}

bool ByteString::IsZero() const {
  for (const auto& i : data_) {
    if (i != 0) {
      return false;
    }
  }
  return true;
}

bool ByteString::BitwiseAnd(const ByteString& b) {
  if (GetLength() != b.GetLength()) {
    return false;
  }
  auto lhs = data_.begin();
  for (const auto& rhs : b.data_) {
    *lhs++ &= rhs;
  }
  return true;
}

bool ByteString::BitwiseOr(const ByteString& b) {
  if (GetLength() != b.GetLength()) {
    return false;
  }
  auto lhs = data_.begin();
  for (const auto& rhs : b.data_) {
    *lhs++ |= rhs;
  }
  return true;
}

void ByteString::BitwiseInvert() {
  for (auto& i : data_) {
    i = ~i;
  }
}

bool ByteString::Equals(const ByteString& b) const {
  if (GetLength() != b.GetLength()) {
    return false;
  }
  auto lhs = data_.begin();
  for (const auto& rhs : b.data_) {
    if (*lhs++ != rhs) {
      return false;
    }
  }
  return true;
}

void ByteString::Append(const ByteString& b) {
  data_.insert(data_.end(), b.data_.begin(), b.data_.end());
}

void ByteString::Clear() {
  data_.clear();
}

void ByteString::Resize(int size) {
  data_.resize(size, 0);
}

string ByteString::HexEncode() const {
  return base::HexEncode(GetConstData(), GetLength());
}

bool ByteString::CopyData(size_t size, void* output) const {
  if (output == nullptr || GetLength() < size) {
    return false;
  }
  memcpy(output, GetConstData(), size);
  return true;
}

// static
bool ByteString::IsLessThan(const ByteString& lhs, const ByteString& rhs) {
  size_t byte_count = min(lhs.GetLength(), rhs.GetLength());
  int result = memcmp(lhs.GetConstData(), rhs.GetConstData(), byte_count);
  if (result == 0) {
    return lhs.GetLength() < rhs.GetLength();
  }
  return result < 0;
}

}  // namespace shill
