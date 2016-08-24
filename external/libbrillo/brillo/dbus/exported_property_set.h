// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_EXPORTED_PROPERTY_SET_H_
#define LIBBRILLO_BRILLO_DBUS_EXPORTED_PROPERTY_SET_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <brillo/any.h>
#include <brillo/brillo_export.h>
#include <brillo/dbus/dbus_signal.h>
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <brillo/variant_dictionary.h>
#include <dbus/exported_object.h>
#include <dbus/message.h>

namespace brillo {

namespace dbus_utils {

// This class may be used to implement the org.freedesktop.DBus.Properties
// interface.  It sends the update signal on property updates:
//
//   org.freedesktop.DBus.Properties.PropertiesChanged (
//       STRING interface_name,
//       DICT<STRING,VARIANT> changed_properties,
//       ARRAY<STRING> invalidated_properties);
//
//
// and implements the required methods of the interface:
//
//   org.freedesktop.DBus.Properties.Get(in STRING interface_name,
//                                       in STRING property_name,
//                                       out VARIANT value);
//   org.freedesktop.DBus.Properties.Set(in STRING interface_name,
//                                       in STRING property_name,
//                                       in VARIANT value);
//   org.freedesktop.DBus.Properties.GetAll(in STRING interface_name,
//                                          out DICT<STRING,VARIANT> props);
//
//  This class is very similar to the PropertySet class in Chrome, except that
//  it allows objects to expose properties rather than to consume them.
//  It is used as part of DBusObject to implement D-Bus object properties on
//  registered interfaces. See description of DBusObject class for more details.

class DBusInterface;
class DBusObject;

class BRILLO_EXPORT ExportedPropertyBase {
 public:
  enum class Access {
    kReadOnly,
    kWriteOnly,
    kReadWrite,
  };

  ExportedPropertyBase() = default;
  virtual ~ExportedPropertyBase() = default;

  using OnUpdateCallback = base::Callback<void(const ExportedPropertyBase*)>;

  // Called by ExportedPropertySet to register a callback.  This callback
  // triggers ExportedPropertySet to send a signal from the properties
  // interface of the exported object.
  virtual void SetUpdateCallback(const OnUpdateCallback& cb);

  // Returns the contained value as Any.
  virtual brillo::Any GetValue() const = 0;

  virtual bool SetValue(brillo::ErrorPtr* error,
                        const brillo::Any& value) = 0;

  void SetAccessMode(Access access_mode);
  Access GetAccessMode() const;

 protected:
  // Notify the listeners of OnUpdateCallback that the property has changed.
  void NotifyPropertyChanged();

 private:
  OnUpdateCallback on_update_callback_;
  // Default to read-only.
  Access access_mode_{Access::kReadOnly};
};

class BRILLO_EXPORT ExportedPropertySet {
 public:
  using PropertyWriter = base::Callback<void(VariantDictionary* dict)>;

  explicit ExportedPropertySet(dbus::Bus* bus);
  virtual ~ExportedPropertySet() = default;

  // Called to notify ExportedPropertySet that the Properties interface of the
  // D-Bus object has been exported successfully and property notification
  // signals can be sent out.
  void OnPropertiesInterfaceExported(DBusInterface* prop_interface);

  // Return a callback that knows how to write this property set's properties
  // to a message.  This writer retains a weak pointer to this, and must
  // only be invoked on the same thread as the rest of ExportedPropertySet.
  PropertyWriter GetPropertyWriter(const std::string& interface_name);

  void RegisterProperty(const std::string& interface_name,
                        const std::string& property_name,
                        ExportedPropertyBase* exported_property);

  // D-Bus methods for org.freedesktop.DBus.Properties interface.
  VariantDictionary HandleGetAll(const std::string& interface_name);
  bool HandleGet(brillo::ErrorPtr* error,
                 const std::string& interface_name,
                 const std::string& property_name,
                 brillo::Any* result);
  // While Properties.Set has a handler to complete the interface,  we don't
  // support writable properties.  This is almost a feature, since bindings for
  // many languages don't support errors coming back from invalid writes.
  // Instead, use setters in exposed interfaces.
  bool HandleSet(brillo::ErrorPtr* error,
                 const std::string& interface_name,
                 const std::string& property_name,
                 const brillo::Any& value);
  // Returns a string-to-variant map of all the properties for the given
  // interface and their values.
  VariantDictionary GetInterfaceProperties(
      const std::string& interface_name) const;

 private:
  // Used to write the dictionary of string->variant to a message.
  // This dictionary represents the property name/value pairs for the
  // given interface.
  BRILLO_PRIVATE void WritePropertiesToDict(const std::string& interface_name,
                                            VariantDictionary* dict);
  BRILLO_PRIVATE void HandlePropertyUpdated(
      const std::string& interface_name,
      const std::string& property_name,
      const ExportedPropertyBase* exported_property);

  dbus::Bus* bus_;  // weak; owned by outer DBusObject containing this object.
  // This is a map from interface name -> property name -> pointer to property.
  std::map<std::string, std::map<std::string, ExportedPropertyBase*>>
      properties_;

  // D-Bus callbacks may last longer the property set exporting those methods.
  base::WeakPtrFactory<ExportedPropertySet> weak_ptr_factory_;

  using SignalPropertiesChanged =
      DBusSignal<std::string, VariantDictionary, std::vector<std::string>>;

  std::weak_ptr<SignalPropertiesChanged> signal_properties_changed_;

  friend class DBusObject;
  friend class ExportedPropertySetTest;
  DISALLOW_COPY_AND_ASSIGN(ExportedPropertySet);
};

template<typename T>
class ExportedProperty : public ExportedPropertyBase {
 public:
  ExportedProperty() = default;
  ~ExportedProperty() override = default;

  // Retrieves the current value.
  const T& value() const { return value_; }

  // Set the value exposed to remote applications.  This triggers notifications
  // of changes over the Properties interface.
  void SetValue(const T& new_value) {
    if (value_ != new_value) {
      value_ = new_value;
      this->NotifyPropertyChanged();
    }
  }

  // Set the validator for value checking when setting the property by remote
  // application.
  void SetValidator(
      const base::Callback<bool(brillo::ErrorPtr*, const T&)>& validator) {
    validator_ = validator;
  }

  // Implementation provided by specialization.
  brillo::Any GetValue() const override { return value_; }

  bool SetValue(brillo::ErrorPtr* error,
                const brillo::Any& value) override {
    if (GetAccessMode() == ExportedPropertyBase::Access::kReadOnly) {
      brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                           DBUS_ERROR_PROPERTY_READ_ONLY,
                           "Property is read-only.");
      return false;
    }
    if (!value.IsTypeCompatible<T>()) {
      brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                           DBUS_ERROR_INVALID_ARGS,
                           "Argument type mismatched.");
      return false;
    }
    if (value_ == value.Get<T>()) {
      // No change to the property value, nothing to be done.
      return true;
    }
    if (!validator_.is_null() && !validator_.Run(error, value.Get<T>())) {
      return false;
    }
    value_ = value.Get<T>();
    return true;
  }

 private:
  T value_{};
  base::Callback<bool(brillo::ErrorPtr*, const T&)> validator_;

  DISALLOW_COPY_AND_ASSIGN(ExportedProperty);
};

}  // namespace dbus_utils

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_EXPORTED_PROPERTY_SET_H_
