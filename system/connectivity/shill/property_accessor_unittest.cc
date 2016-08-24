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

#include "shill/property_accessor.h"

#include <limits>
#include <map>
#include <string>
#include <vector>

#include <base/stl_util.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "shill/error.h"

using std::map;
using std::string;
using std::vector;
using ::testing::Return;
using ::testing::Test;

namespace shill {

TEST(PropertyAccessorTest, SignedIntCorrectness) {
  int32_t int_store = 0;
  {
    Error error;
    int32_t orig_value = int_store;
    Int32Accessor accessor(new PropertyAccessor<int32_t>(&int_store));
    EXPECT_EQ(int_store, accessor->Get(&error));

    int32_t expected_int32 = 127;
    EXPECT_TRUE(accessor->Set(expected_int32, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_int32, accessor->Get(&error));
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor->Set(expected_int32, &error));
    EXPECT_TRUE(error.IsSuccess());

    accessor->Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, accessor->Get(&error));

    int_store = std::numeric_limits<int32_t>::max();
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), accessor->Get(&error));
  }
  {
    Error error;
    Int32Accessor accessor(new ConstPropertyAccessor<int32_t>(&int_store));
    EXPECT_EQ(int_store, accessor->Get(&error));

    int32_t expected_int32 = 127;
    accessor->Set(expected_int32, &error);
    ASSERT_FALSE(error.IsSuccess());
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(int_store, accessor->Get(&error));

    int_store = std::numeric_limits<int32_t>::max();
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), accessor->Get(&error));
  }
  {
    Error error;
    Int32Accessor accessor(new ConstPropertyAccessor<int32_t>(&int_store));
    accessor->Clear(&error);
    ASSERT_FALSE(error.IsSuccess());
  }
  {
    Error error;
    Int32Accessor accessor(new WriteOnlyPropertyAccessor<int32_t>(&int_store));
    accessor->Get(&error);
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    Error error;
    int32_t expected_int32 = 127;
    WriteOnlyPropertyAccessor<int32_t> accessor(&int_store);
    EXPECT_TRUE(accessor.Set(expected_int32, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_int32, *accessor.property_);
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor.Set(expected_int32, &error));
    EXPECT_TRUE(error.IsSuccess());
    // As a write-only, the value can't be read.
    EXPECT_EQ(int32_t(), accessor.Get(&error));
    ASSERT_FALSE(error.IsSuccess());

    int_store = std::numeric_limits<int32_t>::max();
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), *accessor.property_);
  }
  {
    Error error;
    int32_t orig_value = int_store = 0;
    WriteOnlyPropertyAccessor<int32_t> accessor(&int_store);

    EXPECT_TRUE(accessor.Set(127, &error));
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, *accessor.property_);
  }
}

TEST(PropertyAccessorTest, UnsignedIntCorrectness) {
  uint32_t int_store = 0;
  {
    Error error;
    uint32_t orig_value = int_store;
    Uint32Accessor accessor(new PropertyAccessor<uint32_t>(&int_store));
    EXPECT_EQ(int_store, accessor->Get(&error));

    uint32_t expected_uint32 = 127;
    EXPECT_TRUE(accessor->Set(expected_uint32, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_uint32, accessor->Get(&error));
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor->Set(expected_uint32, &error));
    EXPECT_TRUE(error.IsSuccess());

    accessor->Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, accessor->Get(&error));

    int_store = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), accessor->Get(&error));
  }
  {
    Error error;
    Uint32Accessor accessor(new ConstPropertyAccessor<uint32_t>(&int_store));
    EXPECT_EQ(int_store, accessor->Get(&error));

    uint32_t expected_uint32 = 127;
    EXPECT_FALSE(accessor->Set(expected_uint32, &error));
    ASSERT_FALSE(error.IsSuccess());
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(int_store, accessor->Get(&error));

    int_store = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), accessor->Get(&error));
  }
  {
    Error error;
    Uint32Accessor accessor(new ConstPropertyAccessor<uint32_t>(&int_store));
    accessor->Clear(&error);
    ASSERT_FALSE(error.IsSuccess());
  }
  {
    Error error;
    Uint32Accessor accessor(
        new WriteOnlyPropertyAccessor<uint32_t>(&int_store));
    accessor->Get(&error);
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    Error error;
    uint32_t expected_uint32 = 127;
    WriteOnlyPropertyAccessor<uint32_t> accessor(&int_store);
    EXPECT_TRUE(accessor.Set(expected_uint32, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_uint32, *accessor.property_);
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor.Set(expected_uint32, &error));
    EXPECT_TRUE(error.IsSuccess());
    // As a write-only, the value can't be read.
    EXPECT_EQ(uint32_t(), accessor.Get(&error));
    ASSERT_FALSE(error.IsSuccess());

    int_store = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), *accessor.property_);
  }
  {
    Error error;
    uint32_t orig_value = int_store = 0;
    WriteOnlyPropertyAccessor<uint32_t> accessor(&int_store);

    EXPECT_TRUE(accessor.Set(127, &error));
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, *accessor.property_);
  }
}

TEST(PropertyAccessorTest, StringCorrectness) {
  string storage;
  {
    Error error;
    string orig_value = storage;
    StringAccessor accessor(new PropertyAccessor<string>(&storage));
    EXPECT_EQ(storage, accessor->Get(&error));

    string expected_string("what");
    EXPECT_TRUE(accessor->Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_string, accessor->Get(&error));
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor->Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());

    accessor->Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, accessor->Get(&error));

    storage = "nooooo";
    EXPECT_EQ(storage, accessor->Get(&error));
  }
  {
    Error error;
    StringAccessor accessor(new ConstPropertyAccessor<string>(&storage));
    EXPECT_EQ(storage, accessor->Get(&error));

    string expected_string("what");
    EXPECT_FALSE(accessor->Set(expected_string, &error));
    ASSERT_FALSE(error.IsSuccess());
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(storage, accessor->Get(&error));

    storage = "nooooo";
    EXPECT_EQ(storage, accessor->Get(&error));
  }
  {
    Error error;
    StringAccessor accessor(new ConstPropertyAccessor<string>(&storage));
    accessor->Clear(&error);
    ASSERT_FALSE(error.IsSuccess());
  }
  {
    Error error;
    StringAccessor accessor(new WriteOnlyPropertyAccessor<string>(&storage));
    accessor->Get(&error);
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    Error error;
    string expected_string = "what";
    WriteOnlyPropertyAccessor<string> accessor(&storage);
    EXPECT_TRUE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_string, *accessor.property_);
    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    // As a write-only, the value can't be read.
    EXPECT_EQ(string(), accessor.Get(&error));
    ASSERT_FALSE(error.IsSuccess());

    storage = "nooooo";
    EXPECT_EQ("nooooo", *accessor.property_);
  }
  {
    Error error;
    string orig_value = storage = "original value";
    WriteOnlyPropertyAccessor<string> accessor(&storage);
    EXPECT_TRUE(accessor.Set("new value", &error));
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, *accessor.property_);
  }
}

TEST(PropertyAccessorTest, ByteArrayCorrectness) {
  ByteArray byteArray;
  {
    Error error;
    ByteArray orig_byteArray = byteArray;
    ByteArrayAccessor accessor(new PropertyAccessor<ByteArray>(&byteArray));
    EXPECT_EQ(byteArray, accessor->Get(&error));

    ByteArray expected_byteArray({ 0x01, 0x7F, 0x80, 0xFF });
    EXPECT_TRUE(accessor->Set(expected_byteArray, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_byteArray, accessor->Get(&error));

    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor->Set(expected_byteArray, &error));
    EXPECT_TRUE(error.IsSuccess());

    accessor->Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_byteArray, accessor->Get(&error));

    byteArray = ByteArray({ 0xFF, 0x7F, 0x80, 0x00 });
    EXPECT_EQ(byteArray, accessor->Get(&error));
  }
  {
    Error error;
    ByteArrayAccessor accessor(new ConstPropertyAccessor<ByteArray>(&byteArray));
    EXPECT_EQ(byteArray, accessor->Get(&error));

    ByteArray expected_byteArray({ 0x01, 0x7F, 0x80, 0xFF });
    EXPECT_FALSE(accessor->Set(expected_byteArray, &error));
    ASSERT_FALSE(error.IsSuccess());
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(byteArray, accessor->Get(&error));

    byteArray = ByteArray({ 0xFF, 0x7F, 0x80, 0x00 });
    EXPECT_EQ(byteArray, accessor->Get(&error));
  }
  {
    Error error;
    ByteArrayAccessor accessor(new ConstPropertyAccessor<ByteArray>(&byteArray));
    accessor->Clear(&error);
    ASSERT_FALSE(error.IsSuccess());
  }
  {
    Error error;
    ByteArrayAccessor accessor(new WriteOnlyPropertyAccessor<ByteArray>(&byteArray));
    accessor->Get(&error);
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    Error error;
    ByteArray expected_byteArray({ 0x01, 0x7F, 0x80, 0xFF });
    WriteOnlyPropertyAccessor<ByteArray> accessor(&byteArray);

    EXPECT_TRUE(accessor.Set(expected_byteArray, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_byteArray, *accessor.property_);

    // Resetting to the same value should return false, but without
    // an error.
    EXPECT_FALSE(accessor.Set(expected_byteArray, &error));
    EXPECT_TRUE(error.IsSuccess());

    // As a write-only, the value can't be read.
    EXPECT_EQ(ByteArray(), accessor.Get(&error));
    EXPECT_FALSE(error.IsSuccess());

    byteArray = ByteArray({ 0xFF, 0x7F, 0x80, 0x00 });
    EXPECT_EQ(ByteArray({ 0xFF, 0x7F, 0x80, 0x00 }), *accessor.property_);
  }
  {
    Error error;
    ByteArray orig_byteArray = byteArray = ByteArray({ 0x00, 0x7F, 0x80, 0xFF });
    WriteOnlyPropertyAccessor<ByteArray> accessor(&byteArray);

    EXPECT_TRUE(accessor.Set(ByteArray({ 0xFF, 0x7F, 0x80, 0x00 }), &error));
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_byteArray, *accessor.property_);
  }
}

class StringWrapper {
 public:
  string Get(Error* /*error*/) {
    return value_;
  }
  string ConstGet(Error* /*error*/) const {
    return value_;
  }
  bool Set(const string& value, Error* /*error*/) {
    if (value_ == value) {
      return false;
    }
    value_ = value;
    return true;
  }
  void Clear(Error* /*error*/) {
    value_.clear();
  }

  string value_;
};

TEST(PropertyAccessorTest, CustomAccessorCorrectness) {
  StringWrapper wrapper;
  {
    // Custom accessor: read, write, write-same, clear, read-updated.
    // Together, write and write-same verify that the CustomAccessor
    // template passes through the value from the called function.
    Error error;
    const string orig_value = wrapper.value_ = "original value";
    CustomAccessor<StringWrapper, string> accessor(&wrapper,
                                                   &StringWrapper::Get,
                                                   &StringWrapper::Set);
    EXPECT_EQ(orig_value, accessor.Get(&error));
    EXPECT_TRUE(error.IsSuccess());

    const string expected_string = "new value";
    EXPECT_TRUE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_string, accessor.Get(&error));
    // Set to same value.
    EXPECT_FALSE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());

    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(orig_value, accessor.Get(&error));

    wrapper.value_ = "nooooo";
    EXPECT_EQ(wrapper.value_, accessor.Get(&error));
  }
  {
    // Custom read-only accessor: read, write, read-updated.
    Error error;
    CustomAccessor<StringWrapper, string> accessor(&wrapper,
                                                   &StringWrapper::Get,
                                                   nullptr);
    EXPECT_EQ(wrapper.value_, accessor.Get(&error));

    const string expected_string = "what";
    EXPECT_FALSE(accessor.Set(expected_string, &error));
    ASSERT_FALSE(error.IsSuccess());
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(wrapper.value_, accessor.Get(&error));

    wrapper.value_ = "nooooo";
    EXPECT_EQ(wrapper.value_, accessor.Get(&error));
  }
  {
    // Custom read-only accessor: clear.
    Error error;
    CustomAccessor<StringWrapper, string> accessor(&wrapper,
                                                   &StringWrapper::Get,
                                                   nullptr);
    accessor.Clear(&error);
    ASSERT_FALSE(error.IsSuccess());
  }
  {
    // Custom read-only accessor with custom clear method.
    Error error;
    CustomAccessor<StringWrapper, string> accessor(&wrapper,
                                                   &StringWrapper::Get,
                                                   nullptr,
                                                   &StringWrapper::Clear);
    wrapper.value_ = "empty this";
    accessor.Clear(&error);
    ASSERT_TRUE(error.IsSuccess());
    EXPECT_TRUE(wrapper.value_.empty());
  }
}

TEST(PropertyAccessorTest, CustomWriteOnlyAccessorWithDefault) {
  StringWrapper wrapper;
  {
    // Test reading.
    Error error;
    const string default_value = "default value";
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, nullptr, &default_value);
    wrapper.value_ = "can't read this";
    EXPECT_EQ(string(), accessor.Get(&error));
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    // Test writing.
    Error error;
    const string default_value = "default value";
    const string expected_string = "what";
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, nullptr, &default_value);
    EXPECT_TRUE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_string, wrapper.value_);
    // Set to same value. With the above, this verifies that the
    // CustomWriteOnlyAccessor template passes through the return
    // value.
    EXPECT_FALSE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // Test clearing.
    Error error;
    const string default_value = "default value";
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, nullptr, &default_value);
    accessor.Set("new value", &error);
    EXPECT_EQ("new value", wrapper.value_);
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(default_value, wrapper.value_);
  }
}

TEST(PropertyAccessorTest, CustomWriteOnlyAccessorWithClear) {
  StringWrapper wrapper;
  {
    // Test reading.
    Error error;
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, &StringWrapper::Clear, nullptr);
    wrapper.value_ = "can't read this";
    EXPECT_EQ(string(), accessor.Get(&error));
    EXPECT_TRUE(error.IsFailure());
    EXPECT_EQ(Error::kPermissionDenied, error.type());
  }
  {
    // Test writing.
    Error error;
    const string expected_string = "what";
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, &StringWrapper::Clear, nullptr);
    EXPECT_TRUE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(expected_string, wrapper.value_);
    // Set to same value. With the above, this verifies that the
    // CustomWriteOnlyAccessor template passes through the return
    // value.
    EXPECT_FALSE(accessor.Set(expected_string, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // Test clearing.
    Error error;
    CustomWriteOnlyAccessor<StringWrapper, string> accessor(
        &wrapper, &StringWrapper::Set, &StringWrapper::Clear, nullptr);
    EXPECT_TRUE(accessor.Set("new value", &error));
    EXPECT_EQ("new value", wrapper.value_);
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ("", wrapper.value_);
  }
}

TEST(PropertyAccessorTest, CustomReadOnlyAccessor) {
  StringWrapper wrapper;
  CustomReadOnlyAccessor<StringWrapper, string> accessor(
      &wrapper, &StringWrapper::ConstGet);
  const string orig_value = wrapper.value_ = "original value";
  {
    // Test reading.
    Error error;
    EXPECT_EQ(orig_value, accessor.Get(&error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // Test writing.
    Error error;
    EXPECT_FALSE(accessor.Set("new value", &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(orig_value, accessor.Get(&error));
  }
  {
    // Test writing original value -- this also fails.
    Error error;
    EXPECT_FALSE(accessor.Set(orig_value, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(orig_value, accessor.Get(&error));
  }
  {
    // Test clearing.
    Error error;
    accessor.Clear(&error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ(orig_value, accessor.Get(&error));
  }
}

class StringMapWrapper {
 public:
  void Clear(const string& key, Error* /*error*/) {
    value_.erase(key);
  }
  string Get(const string& key, Error* /*error*/) {
    EXPECT_TRUE(ContainsKey(value_, key));
    return value_[key];
  }
  bool Set(const string& key, const string& value, Error* /*error*/) {
    if (value_[key] == value) {
      return false;
    }
    value_[key] = value;
    return true;
  }

  map<string, string> value_;
};

TEST(PropertyAccessorTest, CustomMappedAccessor) {
  const string kKey = "entry_key";
  const string kValue = "entry_value";
  {
    // Test reading.
    StringMapWrapper wrapper;
    CustomMappedAccessor<StringMapWrapper, string, string> accessor(
        &wrapper, &StringMapWrapper::Clear, &StringMapWrapper::Get,
        &StringMapWrapper::Set, kKey);
    wrapper.value_[kKey] = kValue;
    Error error;
    EXPECT_EQ(kValue, accessor.Get(&error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // Test writing.
    StringMapWrapper wrapper;
    CustomMappedAccessor<StringMapWrapper, string, string> accessor(
        &wrapper, &StringMapWrapper::Clear, &StringMapWrapper::Get,
        &StringMapWrapper::Set, kKey);
    Error error;
    EXPECT_TRUE(accessor.Set(kValue, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(kValue, wrapper.value_[kKey]);
    // Set to same value. With the above, this verifies that the
    // CustomMappedAccessor template passes through the return
    // value.
    EXPECT_FALSE(accessor.Set(kValue, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    // Test clearing.
    StringMapWrapper wrapper;
    CustomMappedAccessor<StringMapWrapper, string, string> accessor(
        &wrapper, &StringMapWrapper::Clear, &StringMapWrapper::Get,
        &StringMapWrapper::Set, kKey);
    wrapper.value_[kKey] = kValue;
    Error error;
    accessor.Clear(&error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_FALSE(ContainsKey(wrapper.value_, kKey));
  }
}

}  // namespace shill
