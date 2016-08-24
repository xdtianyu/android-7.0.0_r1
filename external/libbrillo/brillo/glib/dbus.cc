// Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/glib/dbus.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <base/logging.h>
#include <base/strings/stringprintf.h>

namespace brillo {
namespace dbus {

bool CallPtrArray(const Proxy& proxy,
                  const char* method,
                  glib::ScopedPtrArray<const char*>* result) {
  glib::ScopedError error;

  ::GType g_type_array = ::dbus_g_type_get_collection("GPtrArray",
                                                       DBUS_TYPE_G_OBJECT_PATH);


  if (!::dbus_g_proxy_call(proxy.gproxy(), method, &Resetter(&error).lvalue(),
                           G_TYPE_INVALID, g_type_array,
                           &Resetter(result).lvalue(), G_TYPE_INVALID)) {
    LOG(WARNING) << "CallPtrArray failed: "
        << (error->message ? error->message : "Unknown Error.");
    return false;
  }

  return true;
}

BusConnection GetSystemBusConnection() {
  glib::ScopedError error;
  ::DBusGConnection* result = ::dbus_g_bus_get(DBUS_BUS_SYSTEM,
                                               &Resetter(&error).lvalue());
  if (!result) {
    LOG(ERROR) << "dbus_g_bus_get(DBUS_BUS_SYSTEM) failed: "
               << ((error.get() && error->message) ?
                   error->message : "Unknown Error");
    return BusConnection(nullptr);
  }
  // Set to not exit when system bus is disconnected.
  // This fixes the problem where when the dbus daemon is stopped, exit is
  // called which kills Chrome.
  ::dbus_connection_set_exit_on_disconnect(
      ::dbus_g_connection_get_connection(result), FALSE);
  return BusConnection(result);
}

BusConnection GetPrivateBusConnection(const char* address) {
  // Since dbus-glib does not have an API like dbus_g_connection_open_private(),
  // we have to implement our own.

  // We have to call _dbus_g_value_types_init() to register standard marshalers
  // just like as dbus_g_bus_get() and dbus_g_connection_open() do, but the
  // function is not exported. So we call GetPrivateBusConnection() which calls
  // dbus_g_bus_get() here instead. Note that if we don't call
  // _dbus_g_value_types_init(), we might get "WARNING **: No demarshaller
  // registered for type xxxxx" error and might not be able to handle incoming
  // signals nor method calls.
  {
    BusConnection system_bus_connection = GetSystemBusConnection();
    if (!system_bus_connection.HasConnection()) {
      return system_bus_connection;  // returns NULL connection.
    }
  }

  ::DBusError error;
  ::dbus_error_init(&error);

  ::DBusGConnection* result = nullptr;
  ::DBusConnection* raw_connection
        = ::dbus_connection_open_private(address, &error);
  if (!raw_connection) {
    LOG(WARNING) << "dbus_connection_open_private failed: " << address;
    return BusConnection(nullptr);
  }

  if (!::dbus_bus_register(raw_connection, &error)) {
    LOG(ERROR) << "dbus_bus_register failed: "
               << (error.message ? error.message : "Unknown Error.");
    ::dbus_error_free(&error);
    // TODO(yusukes): We don't call dbus_connection_close() nor g_object_unref()
    // here for now since these calls might interfere with IBusBus connections
    // in libcros and Chrome. See the comment in ~InputMethodStatusConnection()
    // function in platform/cros/chromeos_input_method.cc for details.
    return BusConnection(nullptr);
  }

  ::dbus_connection_setup_with_g_main(
      raw_connection, nullptr /* default context */);

  // A reference count of |raw_connection| is transferred to |result|. You don't
  // have to (and should not) unref the |raw_connection|.
  result = ::dbus_connection_get_g_connection(raw_connection);
  CHECK(result);

  ::dbus_connection_set_exit_on_disconnect(
      ::dbus_g_connection_get_connection(result), FALSE);

  return BusConnection(result);
}

bool RetrieveProperties(const Proxy& proxy,
                        const char* interface,
                        glib::ScopedHashTable* result) {
  glib::ScopedError error;

  if (!::dbus_g_proxy_call(proxy.gproxy(), "GetAll", &Resetter(&error).lvalue(),
                           G_TYPE_STRING, interface, G_TYPE_INVALID,
                           ::dbus_g_type_get_map("GHashTable", G_TYPE_STRING,
                                                 G_TYPE_VALUE),
                           &Resetter(result).lvalue(), G_TYPE_INVALID)) {
    LOG(WARNING) << "RetrieveProperties failed: "
        << (error->message ? error->message : "Unknown Error.");
    return false;
  }
  return true;
}

Proxy::Proxy()
    : object_(nullptr) {
}

// Set |connect_to_name_owner| true if you'd like to use
// dbus_g_proxy_new_for_name_owner() rather than dbus_g_proxy_new_for_name().
Proxy::Proxy(const BusConnection& connection,
             const char* name,
             const char* path,
             const char* interface,
             bool connect_to_name_owner)
    : object_(GetGProxy(
        connection, name, path, interface, connect_to_name_owner)) {
}

// Equivalent to Proxy(connection, name, path, interface, false).
Proxy::Proxy(const BusConnection& connection,
             const char* name,
             const char* path,
             const char* interface)
    : object_(GetGProxy(connection, name, path, interface, false)) {
}

// Creates a peer proxy using dbus_g_proxy_new_for_peer.
Proxy::Proxy(const BusConnection& connection,
             const char* path,
             const char* interface)
    : object_(GetGPeerProxy(connection, path, interface)) {
}

Proxy::Proxy(const Proxy& x)
    : object_(x.object_) {
  if (object_)
    ::g_object_ref(object_);
}

Proxy::~Proxy() {
  if (object_)
    ::g_object_unref(object_);
}

/* static */
Proxy::value_type Proxy::GetGProxy(const BusConnection& connection,
                                   const char* name,
                                   const char* path,
                                   const char* interface,
                                   bool connect_to_name_owner) {
  value_type result = nullptr;
  if (connect_to_name_owner) {
    glib::ScopedError error;
    result = ::dbus_g_proxy_new_for_name_owner(connection.object_,
                                               name,
                                               path,
                                               interface,
                                               &Resetter(&error).lvalue());
    if (!result) {
      DLOG(ERROR) << "Failed to construct proxy: "
                  << (error->message ? error->message : "Unknown Error")
                  << ": " << path;
    }
  } else {
    result = ::dbus_g_proxy_new_for_name(connection.object_,
                                         name,
                                         path,
                                         interface);
    if (!result) {
      LOG(ERROR) << "Failed to construct proxy: " << path;
    }
  }
  return result;
}

/* static */
Proxy::value_type Proxy::GetGPeerProxy(const BusConnection& connection,
                                       const char* path,
                                       const char* interface) {
  value_type result = ::dbus_g_proxy_new_for_peer(connection.object_,
                                                  path,
                                                  interface);
  if (!result)
    LOG(ERROR) << "Failed to construct peer proxy: " << path;

  return result;
}

bool RegisterExclusiveService(const BusConnection& connection,
                              const char* interface_name,
                              const char* service_name,
                              const char* service_path,
                              GObject* object) {
  CHECK(object);
  CHECK(interface_name);
  CHECK(service_name);
  // Create a proxy to DBus itself so that we can request to become a
  // service name owner and then register an object at the related service path.
  Proxy proxy = brillo::dbus::Proxy(connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);
  // Exclusivity is determined by replacing any existing
  // service, not queuing, and ensuring we are the primary
  // owner after the name is ours.
  glib::ScopedError err;
  guint result = 0;
  // TODO(wad) determine if we are moving away from using generated functions
  if (!org_freedesktop_DBus_request_name(proxy.gproxy(),
                                         service_name,
                                         0,
                                         &result,
                                         &Resetter(&err).lvalue())) {
    LOG(ERROR) << "Unable to request service name: "
               << (err->message ? err->message : "Unknown Error.");
    return false;
  }

  // Handle the error codes, releasing the name if exclusivity conditions
  // are not met.
  bool needs_release = false;
  if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    LOG(ERROR) << "Failed to become the primary owner. Releasing . . .";
    needs_release = true;
  }
  if (result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
    LOG(ERROR) << "Service name exists: " << service_name;
    return false;
  } else if (result == DBUS_REQUEST_NAME_REPLY_IN_QUEUE) {
    LOG(ERROR) << "Service name request enqueued despite our flags. Releasing";
    needs_release = true;
  }
  LOG_IF(WARNING, result == DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER)
    << "Service name already owned by this process";
  if (needs_release) {
    if (!org_freedesktop_DBus_release_name(
           proxy.gproxy(),
           service_name,
           &result,
           &Resetter(&err).lvalue())) {
      LOG(ERROR) << "Unabled to release service name: "
                 << (err->message ? err->message : "Unknown Error.");
    }
    DLOG(INFO) << "ReleaseName returned code " << result;
    return false;
  }

  // Determine a path from the service name and register the object.
  dbus_g_connection_register_g_object(connection.g_connection(),
                                      service_path,
                                      object);
  return true;
}

void CallMethodWithNoArguments(const char* service_name,
                               const char* path,
                               const char* interface_name,
                               const char* method_name) {
  Proxy proxy(dbus::GetSystemBusConnection(),
              service_name,
              path,
              interface_name);
  ::dbus_g_proxy_call_no_reply(proxy.gproxy(), method_name, G_TYPE_INVALID);
}

void SignalWatcher::StartMonitoring(const std::string& interface,
                                    const std::string& signal) {
  DCHECK(interface_.empty()) << "StartMonitoring() must be called only once";
  interface_ = interface;
  signal_ = signal;

  // Snoop on D-Bus messages so we can get notified about signals.
  DBusConnection* dbus_conn = dbus_g_connection_get_connection(
      GetSystemBusConnection().g_connection());
  DCHECK(dbus_conn);

  DBusError error;
  dbus_error_init(&error);
  dbus_bus_add_match(dbus_conn, GetDBusMatchString().c_str(), &error);
  if (dbus_error_is_set(&error)) {
    LOG(DFATAL) << "Got error while adding D-Bus match rule: " << error.name
                << " (" << error.message << ")";
  }

  if (!dbus_connection_add_filter(dbus_conn,
                                  &SignalWatcher::FilterDBusMessage,
                                  this,        // user_data
                                  nullptr)) {  // free_data_function
    LOG(DFATAL) << "Unable to add D-Bus filter";
  }
}

SignalWatcher::~SignalWatcher() {
  if (interface_.empty())
    return;

  DBusConnection* dbus_conn = dbus_g_connection_get_connection(
      dbus::GetSystemBusConnection().g_connection());
  DCHECK(dbus_conn);

  dbus_connection_remove_filter(dbus_conn,
                                &SignalWatcher::FilterDBusMessage,
                                this);

  DBusError error;
  dbus_error_init(&error);
  dbus_bus_remove_match(dbus_conn, GetDBusMatchString().c_str(), &error);
  if (dbus_error_is_set(&error)) {
    LOG(DFATAL) << "Got error while removing D-Bus match rule: " << error.name
                << " (" << error.message << ")";
  }
}

std::string SignalWatcher::GetDBusMatchString() const {
  return base::StringPrintf("type='signal', interface='%s', member='%s'",
                            interface_.c_str(), signal_.c_str());
}

/* static */
DBusHandlerResult SignalWatcher::FilterDBusMessage(DBusConnection* dbus_conn,
                                                   DBusMessage* message,
                                                   void* data) {
  SignalWatcher* self = static_cast<SignalWatcher*>(data);
  if (dbus_message_is_signal(
          message, self->interface_.c_str(), self->signal_.c_str())) {
    self->OnSignal(message);
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

}  // namespace dbus
}  // namespace brillo
