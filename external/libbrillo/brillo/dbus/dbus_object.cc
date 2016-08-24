// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_object.h>

#include <vector>

#include <base/bind.h>
#include <base/logging.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>
#include <brillo/dbus/exported_property_set.h>
#include <dbus/property.h>

namespace brillo {
namespace dbus_utils {

//////////////////////////////////////////////////////////////////////////////

DBusInterface::DBusInterface(DBusObject* dbus_object,
                             const std::string& interface_name)
    : dbus_object_(dbus_object), interface_name_(interface_name) {
}

void DBusInterface::AddProperty(const std::string& property_name,
                                ExportedPropertyBase* prop_base) {
  dbus_object_->property_set_.RegisterProperty(
      interface_name_, property_name, prop_base);
}

void DBusInterface::ExportAsync(
    ExportedObjectManager* object_manager,
    dbus::Bus* /* bus */,
    dbus::ExportedObject* exported_object,
    const dbus::ObjectPath& object_path,
    const AsyncEventSequencer::CompletionAction& completion_callback) {
  VLOG(1) << "Registering D-Bus interface '" << interface_name_ << "' for '"
          << object_path.value() << "'";
  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  for (const auto& pair : handlers_) {
    std::string method_name = pair.first;
    VLOG(1) << "Exporting method: " << interface_name_ << "." << method_name;
    std::string export_error = "Failed exporting " + method_name + " method";
    auto export_handler = sequencer->GetExportHandler(
        interface_name_, method_name, export_error, true);
    auto method_handler =
        base::Bind(&DBusInterface::HandleMethodCall, base::Unretained(this));
    exported_object->ExportMethod(
        interface_name_, method_name, method_handler, export_handler);
  }

  std::vector<AsyncEventSequencer::CompletionAction> actions;
  if (object_manager) {
    auto property_writer_callback =
        dbus_object_->property_set_.GetPropertyWriter(interface_name_);
    actions.push_back(
        base::Bind(&DBusInterface::ClaimInterface,
                   weak_factory_.GetWeakPtr(),
                   object_manager->AsWeakPtr(),
                   object_path,
                   property_writer_callback));
  }
  actions.push_back(completion_callback);
  sequencer->OnAllTasksCompletedCall(actions);
}

void DBusInterface::ExportAndBlock(
    ExportedObjectManager* object_manager,
    dbus::Bus* /* bus */,
    dbus::ExportedObject* exported_object,
    const dbus::ObjectPath& object_path) {
  VLOG(1) << "Registering D-Bus interface '" << interface_name_ << "' for '"
          << object_path.value() << "'";
  for (const auto& pair : handlers_) {
    std::string method_name = pair.first;
    VLOG(1) << "Exporting method: " << interface_name_ << "." << method_name;
    auto method_handler =
        base::Bind(&DBusInterface::HandleMethodCall, base::Unretained(this));
    if (!exported_object->ExportMethodAndBlock(
            interface_name_, method_name, method_handler)) {
        LOG(FATAL) << "Failed exporting " << method_name << " method";
    }
  }

  if (object_manager) {
    auto property_writer_callback =
        dbus_object_->property_set_.GetPropertyWriter(interface_name_);
    ClaimInterface(object_manager->AsWeakPtr(),
                   object_path,
                   property_writer_callback,
                   true);
  }
}

void DBusInterface::ClaimInterface(
      base::WeakPtr<ExportedObjectManager> object_manager,
      const dbus::ObjectPath& object_path,
      const ExportedPropertySet::PropertyWriter& writer,
      bool all_succeeded) {
  if (!all_succeeded || !object_manager) {
    LOG(ERROR) << "Skipping claiming interface: " << interface_name_;
    return;
  }
  object_manager->ClaimInterface(object_path, interface_name_, writer);
  release_interface_cb_.Reset(
      base::Bind(&ExportedObjectManager::ReleaseInterface,
                 object_manager, object_path, interface_name_));
}

void DBusInterface::HandleMethodCall(dbus::MethodCall* method_call,
                                     ResponseSender sender) {
  std::string method_name = method_call->GetMember();
  // Make a local copy of |interface_name_| because calling HandleMethod()
  // can potentially kill this interface object...
  std::string interface_name = interface_name_;
  VLOG(1) << "Received method call request: " << interface_name << "."
          << method_name << "(" << method_call->GetSignature() << ")";
  auto pair = handlers_.find(method_name);
  if (pair == handlers_.end()) {
    auto response =
        dbus::ErrorResponse::FromMethodCall(method_call,
                                            DBUS_ERROR_UNKNOWN_METHOD,
                                            "Unknown method: " + method_name);
    sender.Run(std::move(response));
    return;
  }
  VLOG(1) << "Dispatching DBus method call: " << method_name;
  pair->second->HandleMethod(method_call, sender);
}

void DBusInterface::AddHandlerImpl(
    const std::string& method_name,
    std::unique_ptr<DBusInterfaceMethodHandlerInterface> handler) {
  VLOG(1) << "Declaring method handler: " << interface_name_ << "."
          << method_name;
  auto res = handlers_.insert(std::make_pair(method_name, std::move(handler)));
  CHECK(res.second) << "Method '" << method_name << "' already exists";
}

void DBusInterface::AddSignalImpl(
    const std::string& signal_name,
    const std::shared_ptr<DBusSignalBase>& signal) {
  VLOG(1) << "Declaring a signal sink: " << interface_name_ << "."
          << signal_name;
  CHECK(signals_.insert(std::make_pair(signal_name, signal)).second)
      << "The signal '" << signal_name << "' is already registered";
}

///////////////////////////////////////////////////////////////////////////////

DBusObject::DBusObject(ExportedObjectManager* object_manager,
                       const scoped_refptr<dbus::Bus>& bus,
                       const dbus::ObjectPath& object_path)
    : property_set_(bus.get()), bus_(bus), object_path_(object_path) {
  if (object_manager)
    object_manager_ = object_manager->AsWeakPtr();
}

DBusObject::~DBusObject() {
  if (exported_object_)
    exported_object_->Unregister();
}

DBusInterface* DBusObject::AddOrGetInterface(
    const std::string& interface_name) {
  auto iter = interfaces_.find(interface_name);
  if (iter == interfaces_.end()) {
    VLOG(1) << "Adding an interface '" << interface_name << "' to object '"
            << object_path_.value() << "'.";
    // Interface doesn't exist yet. Create one...
    std::unique_ptr<DBusInterface> new_itf(
        new DBusInterface(this, interface_name));
    iter = interfaces_.insert(std::make_pair(interface_name,
                                             std::move(new_itf))).first;
  }
  return iter->second.get();
}

DBusInterface* DBusObject::FindInterface(
    const std::string& interface_name) const {
  auto itf_iter = interfaces_.find(interface_name);
  return (itf_iter == interfaces_.end()) ? nullptr : itf_iter->second.get();
}

void DBusObject::RegisterAsync(
    const AsyncEventSequencer::CompletionAction& completion_callback) {
  VLOG(1) << "Registering D-Bus object '" << object_path_.value() << "'.";
  CHECK(exported_object_ == nullptr) << "Object already registered.";
  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  exported_object_ = bus_->GetExportedObject(object_path_);

  // Add the org.freedesktop.DBus.Properties interface to the object.
  DBusInterface* prop_interface = AddOrGetInterface(dbus::kPropertiesInterface);
  prop_interface->AddSimpleMethodHandler(
      dbus::kPropertiesGetAll,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleGetAll);
  prop_interface->AddSimpleMethodHandlerWithError(
      dbus::kPropertiesGet,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleGet);
  prop_interface->AddSimpleMethodHandlerWithError(
      dbus::kPropertiesSet,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleSet);
  property_set_.OnPropertiesInterfaceExported(prop_interface);

  // Export interface methods
  for (const auto& pair : interfaces_) {
    pair.second->ExportAsync(
        object_manager_.get(),
        bus_.get(),
        exported_object_,
        object_path_,
        sequencer->GetHandler("Failed to export interface " + pair.first,
                              false));
  }

  sequencer->OnAllTasksCompletedCall({completion_callback});
}

void DBusObject::RegisterAndBlock() {
  VLOG(1) << "Registering D-Bus object '" << object_path_.value() << "'.";
  CHECK(exported_object_ == nullptr) << "Object already registered.";
  exported_object_ = bus_->GetExportedObject(object_path_);

  // Add the org.freedesktop.DBus.Properties interface to the object.
  DBusInterface* prop_interface = AddOrGetInterface(dbus::kPropertiesInterface);
  prop_interface->AddSimpleMethodHandler(
      dbus::kPropertiesGetAll,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleGetAll);
  prop_interface->AddSimpleMethodHandlerWithError(
      dbus::kPropertiesGet,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleGet);
  prop_interface->AddSimpleMethodHandlerWithError(
      dbus::kPropertiesSet,
      base::Unretained(&property_set_),
      &ExportedPropertySet::HandleSet);
  property_set_.OnPropertiesInterfaceExported(prop_interface);

  // Export interface methods
  for (const auto& pair : interfaces_) {
    pair.second->ExportAndBlock(
        object_manager_.get(),
        bus_.get(),
        exported_object_,
        object_path_);
  }
}

void DBusObject::UnregisterAsync() {
  VLOG(1) << "Unregistering D-Bus object '" << object_path_.value() << "'.";
  CHECK(exported_object_ != nullptr) << "Object not registered.";

  // This will unregister the object path from the bus.
  exported_object_->Unregister();
  // This will remove |exported_object_| from bus's object table. This function
  // will also post a task to unregister |exported_object_| (same as the call
  // above), which will be a no-op since it is already done by then.
  // By doing both in here, the object path is guarantee to be reusable upon
  // return from this function.
  bus_->UnregisterExportedObject(object_path_);
  exported_object_ = nullptr;
}

bool DBusObject::SendSignal(dbus::Signal* signal) {
  if (exported_object_) {
    exported_object_->SendSignal(signal);
    return true;
  }
  LOG(ERROR) << "Trying to send a signal from an object that is not exported";
  return false;
}

}  // namespace dbus_utils
}  // namespace brillo
