// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DBusObject is a special helper class that simplifies the implementation of
// D-Bus objects in C++. It provides an easy way to define interfaces with
// methods and properties and offloads a lot of work to register the object and
// all of its interfaces, to marshal method calls (by converting D-Bus method
// parameters to native C++ types and invoking native method handlers), etc.

// The basic usage pattern of this class is as follows:
/*
class MyDbusObject {
 public:
  MyDbusObject(ExportedObjectManager* object_manager,
               const scoped_refptr<dbus::Bus>& bus)
      : dbus_object_(object_manager, bus,
                     dbus::ObjectPath("/org/chromium/my_obj")) {}

  void Init(const AsyncEventSequencer::CompletionAction& callback) {
    DBusInterface* my_interface =
        dbus_object_.AddOrGetInterface("org.chromium.MyInterface");
    my_interface->AddSimpleMethodHandler("Method1", this,
                                         &MyDbusObject::Method1);
    my_interface->AddSimpleMethodHandlerWithError("Method2", this,
                                                  &MyDbusObject::Method2);
    my_interface->AddMethodHandler("Method3", this, &MyDbusObject::Method3);
    my_interface->AddProperty("Property1", &prop1_);
    my_interface->AddProperty("Property2", &prop2_);
    prop1_.SetValue("prop1_value");
    prop2_.SetValue(50);
    // Register the object by exporting its methods and properties and
    // exposing them to D-Bus clients.
    dbus_object_.RegisterAsync(callback);
  }

 private:
  DBusObject dbus_object_;

  // Make sure the properties outlive the DBusObject they are registered with.
  brillo::dbus_utils::ExportedProperty<std::string> prop1_;
  brillo::dbus_utils::ExportedProperty<int> prop2_;
  int Method1() { return 5; }
  bool Method2(brillo::ErrorPtr* error, const std::string& message);
  void Method3(std::unique_ptr<DBusMethodResponse<int_32>> response,
               const std::string& message) {
    if (message.empty()) {
       response->ReplyWithError(brillo::errors::dbus::kDomain,
                                DBUS_ERROR_INVALID_ARGS,
                                "Message string cannot be empty");
       return;
    }
    int32_t message_len = message.length();
    response->Return(message_len);
  }

  DISALLOW_COPY_AND_ASSIGN(MyDbusObject);
};
*/

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_H_

#include <map>
#include <string>

#include <base/bind.h>
#include <base/callback_helpers.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <brillo/brillo_export.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/dbus_object_internal_impl.h>
#include <brillo/dbus/dbus_signal.h>
#include <brillo/dbus/exported_property_set.h>
#include <brillo/errors/error.h>
#include <dbus/bus.h>
#include <dbus/exported_object.h>
#include <dbus/message.h>
#include <dbus/object_path.h>

namespace brillo {
namespace dbus_utils {

class ExportedObjectManager;
class ExportedPropertyBase;
class DBusObject;

// This is an implementation proxy class for a D-Bus interface of an object.
// The important functionality for the users is the ability to add D-Bus method
// handlers and define D-Bus object properties. This is achieved by using one
// of the overload of AddSimpleMethodHandler()/AddMethodHandler() and
// AddProperty() respectively.
// There are three overloads for DBusInterface::AddSimpleMethodHandler() and
// AddMethodHandler() each:
//  1. That takes a handler as base::Callback
//  2. That takes a static function
//  3. That takes a class instance pointer and a class member function
// The signature of the handler for AddSimpleMethodHandler must be one of:
//    R(Args... args)                     [IN only]
//    void(Args... args)                  [IN/OUT]
// The signature of the handler for AddSimpleMethodHandlerWithError must be:
//    bool(ErrorPtr* error, Args... args) [IN/OUT]
// The signature of the handler for AddSimpleMethodHandlerWithErrorAndMessage:
//    bool(ErrorPtr* error, dbus::Message* msg, Args... args) [IN/OUT]
// The signature of the handler for AddMethodHandler must be:
//    void(std::unique_ptr<DBusMethodResponse<T...>> response,
//         Args... args) [IN]
// The signature of the handler for AddMethodHandlerWithMessage must be:
//    void(std::unique_ptr<DBusMethodResponse<T...>> response,
//         dbus::Message* msg, Args... args) [IN]
// There is also an AddRawMethodHandler() call that lets provide a custom
// handler that can parse its own input parameter and construct a custom
// response.
// The signature of the handler for AddRawMethodHandler must be:
//    void(dbus::MethodCall* method_call, ResponseSender sender)
class BRILLO_EXPORT DBusInterface final {
 public:
  DBusInterface(DBusObject* dbus_object, const std::string& interface_name);

  // Register sync DBus method handler for |method_name| as base::Callback.
  template<typename R, typename... Args>
  inline void AddSimpleMethodHandler(
      const std::string& method_name,
      const base::Callback<R(Args...)>& handler) {
    Handler<SimpleDBusInterfaceMethodHandler<R, Args...>>::Add(
        this, method_name, handler);
  }

  // Register sync D-Bus method handler for |method_name| as a static
  // function.
  template<typename R, typename... Args>
  inline void AddSimpleMethodHandler(const std::string& method_name,
                                     R(*handler)(Args...)) {
    Handler<SimpleDBusInterfaceMethodHandler<R, Args...>>::Add(
        this, method_name, base::Bind(handler));
  }

  // Register sync D-Bus method handler for |method_name| as a class member
  // function.
  template<typename Instance, typename Class, typename R, typename... Args>
  inline void AddSimpleMethodHandler(const std::string& method_name,
                                     Instance instance,
                                     R(Class::*handler)(Args...)) {
    Handler<SimpleDBusInterfaceMethodHandler<R, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Same as above but for const-method of a class.
  template<typename Instance, typename Class, typename R, typename... Args>
  inline void AddSimpleMethodHandler(const std::string& method_name,
                                     Instance instance,
                                     R(Class::*handler)(Args...) const) {
    Handler<SimpleDBusInterfaceMethodHandler<R, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register sync DBus method handler for |method_name| as base::Callback.
  template<typename... Args>
  inline void AddSimpleMethodHandlerWithError(
      const std::string& method_name,
      const base::Callback<bool(ErrorPtr*, Args...)>& handler) {
    Handler<SimpleDBusInterfaceMethodHandlerWithError<Args...>>::Add(
        this, method_name, handler);
  }

  // Register sync D-Bus method handler for |method_name| as a static
  // function.
  template<typename... Args>
  inline void AddSimpleMethodHandlerWithError(
      const std::string& method_name,
      bool(*handler)(ErrorPtr*, Args...)) {
    Handler<SimpleDBusInterfaceMethodHandlerWithError<Args...>>::Add(
        this, method_name, base::Bind(handler));
  }

  // Register sync D-Bus method handler for |method_name| as a class member
  // function.
  template<typename Instance, typename Class, typename... Args>
  inline void AddSimpleMethodHandlerWithError(
      const std::string& method_name,
      Instance instance,
      bool(Class::*handler)(ErrorPtr*, Args...)) {
    Handler<SimpleDBusInterfaceMethodHandlerWithError<Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Same as above but for const-method of a class.
  template<typename Instance, typename Class, typename... Args>
  inline void AddSimpleMethodHandlerWithError(
      const std::string& method_name,
      Instance instance,
      bool(Class::*handler)(ErrorPtr*, Args...) const) {
    Handler<SimpleDBusInterfaceMethodHandlerWithError<Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register sync DBus method handler for |method_name| as base::Callback.
  // Passing the method sender as a first parameter to the callback.
  template<typename... Args>
  inline void AddSimpleMethodHandlerWithErrorAndMessage(
      const std::string& method_name,
      const base::Callback<bool(ErrorPtr*, dbus::Message*, Args...)>&
          handler) {
    Handler<SimpleDBusInterfaceMethodHandlerWithErrorAndMessage<Args...>>::Add(
        this, method_name, handler);
  }

  // Register sync D-Bus method handler for |method_name| as a static
  // function. Passing the method D-Bus message as the second parameter to the
  // callback.
  template<typename... Args>
  inline void AddSimpleMethodHandlerWithErrorAndMessage(
      const std::string& method_name,
      bool(*handler)(ErrorPtr*, dbus::Message*, Args...)) {
    Handler<SimpleDBusInterfaceMethodHandlerWithErrorAndMessage<Args...>>::Add(
        this, method_name, base::Bind(handler));
  }

  // Register sync D-Bus method handler for |method_name| as a class member
  // function. Passing the method D-Bus message as the second parameter to the
  // callback.
  template<typename Instance, typename Class, typename... Args>
  inline void AddSimpleMethodHandlerWithErrorAndMessage(
      const std::string& method_name,
      Instance instance,
      bool(Class::*handler)(ErrorPtr*, dbus::Message*, Args...)) {
    Handler<SimpleDBusInterfaceMethodHandlerWithErrorAndMessage<Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Same as above but for const-method of a class.
  template<typename Instance, typename Class, typename... Args>
  inline void AddSimpleMethodHandlerWithErrorAndMessage(
      const std::string& method_name,
      Instance instance,
      bool(Class::*handler)(ErrorPtr*, dbus::Message*, Args...) const) {
    Handler<SimpleDBusInterfaceMethodHandlerWithErrorAndMessage<Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register an async DBus method handler for |method_name| as base::Callback.
  template<typename Response, typename... Args>
  inline void AddMethodHandler(
      const std::string& method_name,
      const base::Callback<void(std::unique_ptr<Response>, Args...)>& handler) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandler<Response, Args...>>::Add(
        this, method_name, handler);
  }

  // Register an async D-Bus method handler for |method_name| as a static
  // function.
  template<typename Response, typename... Args>
  inline void AddMethodHandler(
      const std::string& method_name,
      void (*handler)(std::unique_ptr<Response>, Args...)) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandler<Response, Args...>>::Add(
        this, method_name, base::Bind(handler));
  }

  // Register an async D-Bus method handler for |method_name| as a class member
  // function.
  template<typename Response,
           typename Instance,
           typename Class,
           typename... Args>
  inline void AddMethodHandler(
      const std::string& method_name,
      Instance instance,
      void(Class::*handler)(std::unique_ptr<Response>, Args...)) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandler<Response, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Same as above but for const-method of a class.
  template<typename Response,
           typename Instance,
           typename Class,
           typename... Args>
  inline void AddMethodHandler(
      const std::string& method_name,
      Instance instance,
      void(Class::*handler)(std::unique_ptr<Response>, Args...) const) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandler<Response, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register an async DBus method handler for |method_name| as base::Callback.
  template<typename Response, typename... Args>
  inline void AddMethodHandlerWithMessage(
      const std::string& method_name,
      const base::Callback<void(std::unique_ptr<Response>, dbus::Message*,
                                Args...)>& handler) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandlerWithMessage<Response, Args...>>::Add(
        this, method_name, handler);
  }

  // Register an async D-Bus method handler for |method_name| as a static
  // function.
  template<typename Response, typename... Args>
  inline void AddMethodHandlerWithMessage(
      const std::string& method_name,
      void (*handler)(std::unique_ptr<Response>, dbus::Message*, Args...)) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandlerWithMessage<Response, Args...>>::Add(
        this, method_name, base::Bind(handler));
  }

  // Register an async D-Bus method handler for |method_name| as a class member
  // function.
  template<typename Response,
           typename Instance,
           typename Class,
           typename... Args>
  inline void AddMethodHandlerWithMessage(
      const std::string& method_name,
      Instance instance,
      void(Class::*handler)(std::unique_ptr<Response>,
                            dbus::Message*, Args...)) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandlerWithMessage<Response, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Same as above but for const-method of a class.
  template<typename Response,
           typename Instance,
           typename Class,
           typename... Args>
  inline void AddMethodHandlerWithMessage(
      const std::string& method_name,
      Instance instance,
      void(Class::*handler)(std::unique_ptr<Response>, dbus::Message*,
                            Args...) const) {
    static_assert(std::is_base_of<DBusMethodResponseBase, Response>::value,
                  "Response must be DBusMethodResponse<T...>");
    Handler<DBusInterfaceMethodHandlerWithMessage<Response, Args...>>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register a raw D-Bus method handler for |method_name| as base::Callback.
  inline void AddRawMethodHandler(
      const std::string& method_name,
      const base::Callback<void(dbus::MethodCall*, ResponseSender)>& handler) {
    Handler<RawDBusInterfaceMethodHandler>::Add(this, method_name, handler);
  }

  // Register a raw D-Bus method handler for |method_name| as a class member
  // function.
  template<typename Instance, typename Class>
  inline void AddRawMethodHandler(
      const std::string& method_name,
      Instance instance,
      void(Class::*handler)(dbus::MethodCall*, ResponseSender)) {
    Handler<RawDBusInterfaceMethodHandler>::Add(
        this, method_name, base::Bind(handler, instance));
  }

  // Register a D-Bus property.
  void AddProperty(const std::string& property_name,
                   ExportedPropertyBase* prop_base);

  // Registers a D-Bus signal that has a specified number and types (|Args|) of
  // arguments. Returns a weak pointer to the DBusSignal object which can be
  // used to send the signal on this interface when needed:
  /*
    DBusInterface* itf = dbus_object->AddOrGetInterface("Interface");
    auto signal = itf->RegisterSignal<int, bool>("MySignal");
    ...
    // Send the Interface.MySig(12, true) signal.
    if (signal.lock()->Send(12, true)) { ... }
  */
  // Or if the signal signature is long or complex, you can alias the
  // DBusSignal<Args...> signal type and use RegisterSignalOfType method
  // instead:
  /*
    DBusInterface* itf = dbus_object->AddOrGetInterface("Interface");
    using MySignal = DBusSignal<int, bool>;
    auto signal = itf->RegisterSignalOfType<MySignal>("MySignal");
    ...
    // Send the Interface.MySig(12, true) signal.
    if (signal.lock()->Send(12, true)) { ... }
  */
  // If the signal with the given name was already registered, the existing
  // copy of the signal proxy object is returned as long as the method signature
  // of the original signal matches the current call. If it doesn't, the method
  // aborts.

  // RegisterSignalOfType can be used to create a signal if the type of the
  // complete DBusSignal<Args...> class which is pre-defined/aliased earlier.
  template<typename DBusSignalType>
  inline std::weak_ptr<DBusSignalType> RegisterSignalOfType(
      const std::string& signal_name) {
    auto signal = std::make_shared<DBusSignalType>(
        dbus_object_, interface_name_, signal_name);
    AddSignalImpl(signal_name, signal);
    return signal;
  }

  // For simple signal arguments, you can specify their types directly in
  // RegisterSignal<t1, t2, ...>():
  //  auto signal = itf->RegisterSignal<int>("SignalName");
  // This will create a callback signal object that expects one int argument.
  template<typename... Args>
  inline std::weak_ptr<DBusSignal<Args...>> RegisterSignal(
      const std::string& signal_name) {
    return RegisterSignalOfType<DBusSignal<Args...>>(signal_name);
  }

 private:
  // Helper to create an instance of DBusInterfaceMethodHandlerInterface-derived
  // handler and add it to the method handler map of the interface.
  // This makes the actual AddXXXMethodHandler() methods very light-weight and
  // easier to provide different overloads for various method handler kinds.
  // Using struct here to allow partial specialization on HandlerType while
  // letting the compiler to deduce the type of the callback without explicitly
  // specifying it.
  template<typename HandlerType>
  struct Handler {
    template<typename CallbackType>
    inline static void Add(DBusInterface* self,
                           const std::string& method_name,
                           const CallbackType& callback) {
      std::unique_ptr<DBusInterfaceMethodHandlerInterface> sync_method_handler(
          new HandlerType(callback));
      self->AddHandlerImpl(method_name, std::move(sync_method_handler));
    }
  };
  // A generic D-Bus method handler for the interface. It extracts the method
  // name from |method_call|, looks up a registered handler from |handlers_|
  // map and dispatched the call to that handler.
  void HandleMethodCall(dbus::MethodCall* method_call, ResponseSender sender);
  // Helper to add a handler for method |method_name| to the |handlers_| map.
  // Not marked BRILLO_PRIVATE because it needs to be called by the inline
  // template functions AddMethodHandler(...)
  void AddHandlerImpl(
      const std::string& method_name,
      std::unique_ptr<DBusInterfaceMethodHandlerInterface> handler);
  // Helper to add a signal object to the |signals_| map.
  // Not marked BRILLO_PRIVATE because it needs to be called by the inline
  // template function RegisterSignalOfType(...)
  void AddSignalImpl(const std::string& signal_name,
                     const std::shared_ptr<DBusSignalBase>& signal);
  // Exports all the methods and properties of this interface and claims the
  // D-Bus interface.
  // object_manager - ExportedObjectManager instance that notifies D-Bus
  //                  listeners of a new interface being claimed.
  // exported_object - instance of D-Bus object the interface is being added to.
  // object_path - D-Bus object path for the object instance.
  // interface_name - name of interface being registered.
  // completion_callback - a callback to be called when the asynchronous
  //                       registration operation is completed.
  BRILLO_PRIVATE void ExportAsync(
      ExportedObjectManager* object_manager,
      dbus::Bus* bus,
      dbus::ExportedObject* exported_object,
      const dbus::ObjectPath& object_path,
      const AsyncEventSequencer::CompletionAction& completion_callback);
  // Exports all the methods and properties of this interface and claims the
  // D-Bus interface synchronously.
  // object_manager - ExportedObjectManager instance that notifies D-Bus
  //                  listeners of a new interface being claimed.
  // exported_object - instance of D-Bus object the interface is being added to.
  // object_path - D-Bus object path for the object instance.
  // interface_name - name of interface being registered.
  BRILLO_PRIVATE void ExportAndBlock(
      ExportedObjectManager* object_manager,
      dbus::Bus* bus,
      dbus::ExportedObject* exported_object,
      const dbus::ObjectPath& object_path);

  BRILLO_PRIVATE void ClaimInterface(
      base::WeakPtr<ExportedObjectManager> object_manager,
      const dbus::ObjectPath& object_path,
      const ExportedPropertySet::PropertyWriter& writer,
      bool all_succeeded);

  // Method registration map.
  std::map<std::string, std::unique_ptr<DBusInterfaceMethodHandlerInterface>>
      handlers_;
  // Signal registration map.
  std::map<std::string, std::shared_ptr<DBusSignalBase>> signals_;

  friend class DBusObject;
  friend class DBusInterfaceTestHelper;
  DBusObject* dbus_object_;
  std::string interface_name_;
  base::ScopedClosureRunner release_interface_cb_;

  base::WeakPtrFactory<DBusInterface> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DBusInterface);
};

// A D-Bus object implementation class. Manages the interfaces implemented
// by this object.
class BRILLO_EXPORT DBusObject {
 public:
  // object_manager - ExportedObjectManager instance that notifies D-Bus
  //                  listeners of a new interface being claimed and property
  //                  changes on those interfaces.
  // object_path - D-Bus object path for the object instance.
  DBusObject(ExportedObjectManager* object_manager,
             const scoped_refptr<dbus::Bus>& bus,
             const dbus::ObjectPath& object_path);
  virtual ~DBusObject();

  // Returns an proxy handler for the interface |interface_name|. If the
  // interface proxy does not exist yet, it will be automatically created.
  DBusInterface* AddOrGetInterface(const std::string& interface_name);

  // Finds an interface with the given name. Returns nullptr if there is no
  // interface registered by this name.
  DBusInterface* FindInterface(const std::string& interface_name) const;

  // Registers the object instance with D-Bus. This is an asynchronous call
  // that will call |completion_callback| when the object and all of its
  // interfaces are registered.
  virtual void RegisterAsync(
      const AsyncEventSequencer::CompletionAction& completion_callback);

  // Registers the object instance with D-Bus. This is call is synchronous and
  // will block until the object and all of its interfaces are registered.
  virtual void RegisterAndBlock();

  // Unregister the object instance with D-Bus.  This will unregister the
  // |exported_object_| and its path from the bus.  The destruction of
  // |exported_object_| will be deferred in an async task posted by the bus.
  // It is guarantee that upon return from this call a new DBusObject with the
  // same object path can be created/registered.
  virtual void UnregisterAsync();

  // Returns the ExportedObjectManager proxy, if any. If DBusObject has been
  // constructed without an object manager, this method returns an empty
  // smart pointer (containing nullptr).
  const base::WeakPtr<ExportedObjectManager>& GetObjectManager() const {
    return object_manager_;
  }

  // Sends a signal from the exported D-Bus object.
  bool SendSignal(dbus::Signal* signal);

  // Returns the reference to dbus::Bus this object is associated with.
  scoped_refptr<dbus::Bus> GetBus() { return bus_; }

 private:
  // A map of all the interfaces added to this object.
  std::map<std::string, std::unique_ptr<DBusInterface>> interfaces_;
  // Exported property set for properties registered with the interfaces
  // implemented by this D-Bus object.
  ExportedPropertySet property_set_;
  // Delegate object implementing org.freedesktop.DBus.ObjectManager interface.
  base::WeakPtr<ExportedObjectManager> object_manager_;
  // D-Bus bus object.
  scoped_refptr<dbus::Bus> bus_;
  // D-Bus object path for this object.
  dbus::ObjectPath object_path_;
  // D-Bus object instance once this object is successfully exported.
  dbus::ExportedObject* exported_object_ = nullptr;  // weak; owned by |bus_|.

  friend class DBusInterface;
  DISALLOW_COPY_AND_ASSIGN(DBusObject);
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_H_
