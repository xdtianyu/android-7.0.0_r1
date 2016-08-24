// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/dbus_signature.h"

#include <map>
#include <string>

#include <dbus/dbus-protocol.h>
#include <gtest/gtest.h>

using std::map;
using std::string;
using testing::Test;

namespace chromeos_dbus_bindings {

namespace {

// Failing signatures.
const char kEmptySignature[] = "";
const char kEmptyDictSignature[] = "a{}";
const char kMissingArraryParameterSignature[] = "a";
const char kMissingArraryParameterInnerSignature[] = "a{sa}i";
const char kOrphanDictSignature[] = "a{s{i}}";
const char kTooFewDictMembersSignature[] = "a{s}";
const char kTooManyDictMembersSignature[] = "a{sa{i}u}";
const char kUnclosedDictOuterSignature[] = "a{s";
const char kUnclosedDictInnerSignature[] = "a{a{u}";
const char kUnexpectedCloseSignature[] = "a}i{";
const char kUnknownSignature[] = "al";

// Define an object type name to disambiguate the typenames above.
const char kObjectPathTypename[] = "ObjectPathType";

}  // namespace

class DbusSignatureTest : public Test {
 protected:
  DbusSignature signature_;
};

TEST_F(DbusSignatureTest, ParseFailures) {
  for (const auto& failing_string : { kEmptySignature,
                                      kEmptyDictSignature,
                                      kMissingArraryParameterSignature,
                                      kMissingArraryParameterInnerSignature,
                                      kOrphanDictSignature,
                                      kTooFewDictMembersSignature,
                                      kTooManyDictMembersSignature,
                                      kUnclosedDictOuterSignature,
                                      kUnclosedDictInnerSignature,
                                      kUnexpectedCloseSignature,
                                      kUnknownSignature }) {
    string unused_output;
    EXPECT_FALSE(signature_.Parse(failing_string, &unused_output))
        << "Expected signature " << failing_string
        << " to fail but it succeeded";
  }
}

TEST_F(DbusSignatureTest, DefaultObjectPathTypename) {
  // The ParseSuccesses test below overrides the default object typename, so
  // test the default behavior separately.
  string output;
  EXPECT_TRUE(signature_.Parse(DBUS_TYPE_OBJECT_PATH_AS_STRING, &output));
  EXPECT_EQ(DbusSignature::kDefaultObjectPathTypename, output);
}

TEST_F(DbusSignatureTest, ParseSuccesses) {
  const map<string, string> parse_values {
    // Simple types.
    { DBUS_TYPE_BOOLEAN_AS_STRING, DbusSignature::kBooleanTypename },
    { DBUS_TYPE_BYTE_AS_STRING, DbusSignature::kByteTypename },
    { DBUS_TYPE_DOUBLE_AS_STRING, DbusSignature::kDoubleTypename },
    { DBUS_TYPE_OBJECT_PATH_AS_STRING, kObjectPathTypename },
    { DBUS_TYPE_INT16_AS_STRING, DbusSignature::kSigned16Typename },
    { DBUS_TYPE_INT32_AS_STRING, DbusSignature::kSigned32Typename },
    { DBUS_TYPE_INT64_AS_STRING, DbusSignature::kSigned64Typename },
    { DBUS_TYPE_STRING_AS_STRING, DbusSignature::kStringTypename },
    { DBUS_TYPE_UNIX_FD_AS_STRING, DbusSignature::kUnixFdTypename },
    { DBUS_TYPE_UINT16_AS_STRING, DbusSignature::kUnsigned16Typename },
    { DBUS_TYPE_UINT32_AS_STRING, DbusSignature::kUnsigned32Typename },
    { DBUS_TYPE_UINT64_AS_STRING, DbusSignature::kUnsigned64Typename },
    { DBUS_TYPE_VARIANT_AS_STRING, DbusSignature::kVariantTypename },

    // Complex types.
    { "ab",             "std::vector<bool>" },
    { "ay",             "std::vector<uint8_t>" },
    { "aay",            "std::vector<std::vector<uint8_t>>" },
    { "ao",             "std::vector<ObjectPathType>" },
    { "a{oa{sa{sv}}}",  "std::map<ObjectPathType, std::map<std::string, "
                          "brillo::VariantDictionary>>" },
    { "a{os}",          "std::map<ObjectPathType, std::string>" },
    { "as",             "std::vector<std::string>" },
    { "a{ss}",          "std::map<std::string, std::string>" },
    { "a{sa{ss}}",      "std::map<std::string, std::map<std::string, "
                          "std::string>>"},
    { "a{sa{sv}}",      "std::map<std::string, brillo::VariantDictionary>" },
    { "a{sv}",          "brillo::VariantDictionary" },
    { "a{sv}Garbage",   "brillo::VariantDictionary" },
    { "at",             "std::vector<uint64_t>" },
    { "a{iv}",          "std::map<int32_t, brillo::Any>" },
    { "(ib)",           "std::tuple<int32_t, bool>" },
    { "(ibs)",          "std::tuple<int32_t, bool, std::string>" },
  };
  signature_.set_object_path_typename(kObjectPathTypename);
  for (const auto& parse_test : parse_values) {
    string output;
    EXPECT_TRUE(signature_.Parse(parse_test.first, &output))
        << "Expected signature " << parse_test.first
        << " to succeed but it failed.";
    EXPECT_EQ(parse_test.second, output)
        << "Expected typename for " << parse_test.first
        << " to be " << parse_test.second << " but instead it was " << output;
  }
}

}  // namespace chromeos_dbus_bindings
