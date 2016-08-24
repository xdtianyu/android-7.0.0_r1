// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_PROPERTY_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_PROPERTY_H_

#include <brillo/dbus/data_serialization.h>
#include <dbus/property.h>

namespace brillo {
namespace dbus_utils {

// Re-implementation of dbus::Property<T> that can handle any type supported by
// D-Bus data serialization layer, such as vectors, maps, tuples, etc.
// This class is pretty much a copy of dbus::Property<T> from dbus/property.h
// except that it provides the implementations for PopValueFromReader and
// AppendSetValueToWriter.
template<class T>
class Property : public dbus::PropertyBase {
 public:
  Property() = default;

  // Retrieves the cached value.
  const T& value() const { return value_; }

  // Requests an updated value from the remote object incurring a
  // round-trip. |callback| will be called when the new value is available.
  // This may not be implemented by some interfaces.
  void Get(dbus::PropertySet::GetCallback callback) {
    property_set()->Get(this, callback);
  }

  // Synchronous vesion of Get().
  bool GetAndBlock() {
    return property_set()->GetAndBlock(this);
  }

  // Requests that the remote object change the property value to |value|,
  // |callback| will be called to indicate the success or failure of the
  // request, however the new value may not be available depending on the
  // remote object.
  void Set(const T& value, dbus::PropertySet::SetCallback callback) {
    set_value_ = value;
    property_set()->Set(this, callback);
  }

  // Synchronous version of Set().
  bool SetAndBlock(const T& value) {
    set_value_ = value;
    return property_set()->SetAndBlock(this);
  }

  // Method used by PropertySet to retrieve the value from a MessageReader,
  // no knowledge of the contained type is required, this method returns
  // true if its expected type was found, false if not.
  bool PopValueFromReader(dbus::MessageReader* reader) override {
    return PopVariantValueFromReader(reader, &value_);
  }

  // Method used by PropertySet to append the set value to a MessageWriter,
  // no knowledge of the contained type is required.
  // Implementation provided by specialization.
  void AppendSetValueToWriter(dbus::MessageWriter* writer) override {
    AppendValueToWriterAsVariant(writer, set_value_);
  }

  // Method used by test and stub implementations of dbus::PropertySet::Set
  // to replace the property value with the set value without using a
  // dbus::MessageReader.
  void ReplaceValueWithSetValue() override {
    value_ = set_value_;
    property_set()->NotifyPropertyChanged(name());
  }

  // Method used by test and stub implementations to directly set the
  // value of a property.
  void ReplaceValue(const T& value) {
    value_ = value;
    property_set()->NotifyPropertyChanged(name());
  }

 private:
  // Current cached value of the property.
  T value_;

  // Replacement value of the property.
  T set_value_;
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_PROPERTY_H_
