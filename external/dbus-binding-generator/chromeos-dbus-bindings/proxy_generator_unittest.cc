// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/proxy_generator.h"

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "chromeos-dbus-bindings/interface.h"
#include "chromeos-dbus-bindings/test_utils.h"

using std::string;
using std::vector;
using testing::Test;

namespace chromeos_dbus_bindings {

namespace {

const char kDBusTypeArryOfObjects[] = "ao";
const char kDBusTypeArryOfStrings[] = "as";
const char kDBusTypeBool[] = "b";
const char kDBusTypeByte[] = "y";
const char kDBusTypeInt32[] = "i";
const char kDBusTypeInt64[] = "x";
const char kDBusTypeString[] = "s";

const char kExpectedContent[] = R"literal_string(
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <brillo/dbus/dbus_property.h>
#include <brillo/dbus/dbus_signal_handler.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/object_manager.h>
#include <dbus/object_path.h>
#include <dbus/object_proxy.h>

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::TestInterface.
class TestInterfaceProxyInterface {
 public:
  virtual ~TestInterfaceProxyInterface() = default;

  virtual bool Elements(
      const std::string& in_space_walk,
      const std::vector<dbus::ObjectPath>& in_ramblin_man,
      std::string* out_3,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void ElementsAsync(
      const std::string& in_space_walk,
      const std::vector<dbus::ObjectPath>& in_ramblin_man,
      const base::Callback<void(const std::string&)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool ReturnToPatagonia(
      int64_t* out_1,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void ReturnToPatagoniaAsync(
      const base::Callback<void(int64_t)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool NiceWeatherForDucks(
      bool in_1,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void NiceWeatherForDucksAsync(
      bool in_1,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Comment line1
  // line2
  virtual bool ExperimentNumberSix(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // Comment line1
  // line2
  virtual void ExperimentNumberSixAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterTheCurseOfKaZarSignalHandler(
      const base::Callback<void(const std::vector<std::string>&,
                                uint8_t)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::TestInterface.
class TestInterfaceProxy final : public TestInterfaceProxyInterface {
 public:
  TestInterfaceProxy(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name) :
          bus_{bus},
          service_name_{service_name},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~TestInterfaceProxy() override {
  }

  void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "Closer",
        signal_callback,
        on_connected_callback);
  }

  void RegisterTheCurseOfKaZarSignalHandler(
      const base::Callback<void(const std::vector<std::string>&,
                                uint8_t)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "TheCurseOfKaZar",
        signal_callback,
        on_connected_callback);
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

  bool Elements(
      const std::string& in_space_walk,
      const std::vector<dbus::ObjectPath>& in_ramblin_man,
      std::string* out_3,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "Elements",
        error,
        in_space_walk,
        in_ramblin_man);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_3);
  }

  void ElementsAsync(
      const std::string& in_space_walk,
      const std::vector<dbus::ObjectPath>& in_ramblin_man,
      const base::Callback<void(const std::string&)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "Elements",
        success_callback,
        error_callback,
        in_space_walk,
        in_ramblin_man);
  }

  bool ReturnToPatagonia(
      int64_t* out_1,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "ReturnToPatagonia",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_1);
  }

  void ReturnToPatagoniaAsync(
      const base::Callback<void(int64_t)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "ReturnToPatagonia",
        success_callback,
        error_callback);
  }

  bool NiceWeatherForDucks(
      bool in_1,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "NiceWeatherForDucks",
        error,
        in_1);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void NiceWeatherForDucksAsync(
      bool in_1,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "NiceWeatherForDucks",
        success_callback,
        error_callback,
        in_1);
  }

  // Comment line1
  // line2
  bool ExperimentNumberSix(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "ExperimentNumberSix",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // Comment line1
  // line2
  void ExperimentNumberSixAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "ExperimentNumberSix",
        success_callback,
        error_callback);
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  std::string service_name_;
  const dbus::ObjectPath object_path_{"/org/chromium/Test"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(TestInterfaceProxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::TestInterface2.
class TestInterface2ProxyInterface {
 public:
  virtual ~TestInterface2ProxyInterface() = default;

  virtual bool GetPersonInfo(
      std::string* out_name,
      int32_t* out_age,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void GetPersonInfoAsync(
      const base::Callback<void(const std::string& /*name*/, int32_t /*age*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::TestInterface2.
class TestInterface2Proxy final : public TestInterface2ProxyInterface {
 public:
  TestInterface2Proxy(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name,
      const dbus::ObjectPath& object_path) :
          bus_{bus},
          service_name_{service_name},
          object_path_{object_path},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~TestInterface2Proxy() override {
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

  bool GetPersonInfo(
      std::string* out_name,
      int32_t* out_age,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface2",
        "GetPersonInfo",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_name, out_age);
  }

  void GetPersonInfoAsync(
      const base::Callback<void(const std::string& /*name*/, int32_t /*age*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.TestInterface2",
        "GetPersonInfo",
        success_callback,
        error_callback);
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  std::string service_name_;
  dbus::ObjectPath object_path_;
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(TestInterface2Proxy);
};

}  // namespace chromium
}  // namespace org
)literal_string";

const char kExpectedContentWithService[] = R"literal_string(
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <brillo/dbus/dbus_property.h>
#include <brillo/dbus/dbus_signal_handler.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/object_manager.h>
#include <dbus/object_path.h>
#include <dbus/object_proxy.h>

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::TestInterface.
class TestInterfaceProxyInterface {
 public:
  virtual ~TestInterfaceProxyInterface() = default;

  virtual void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::TestInterface.
class TestInterfaceProxy final : public TestInterfaceProxyInterface {
 public:
  TestInterfaceProxy(const scoped_refptr<dbus::Bus>& bus) :
      bus_{bus},
      dbus_object_proxy_{
          bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~TestInterfaceProxy() override {
  }

  void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.TestInterface",
        "Closer",
        signal_callback,
        on_connected_callback);
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.Test"};
  const dbus::ObjectPath object_path_{"/org/chromium/Test"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(TestInterfaceProxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::TestInterface2.
class TestInterface2ProxyInterface {
 public:
  virtual ~TestInterface2ProxyInterface() = default;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::TestInterface2.
class TestInterface2Proxy final : public TestInterface2ProxyInterface {
 public:
  TestInterface2Proxy(
      const scoped_refptr<dbus::Bus>& bus,
      const dbus::ObjectPath& object_path) :
          bus_{bus},
          object_path_{object_path},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~TestInterface2Proxy() override {
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.Test"};
  dbus::ObjectPath object_path_;
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(TestInterface2Proxy);
};

}  // namespace chromium
}  // namespace org
)literal_string";

const char kExpectedContentWithObjectManager[] = R"literal_string(
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <brillo/dbus/dbus_property.h>
#include <brillo/dbus/dbus_signal_handler.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/object_manager.h>
#include <dbus/object_path.h>
#include <dbus/object_proxy.h>

namespace org {
namespace chromium {
class ObjectManagerProxy;
}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::Itf1.
class Itf1ProxyInterface {
 public:
  virtual ~Itf1ProxyInterface() = default;

  virtual void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  static const char* DataName() { return "Data"; }
  virtual const std::string& data() const = 0;
  static const char* NameName() { return "Name"; }
  virtual const std::string& name() const = 0;
  virtual void set_name(const std::string& value,
                        const base::Callback<void(bool)>& callback) = 0;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;

  virtual void SetPropertyChangedCallback(
      const base::Callback<void(Itf1ProxyInterface*, const std::string&)>& callback) = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::Itf1.
class Itf1Proxy final : public Itf1ProxyInterface {
 public:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const PropertyChangedCallback& callback)
        : dbus::PropertySet{object_proxy,
                            "org.chromium.Itf1",
                            callback} {
      RegisterProperty(DataName(), &data);
      RegisterProperty(NameName(), &name);
    }

    brillo::dbus_utils::Property<std::string> data;
    brillo::dbus_utils::Property<std::string> name;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  Itf1Proxy(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name,
      PropertySet* property_set) :
          bus_{bus},
          service_name_{service_name},
          property_set_{property_set},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~Itf1Proxy() override {
  }

  void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.Itf1",
        "Closer",
        signal_callback,
        on_connected_callback);
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

  void SetPropertyChangedCallback(
      const base::Callback<void(Itf1ProxyInterface*, const std::string&)>& callback) override {
    on_property_changed_ = callback;
  }

  const PropertySet* GetProperties() const { return property_set_; }
  PropertySet* GetProperties() { return property_set_; }

  const std::string& data() const override {
    return property_set_->data.value();
  }

  const std::string& name() const override {
    return property_set_->name.value();
  }

  void set_name(const std::string& value,
                const base::Callback<void(bool)>& callback) override {
    property_set_->name.Set(value, callback);
  }

 private:
  void OnPropertyChanged(const std::string& property_name) {
    if (!on_property_changed_.is_null())
      on_property_changed_.Run(this, property_name);
  }

  scoped_refptr<dbus::Bus> bus_;
  std::string service_name_;
  const dbus::ObjectPath object_path_{"/org/chromium/Test/Object"};
  PropertySet* property_set_;
  base::Callback<void(Itf1ProxyInterface*, const std::string&)> on_property_changed_;
  dbus::ObjectProxy* dbus_object_proxy_;

  friend class org::chromium::ObjectManagerProxy;
  DISALLOW_COPY_AND_ASSIGN(Itf1Proxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::Itf2.
class Itf2ProxyInterface {
 public:
  virtual ~Itf2ProxyInterface() = default;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::Itf2.
class Itf2Proxy final : public Itf2ProxyInterface {
 public:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const PropertyChangedCallback& callback)
        : dbus::PropertySet{object_proxy,
                            "org.chromium.Itf2",
                            callback} {
    }


   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  Itf2Proxy(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name,
      const dbus::ObjectPath& object_path) :
          bus_{bus},
          service_name_{service_name},
          object_path_{object_path},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~Itf2Proxy() override {
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

 private:
  scoped_refptr<dbus::Bus> bus_;
  std::string service_name_;
  dbus::ObjectPath object_path_;
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(Itf2Proxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

class ObjectManagerProxy : public dbus::ObjectManager::Interface {
 public:
  ObjectManagerProxy(const scoped_refptr<dbus::Bus>& bus,
                     const std::string& service_name)
      : bus_{bus},
        service_name_{service_name},
        dbus_object_manager_{bus->GetObjectManager(
            service_name,
            dbus::ObjectPath{"/org/chromium/Test"})} {
    dbus_object_manager_->RegisterInterface("org.chromium.Itf1", this);
    dbus_object_manager_->RegisterInterface("org.chromium.Itf2", this);
  }

  ~ObjectManagerProxy() override {
    dbus_object_manager_->UnregisterInterface("org.chromium.Itf1");
    dbus_object_manager_->UnregisterInterface("org.chromium.Itf2");
  }

  dbus::ObjectManager* GetObjectManagerProxy() const {
    return dbus_object_manager_;
  }

  org::chromium::Itf1ProxyInterface* GetItf1Proxy() {
    if (itf1_instances_.empty())
      return nullptr;
    return itf1_instances_.begin()->second.get();
  }
  std::vector<org::chromium::Itf1ProxyInterface*> GetItf1Instances() const {
    std::vector<org::chromium::Itf1ProxyInterface*> values;
    values.reserve(itf1_instances_.size());
    for (const auto& pair : itf1_instances_)
      values.push_back(pair.second.get());
    return values;
  }
  void SetItf1AddedCallback(
      const base::Callback<void(org::chromium::Itf1ProxyInterface*)>& callback) {
    on_itf1_added_ = callback;
  }
  void SetItf1RemovedCallback(
      const base::Callback<void(const dbus::ObjectPath&)>& callback) {
    on_itf1_removed_ = callback;
  }

  org::chromium::Itf2ProxyInterface* GetItf2Proxy(
      const dbus::ObjectPath& object_path) {
    auto p = itf2_instances_.find(object_path);
    if (p != itf2_instances_.end())
      return p->second.get();
    return nullptr;
  }
  std::vector<org::chromium::Itf2ProxyInterface*> GetItf2Instances() const {
    std::vector<org::chromium::Itf2ProxyInterface*> values;
    values.reserve(itf2_instances_.size());
    for (const auto& pair : itf2_instances_)
      values.push_back(pair.second.get());
    return values;
  }
  void SetItf2AddedCallback(
      const base::Callback<void(org::chromium::Itf2ProxyInterface*)>& callback) {
    on_itf2_added_ = callback;
  }
  void SetItf2RemovedCallback(
      const base::Callback<void(const dbus::ObjectPath&)>& callback) {
    on_itf2_removed_ = callback;
  }

 private:
  void OnPropertyChanged(const dbus::ObjectPath& object_path,
                         const std::string& interface_name,
                         const std::string& property_name) {
    if (interface_name == "org.chromium.Itf1") {
      auto p = itf1_instances_.find(object_path);
      if (p == itf1_instances_.end())
        return;
      p->second->OnPropertyChanged(property_name);
      return;
    }
  }

  void ObjectAdded(
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      auto property_set =
          static_cast<org::chromium::Itf1Proxy::PropertySet*>(
              dbus_object_manager_->GetProperties(object_path, interface_name));
      std::unique_ptr<org::chromium::Itf1Proxy> itf1_proxy{
        new org::chromium::Itf1Proxy{bus_, service_name_, property_set}
      };
      auto p = itf1_instances_.emplace(object_path, std::move(itf1_proxy));
      if (!on_itf1_added_.is_null())
        on_itf1_added_.Run(p.first->second.get());
      return;
    }
    if (interface_name == "org.chromium.Itf2") {
      std::unique_ptr<org::chromium::Itf2Proxy> itf2_proxy{
        new org::chromium::Itf2Proxy{bus_, service_name_, object_path}
      };
      auto p = itf2_instances_.emplace(object_path, std::move(itf2_proxy));
      if (!on_itf2_added_.is_null())
        on_itf2_added_.Run(p.first->second.get());
      return;
    }
  }

  void ObjectRemoved(
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      auto p = itf1_instances_.find(object_path);
      if (p != itf1_instances_.end()) {
        if (!on_itf1_removed_.is_null())
          on_itf1_removed_.Run(object_path);
        itf1_instances_.erase(p);
      }
      return;
    }
    if (interface_name == "org.chromium.Itf2") {
      auto p = itf2_instances_.find(object_path);
      if (p != itf2_instances_.end()) {
        if (!on_itf2_removed_.is_null())
          on_itf2_removed_.Run(object_path);
        itf2_instances_.erase(p);
      }
      return;
    }
  }

  dbus::PropertySet* CreateProperties(
      dbus::ObjectProxy* object_proxy,
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      return new org::chromium::Itf1Proxy::PropertySet{
          object_proxy,
          base::Bind(&ObjectManagerProxy::OnPropertyChanged,
                     weak_ptr_factory_.GetWeakPtr(),
                     object_path,
                     interface_name)
      };
    }
    if (interface_name == "org.chromium.Itf2") {
      return new org::chromium::Itf2Proxy::PropertySet{
          object_proxy,
          base::Bind(&ObjectManagerProxy::OnPropertyChanged,
                     weak_ptr_factory_.GetWeakPtr(),
                     object_path,
                     interface_name)
      };
    }
    LOG(FATAL) << "Creating properties for unsupported interface "
               << interface_name;
    return nullptr;
  }

  scoped_refptr<dbus::Bus> bus_;
  std::string service_name_;
  dbus::ObjectManager* dbus_object_manager_;
  std::map<dbus::ObjectPath,
           std::unique_ptr<org::chromium::Itf1Proxy>> itf1_instances_;
  base::Callback<void(org::chromium::Itf1ProxyInterface*)> on_itf1_added_;
  base::Callback<void(const dbus::ObjectPath&)> on_itf1_removed_;
  std::map<dbus::ObjectPath,
           std::unique_ptr<org::chromium::Itf2Proxy>> itf2_instances_;
  base::Callback<void(org::chromium::Itf2ProxyInterface*)> on_itf2_added_;
  base::Callback<void(const dbus::ObjectPath&)> on_itf2_removed_;
  base::WeakPtrFactory<ObjectManagerProxy> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ObjectManagerProxy);
};

}  // namespace chromium
}  // namespace org
)literal_string";

const char kExpectedContentWithObjectManagerAndServiceName[] = R"literal_string(
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <brillo/dbus/dbus_property.h>
#include <brillo/dbus/dbus_signal_handler.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/object_manager.h>
#include <dbus/object_path.h>
#include <dbus/object_proxy.h>

namespace org {
namespace chromium {
class ObjectManagerProxy;
}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::Itf1.
class Itf1ProxyInterface {
 public:
  virtual ~Itf1ProxyInterface() = default;

  virtual void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::Itf1.
class Itf1Proxy final : public Itf1ProxyInterface {
 public:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const PropertyChangedCallback& callback)
        : dbus::PropertySet{object_proxy,
                            "org.chromium.Itf1",
                            callback} {
    }


   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  Itf1Proxy(const scoped_refptr<dbus::Bus>& bus) :
      bus_{bus},
      dbus_object_proxy_{
          bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~Itf1Proxy() override {
  }

  void RegisterCloserSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.Itf1",
        "Closer",
        signal_callback,
        on_connected_callback);
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.Test"};
  const dbus::ObjectPath object_path_{"/org/chromium/Test/Object"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(Itf1Proxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Abstract interface proxy for org::chromium::Itf2.
class Itf2ProxyInterface {
 public:
  virtual ~Itf2ProxyInterface() = default;

  virtual const dbus::ObjectPath& GetObjectPath() const = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::Itf2.
class Itf2Proxy final : public Itf2ProxyInterface {
 public:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const PropertyChangedCallback& callback)
        : dbus::PropertySet{object_proxy,
                            "org.chromium.Itf2",
                            callback} {
    }


   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  Itf2Proxy(
      const scoped_refptr<dbus::Bus>& bus,
      const dbus::ObjectPath& object_path) :
          bus_{bus},
          object_path_{object_path},
          dbus_object_proxy_{
              bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~Itf2Proxy() override {
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const override {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.Test"};
  dbus::ObjectPath object_path_;
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(Itf2Proxy);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

class ObjectManagerProxy : public dbus::ObjectManager::Interface {
 public:
  ObjectManagerProxy(const scoped_refptr<dbus::Bus>& bus)
      : bus_{bus},
        dbus_object_manager_{bus->GetObjectManager(
            "org.chromium.Test",
            dbus::ObjectPath{"/org/chromium/Test"})} {
    dbus_object_manager_->RegisterInterface("org.chromium.Itf1", this);
    dbus_object_manager_->RegisterInterface("org.chromium.Itf2", this);
  }

  ~ObjectManagerProxy() override {
    dbus_object_manager_->UnregisterInterface("org.chromium.Itf1");
    dbus_object_manager_->UnregisterInterface("org.chromium.Itf2");
  }

  dbus::ObjectManager* GetObjectManagerProxy() const {
    return dbus_object_manager_;
  }

  org::chromium::Itf1ProxyInterface* GetItf1Proxy() {
    if (itf1_instances_.empty())
      return nullptr;
    return itf1_instances_.begin()->second.get();
  }
  std::vector<org::chromium::Itf1ProxyInterface*> GetItf1Instances() const {
    std::vector<org::chromium::Itf1ProxyInterface*> values;
    values.reserve(itf1_instances_.size());
    for (const auto& pair : itf1_instances_)
      values.push_back(pair.second.get());
    return values;
  }
  void SetItf1AddedCallback(
      const base::Callback<void(org::chromium::Itf1ProxyInterface*)>& callback) {
    on_itf1_added_ = callback;
  }
  void SetItf1RemovedCallback(
      const base::Callback<void(const dbus::ObjectPath&)>& callback) {
    on_itf1_removed_ = callback;
  }

  org::chromium::Itf2ProxyInterface* GetItf2Proxy(
      const dbus::ObjectPath& object_path) {
    auto p = itf2_instances_.find(object_path);
    if (p != itf2_instances_.end())
      return p->second.get();
    return nullptr;
  }
  std::vector<org::chromium::Itf2ProxyInterface*> GetItf2Instances() const {
    std::vector<org::chromium::Itf2ProxyInterface*> values;
    values.reserve(itf2_instances_.size());
    for (const auto& pair : itf2_instances_)
      values.push_back(pair.second.get());
    return values;
  }
  void SetItf2AddedCallback(
      const base::Callback<void(org::chromium::Itf2ProxyInterface*)>& callback) {
    on_itf2_added_ = callback;
  }
  void SetItf2RemovedCallback(
      const base::Callback<void(const dbus::ObjectPath&)>& callback) {
    on_itf2_removed_ = callback;
  }

 private:
  void OnPropertyChanged(const dbus::ObjectPath& /* object_path */,
                         const std::string& /* interface_name */,
                         const std::string& /* property_name */) {}

  void ObjectAdded(
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      std::unique_ptr<org::chromium::Itf1Proxy> itf1_proxy{
        new org::chromium::Itf1Proxy{bus_}
      };
      auto p = itf1_instances_.emplace(object_path, std::move(itf1_proxy));
      if (!on_itf1_added_.is_null())
        on_itf1_added_.Run(p.first->second.get());
      return;
    }
    if (interface_name == "org.chromium.Itf2") {
      std::unique_ptr<org::chromium::Itf2Proxy> itf2_proxy{
        new org::chromium::Itf2Proxy{bus_, object_path}
      };
      auto p = itf2_instances_.emplace(object_path, std::move(itf2_proxy));
      if (!on_itf2_added_.is_null())
        on_itf2_added_.Run(p.first->second.get());
      return;
    }
  }

  void ObjectRemoved(
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      auto p = itf1_instances_.find(object_path);
      if (p != itf1_instances_.end()) {
        if (!on_itf1_removed_.is_null())
          on_itf1_removed_.Run(object_path);
        itf1_instances_.erase(p);
      }
      return;
    }
    if (interface_name == "org.chromium.Itf2") {
      auto p = itf2_instances_.find(object_path);
      if (p != itf2_instances_.end()) {
        if (!on_itf2_removed_.is_null())
          on_itf2_removed_.Run(object_path);
        itf2_instances_.erase(p);
      }
      return;
    }
  }

  dbus::PropertySet* CreateProperties(
      dbus::ObjectProxy* object_proxy,
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    if (interface_name == "org.chromium.Itf1") {
      return new org::chromium::Itf1Proxy::PropertySet{
          object_proxy,
          base::Bind(&ObjectManagerProxy::OnPropertyChanged,
                     weak_ptr_factory_.GetWeakPtr(),
                     object_path,
                     interface_name)
      };
    }
    if (interface_name == "org.chromium.Itf2") {
      return new org::chromium::Itf2Proxy::PropertySet{
          object_proxy,
          base::Bind(&ObjectManagerProxy::OnPropertyChanged,
                     weak_ptr_factory_.GetWeakPtr(),
                     object_path,
                     interface_name)
      };
    }
    LOG(FATAL) << "Creating properties for unsupported interface "
               << interface_name;
    return nullptr;
  }

  scoped_refptr<dbus::Bus> bus_;
  dbus::ObjectManager* dbus_object_manager_;
  std::map<dbus::ObjectPath,
           std::unique_ptr<org::chromium::Itf1Proxy>> itf1_instances_;
  base::Callback<void(org::chromium::Itf1ProxyInterface*)> on_itf1_added_;
  base::Callback<void(const dbus::ObjectPath&)> on_itf1_removed_;
  std::map<dbus::ObjectPath,
           std::unique_ptr<org::chromium::Itf2Proxy>> itf2_instances_;
  base::Callback<void(org::chromium::Itf2ProxyInterface*)> on_itf2_added_;
  base::Callback<void(const dbus::ObjectPath&)> on_itf2_removed_;
  base::WeakPtrFactory<ObjectManagerProxy> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ObjectManagerProxy);
};

}  // namespace chromium
}  // namespace org
)literal_string";
}  // namespace

class ProxyGeneratorTest : public Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  base::FilePath CreateInputFile(const string& contents) {
    base::FilePath path;
    EXPECT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.path(), &path));
    int written = base::WriteFile(path, contents.c_str(), contents.size());
    EXPECT_EQ(contents.size(), static_cast<size_t>(written));
    return path;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(ProxyGeneratorTest, GenerateAdaptors) {
  Interface interface;
  interface.name = "org.chromium.TestInterface";
  interface.path = "/org/chromium/Test";
  interface.methods.emplace_back(
      "Elements",
      vector<Interface::Argument>{
          {"space_walk", kDBusTypeString},
          {"ramblin_man", kDBusTypeArryOfObjects}},
      vector<Interface::Argument>{{"", kDBusTypeString}});
  interface.methods.emplace_back(
      "ReturnToPatagonia",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{{"", kDBusTypeInt64}});
  interface.methods.emplace_back(
      "NiceWeatherForDucks",
      vector<Interface::Argument>{{"", kDBusTypeBool}},
      vector<Interface::Argument>{});
  interface.methods.emplace_back("ExperimentNumberSix");
  interface.signals.emplace_back("Closer");
  interface.signals.emplace_back(
      "TheCurseOfKaZar",
      vector<Interface::Argument>{
          {"", kDBusTypeArryOfStrings},
          {"", kDBusTypeByte}});
  interface.methods.back().doc_string = "Comment line1\nline2";
  Interface interface2;
  interface2.name = "org.chromium.TestInterface2";
  interface2.methods.emplace_back(
      "GetPersonInfo",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{
          {"name", kDBusTypeString},
          {"age", kDBusTypeInt32}});
  vector<Interface> interfaces{interface, interface2};
  base::FilePath output_path = temp_dir_.path().Append("output.h");
  ServiceConfig config;
  EXPECT_TRUE(ProxyGenerator::GenerateProxies(config, interfaces, output_path));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(kExpectedContent, contents);
}

TEST_F(ProxyGeneratorTest, GenerateAdaptorsWithServiceName) {
  Interface interface;
  interface.name = "org.chromium.TestInterface";
  interface.path = "/org/chromium/Test";
  interface.signals.emplace_back("Closer");
  Interface interface2;
  interface2.name = "org.chromium.TestInterface2";
  vector<Interface> interfaces{interface, interface2};
  base::FilePath output_path = temp_dir_.path().Append("output2.h");
  ServiceConfig config;
  config.service_name = "org.chromium.Test";
  EXPECT_TRUE(ProxyGenerator::GenerateProxies(config, interfaces, output_path));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(kExpectedContentWithService, contents);
}

TEST_F(ProxyGeneratorTest, GenerateAdaptorsWithObjectManager) {
  Interface interface;
  interface.name = "org.chromium.Itf1";
  interface.path = "/org/chromium/Test/Object";
  interface.signals.emplace_back("Closer");
  interface.properties.emplace_back("Data", "s", "read");
  interface.properties.emplace_back("Name", "s", "readwrite");
  Interface interface2;
  interface2.name = "org.chromium.Itf2";
  vector<Interface> interfaces{interface, interface2};
  base::FilePath output_path = temp_dir_.path().Append("output3.h");
  ServiceConfig config;
  config.object_manager.name = "org.chromium.ObjectManager";
  config.object_manager.object_path = "/org/chromium/Test";
  EXPECT_TRUE(ProxyGenerator::GenerateProxies(config, interfaces, output_path));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(
      kExpectedContentWithObjectManager, contents);
}

TEST_F(ProxyGeneratorTest, GenerateAdaptorsWithObjectManagerAndServiceName) {
  Interface interface;
  interface.name = "org.chromium.Itf1";
  interface.path = "/org/chromium/Test/Object";
  interface.signals.emplace_back("Closer");
  Interface interface2;
  interface2.name = "org.chromium.Itf2";
  vector<Interface> interfaces{interface, interface2};
  base::FilePath output_path = temp_dir_.path().Append("output4.h");
  ServiceConfig config;
  config.service_name = "org.chromium.Test";
  config.object_manager.name = "org.chromium.ObjectManager";
  config.object_manager.object_path = "/org/chromium/Test";
  EXPECT_TRUE(ProxyGenerator::GenerateProxies(config, interfaces, output_path));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(
      kExpectedContentWithObjectManagerAndServiceName, contents);
}

}  // namespace chromeos_dbus_bindings
