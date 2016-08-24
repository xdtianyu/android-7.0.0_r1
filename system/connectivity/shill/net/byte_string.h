//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_NET_BYTE_STRING_H_
#define SHILL_NET_BYTE_STRING_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "shill/net/shill_export.h"

namespace shill {

// Provides a holder of a string of bytes
class SHILL_EXPORT ByteString {
 public:
  ByteString() {}
  ByteString(const ByteString& b);

  explicit ByteString(const std::vector<unsigned char>& data) : data_(data) {}

  explicit ByteString(size_t length) : data_(length) {}

  ByteString(const unsigned char* data, size_t length)
      : data_(data, data + length) {}

  ByteString(const char* data, size_t length)
      : data_(data, data + length) {}

  ByteString(const signed char* data, size_t length)
      : data_(data, data + length) {}

  ByteString(const std::string& data, bool copy_terminator)
    : data_(reinterpret_cast<const unsigned char*>(data.c_str()),
            reinterpret_cast<const unsigned char*>(data.c_str() +
                                                    data.length() +
                                                    (copy_terminator ?
                                                     1 : 0))) {}

  ByteString& operator=(const ByteString& b);

  unsigned char* GetData();
  const unsigned char* GetConstData() const;
  size_t GetLength() const;

  // Returns a ByteString containing |length| bytes from the ByteString
  // starting at |offset|.  This function truncates the returned string
  // if part (or all) of this requested data lies outside the bounds of
  // this ByteString.
  ByteString GetSubstring(size_t offset, size_t length) const;

  // Inserts a uint32_t into a ByteString in cpu-order
  static ByteString CreateFromCPUUInt32(uint32_t val);
  // Inserts a uint32_t into a ByteString in network-order
  static ByteString CreateFromNetUInt32(uint32_t val);

  // Creates a ByteString from a string of hexadecimal digits where
  // a pair of hexadecimal digits corresponds to a byte.
  // Returns a default-constructed ByteString if |hex_string| is empty
  // or not a valid string of hexadecimal digits representing a sequence
  // of bytes.
  static ByteString CreateFromHexString(const std::string& hex_string);

  // Converts to a uint32_t from a host-order value stored in the ByteString
  // Returns true on success
  bool ConvertToCPUUInt32(uint32_t* val) const;
  // Converts to a uint32_t from a network-order value stored in the ByteString
  // Returns true on success
  bool ConvertToNetUInt32(uint32_t* val) const;

  // Converts the string of bytes stored in the ByteString from network order
  // to host order in 32-bit chunks. Returns true on success or false if the
  // length of ByteString is not a multiple of 4.
  bool ConvertFromNetToCPUUInt32Array();

  // Converts the string of bytes stored in the ByteString from host order
  // to network order in 32-bit chunks. Returns true on success or false if the
  // length of ByteString is not a multiple of 4.
  bool ConvertFromCPUToNetUInt32Array();

  bool IsEmpty() const { return GetLength() == 0; }

  // Returns true if every element of |this| is zero, false otherwise.
  bool IsZero() const;

  // Perform an AND operation between each element of |this| with the
  // corresponding byte of |b|.  Returns true if both |this| and |b|
  // are the same length, and as such the operation succeeds; false
  // if they are not.  The result of the operation is stored in |this|.
  bool BitwiseAnd(const ByteString& b);

  // Perform an OR operation between each element of |this| with the
  // corresponding byte of |b|.  Returns true if both |this| and |b|
  // are the same length, and as such the operation succeeds; false
  // if they are not.  The result of the operation is stored in |this|.
  bool BitwiseOr(const ByteString& b);

  // Perform an inversion operation on each of the bits this string.
  void BitwiseInvert();

  bool Equals(const ByteString& b) const;
  void Append(const ByteString& b);
  void Clear();
  void Resize(int size);

  std::string HexEncode() const;

  // Ensures that |size| bytes are available in the payload, then copies
  // these bytes to |output|.  Returns false if |this| does not contain enough
  // data.
  bool CopyData(size_t size, void* output) const;

  static bool IsLessThan(const ByteString& lhs, const ByteString& rhs);

 private:
  typedef std::vector<unsigned char> Vector;

  // Converts the string of bytes stored in the ByteString by treating it as
  // an array of unsigned integer of type T and applying |converter| on each
  // unsigned value of type T. Return true on success or false if the length
  // ByteString is not a multiple of sizeof(T).
  template <typename T> bool ConvertByteOrderAsUIntArray(T (*converter)(T));

  Vector data_;

  // NO DISALLOW_COPY_AND_ASSIGN -- we assign ByteStrings in STL hashes
};

}  // namespace shill


#endif  // SHILL_NET_BYTE_STRING_H_
