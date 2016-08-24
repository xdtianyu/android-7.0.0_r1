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

#ifndef SHILL_PROPERTY_STORE_UNITTEST_H_
#define SHILL_PROPERTY_STORE_UNITTEST_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/files/scoped_temp_dir.h>
#include <brillo/any.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/key_value_store.h"
#include "shill/manager.h"
#include "shill/mock_control.h"
#include "shill/mock_metrics.h"
#include "shill/property_store.h"
#include "shill/test_event_dispatcher.h"

namespace shill {

class PropertyStoreTest : public testing::TestWithParam<brillo::Any> {
 public:
  typedef ::testing::Types<bool, int16_t, int32_t, std::string, Stringmap,
                           Stringmaps, Strings, uint8_t, uint16_t, Uint16s,
                           uint32_t> PropertyTypes;

  // In real code, it's frowned upon to have non-POD static members, as there
  // can be ordering issues if your constructors have side effects.
  // These constructors don't, and declaring these as static lets me
  // autogenerate a bunch of unit test code that I would otherwise need to
  // copypaste.  So I think it's safe and worth it.
  static const brillo::Any kBoolV;
  static const brillo::Any kByteV;
  static const brillo::Any kInt16V;
  static const brillo::Any kInt32V;
  static const brillo::Any kKeyValueStoreV;
  static const brillo::Any kStringV;
  static const brillo::Any kStringmapV;
  static const brillo::Any kStringmapsV;
  static const brillo::Any kStringsV;
  static const brillo::Any kUint16V;
  static const brillo::Any kUint16sV;
  static const brillo::Any kUint32V;
  static const brillo::Any kUint64V;

  PropertyStoreTest();
  ~PropertyStoreTest() override;

  void SetUp() override;
  MOCK_METHOD1(TestCallback, void(const std::string& property_name));
  MOCK_METHOD1(GetKeyValueStoreCallback, KeyValueStore(Error* error));
  MOCK_METHOD2(SetKeyValueStoreCallback, bool(const KeyValueStore& value,
                                              Error* error));

  // Convenience overloads for GetProperty. Normally, we don't overload
  // functions. But this is extremely useful for type-parameterized tests.
  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          bool* storage, Error* error) {
    return store.GetBoolProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          int16_t* storage, Error* error) {
    return store.GetInt16Property(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          int32_t* storage, Error* error) {
    return store.GetInt32Property(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          std::string* storage, Error* error) {
    return store.GetStringProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          Stringmap* storage, Error* error) {
    return store.GetStringmapProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          Stringmaps* storage, Error* error) {
    return store.GetStringmapsProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          Strings* storage, Error* error) {
    return store.GetStringsProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          uint8_t* storage, Error* error) {
    return store.GetUint8Property(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          uint16_t* storage, Error* error) {
    return store.GetUint16Property(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          Uint16s* storage, Error* error) {
    return store.GetUint16sProperty(name, storage, error);
  }

  static bool GetProperty(const PropertyStore& store, const std::string& name,
                          uint32_t* storage, Error* error) {
    return store.GetUint32Property(name, storage, error);
  }

  // Convenience overloads for RegisterProperty. Normally, we don't overload
  // functions. But this is extremely useful for type-parameterized tests.
  static void RegisterProperty(
      PropertyStore* store, const std::string& name, bool* storage) {
    store->RegisterBool(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, int16_t* storage) {
    store->RegisterInt16(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, int32_t* storage) {
    store->RegisterInt32(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, std::string* storage) {
    store->RegisterString(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, Stringmap* storage) {
    store->RegisterStringmap(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, Stringmaps* storage) {
    store->RegisterStringmaps(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, Strings* storage) {
    store->RegisterStrings(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, uint8_t* storage) {
    store->RegisterUint8(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, uint16_t* storage) {
    store->RegisterUint16(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, Uint16s* storage) {
    store->RegisterUint16s(name, storage);
  }

  static void RegisterProperty(
      PropertyStore* store, const std::string& name, uint32_t* storage) {
    store->RegisterUint32(name, storage);
  }

 protected:
  Manager* manager() { return &manager_; }
  MockControl* control_interface() { return &control_interface_; }
  EventDispatcher* dispatcher() { return &dispatcher_; }
  MockMetrics* metrics() { return &metrics_; }
  const std::vector<Technology::Identifier>& default_technology_order() {
    return default_technology_order_;
  }

  const std::string& run_path() const { return path_; }
  const std::string& storage_path() const { return path_; }

  const std::string& internal_error() const { return internal_error_; }
  const std::string& invalid_args() const { return invalid_args_; }
  const std::string& invalid_prop() const { return invalid_prop_; }

 private:
  const std::string internal_error_;
  const std::string invalid_args_;
  const std::string invalid_prop_;
  base::ScopedTempDir dir_;
  const std::string path_;

  MockControl control_interface_;
  EventDispatcherForTest dispatcher_;
  testing::NiceMock<MockMetrics> metrics_;
  const std::vector<Technology::Identifier> default_technology_order_;
  Manager manager_;
};

}  // namespace shill

#endif  // SHILL_PROPERTY_STORE_UNITTEST_H_
