// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/exported_property_set.h>

#include <base/bind.h>
#include <dbus/bus.h>
#include <dbus/property.h>  // For kPropertyInterface

#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/dbus_object.h>
#include <brillo/errors/error_codes.h>

using brillo::dbus_utils::AsyncEventSequencer;

namespace brillo {

namespace dbus_utils {

ExportedPropertySet::ExportedPropertySet(dbus::Bus* bus)
    : bus_(bus), weak_ptr_factory_(this) {
}

void ExportedPropertySet::OnPropertiesInterfaceExported(
    DBusInterface* prop_interface) {
  signal_properties_changed_ =
      prop_interface->RegisterSignalOfType<SignalPropertiesChanged>(
          dbus::kPropertiesChanged);
}

ExportedPropertySet::PropertyWriter ExportedPropertySet::GetPropertyWriter(
    const std::string& interface_name) {
  return base::Bind(&ExportedPropertySet::WritePropertiesToDict,
                    weak_ptr_factory_.GetWeakPtr(),
                    interface_name);
}

void ExportedPropertySet::RegisterProperty(
    const std::string& interface_name,
    const std::string& property_name,
    ExportedPropertyBase* exported_property) {
  bus_->AssertOnOriginThread();
  auto& prop_map = properties_[interface_name];
  auto res = prop_map.insert(std::make_pair(property_name, exported_property));
  CHECK(res.second) << "Property '" << property_name << "' already exists";
  // Technically, the property set exists longer than the properties themselves,
  // so we could use Unretained here rather than a weak pointer.
  ExportedPropertyBase::OnUpdateCallback cb =
      base::Bind(&ExportedPropertySet::HandlePropertyUpdated,
                 weak_ptr_factory_.GetWeakPtr(),
                 interface_name,
                 property_name);
  exported_property->SetUpdateCallback(cb);
}

VariantDictionary ExportedPropertySet::HandleGetAll(
    const std::string& interface_name) {
  bus_->AssertOnOriginThread();
  return GetInterfaceProperties(interface_name);
}

VariantDictionary ExportedPropertySet::GetInterfaceProperties(
    const std::string& interface_name) const {
  VariantDictionary properties;
  auto property_map_itr = properties_.find(interface_name);
  if (property_map_itr != properties_.end()) {
    for (const auto& kv : property_map_itr->second)
      properties.insert(std::make_pair(kv.first, kv.second->GetValue()));
  }
  return properties;
}

void ExportedPropertySet::WritePropertiesToDict(
    const std::string& interface_name,
    VariantDictionary* dict) {
  *dict = GetInterfaceProperties(interface_name);
}

bool ExportedPropertySet::HandleGet(brillo::ErrorPtr* error,
                                    const std::string& interface_name,
                                    const std::string& property_name,
                                    brillo::Any* result) {
  bus_->AssertOnOriginThread();
  auto property_map_itr = properties_.find(interface_name);
  if (property_map_itr == properties_.end()) {
    brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                         DBUS_ERROR_UNKNOWN_INTERFACE,
                         "No such interface on object.");
    return false;
  }
  LOG(INFO) << "Looking for " << property_name << " on " << interface_name;
  auto property_itr = property_map_itr->second.find(property_name);
  if (property_itr == property_map_itr->second.end()) {
    brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                         DBUS_ERROR_UNKNOWN_PROPERTY,
                         "No such property on interface.");
    return false;
  }
  *result = property_itr->second->GetValue();
  return true;
}

bool ExportedPropertySet::HandleSet(brillo::ErrorPtr* error,
                                    const std::string& interface_name,
                                    const std::string& property_name,
                                    const brillo::Any& value) {
  bus_->AssertOnOriginThread();
  auto property_map_itr = properties_.find(interface_name);
  if (property_map_itr == properties_.end()) {
    brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                         DBUS_ERROR_UNKNOWN_INTERFACE,
                         "No such interface on object.");
    return false;
  }
  LOG(INFO) << "Looking for " << property_name << " on " << interface_name;
  auto property_itr = property_map_itr->second.find(property_name);
  if (property_itr == property_map_itr->second.end()) {
    brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                         DBUS_ERROR_UNKNOWN_PROPERTY,
                         "No such property on interface.");
    return false;
  }

  return property_itr->second->SetValue(error, value);
}

void ExportedPropertySet::HandlePropertyUpdated(
    const std::string& interface_name,
    const std::string& property_name,
    const ExportedPropertyBase* exported_property) {
  bus_->AssertOnOriginThread();
  // Send signal only if the object has been exported successfully.
  // This could happen when a property value is changed (which triggers
  // the notification) before D-Bus interface is completely exported/claimed.
  auto signal = signal_properties_changed_.lock();
  if (!signal)
    return;
  VariantDictionary changed_properties{
      {property_name, exported_property->GetValue()}};
  // The interface specification tells us to include this list of properties
  // which have changed, but for whom no value is conveyed.  Currently, we
  // don't do anything interesting here.
  std::vector<std::string> invalidated_properties;  // empty.
  signal->Send(interface_name, changed_properties, invalidated_properties);
}

void ExportedPropertyBase::NotifyPropertyChanged() {
  // These is a brief period after the construction of an ExportedProperty
  // when this callback is not initialized because the property has not
  // been registered with the parent ExportedPropertySet.  During this period
  // users should be initializing values via SetValue, and no notifications
  // should be triggered by the ExportedPropertySet.
  if (!on_update_callback_.is_null()) {
    on_update_callback_.Run(this);
  }
}

void ExportedPropertyBase::SetUpdateCallback(const OnUpdateCallback& cb) {
  on_update_callback_ = cb;
}

void ExportedPropertyBase::SetAccessMode(
    ExportedPropertyBase::Access access_mode) {
  access_mode_ = access_mode;
}

ExportedPropertyBase::Access ExportedPropertyBase::GetAccessMode() const {
  return access_mode_;
}

}  // namespace dbus_utils

}  // namespace brillo
