// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_DBUS_SIGNATURE_H_
#define CHROMEOS_DBUS_BINDINGS_DBUS_SIGNATURE_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

namespace chromeos_dbus_bindings {

class DbusSignature {
 public:
  DbusSignature();
  virtual ~DbusSignature() = default;

  // Returns a C++ typename in |output| for a D-Bus signature in |signature|
  // and returns true on success.  Returns false otherwise.
  bool Parse(const std::string& signature, std::string* output);

  void set_object_path_typename(const std::string& object_path_typename) {
    object_path_typename_ = object_path_typename;
  }

 private:
  friend class DbusSignatureTest;
  FRIEND_TEST(DbusSignatureTest, DefaultObjectPathTypename);
  FRIEND_TEST(DbusSignatureTest, ParseSuccesses);

  // Typenames are C++ syntax types.
  static const char kArrayTypename[];
  static const char kBooleanTypename[];
  static const char kByteTypename[];
  static const char kDefaultObjectPathTypename[];
  static const char kDictTypename[];
  static const char kDoubleTypename[];
  static const char kSigned16Typename[];
  static const char kSigned32Typename[];
  static const char kSigned64Typename[];
  static const char kStringTypename[];
  static const char kUnixFdTypename[];
  static const char kUnsigned16Typename[];
  static const char kUnsigned32Typename[];
  static const char kUnsigned64Typename[];
  static const char kVariantTypename[];
  static const char kVariantDictTypename[];
  static const char kPairTypename[];
  static const char kTupleTypename[];

  // Returns the C++ type name for the next D-Bus signature in the string at
  // |signature| in |output|, as well as the next position within the string
  // that parsing should continue |next|.  It is not an error to pass a
  // pointer to |signature| or nullptr as |next|.  Returns true on success.
  bool GetTypenameForSignature(std::string::const_iterator signature,
                               std::string::const_iterator end,
                               std::string::const_iterator* next,
                               std::string* output);

  // Utility task for GetTypenameForSignature() which handles array objects
  // and decodes them into a map or vector depending on the encoded sub-elements
  // in the array.  The arguments and return values are the same
  // as GetTypenameForSignature().
  bool GetArrayTypenameForSignature(std::string::const_iterator signature,
                                    std::string::const_iterator end,
                                    std::string::const_iterator* next,
                                    std::string* output);

  // Utility task for GetTypenameForSignature() which handles STRUCT objects
  // and decodes them into a pair or tuple depending on the number of structure
  // elements.  The arguments and return values are the same
  // as GetTypenameForSignature().
  bool GetStructTypenameForSignature(std::string::const_iterator signature,
                                     std::string::const_iterator end,
                                     std::string::const_iterator* next,
                                     std::string* output);


  // The C++ typename to be used for D-Bus object pathnames.
  std::string object_path_typename_;

  DISALLOW_COPY_AND_ASSIGN(DbusSignature);
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_DBUS_SIGNATURE_H_
