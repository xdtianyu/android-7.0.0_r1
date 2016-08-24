//
// Copyright (C) 2015 The Android Open Source Project
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
#include <time.h>

#include <base/message_loop/message_loop.h>

#include "proxy_dbus_client.h"

const char ProxyDbusClient::kCommonLogScopes[] =
  "connection+dbus+device+link+manager+portal+service";
const int ProxyDbusClient::kLogLevel = -4;
const char ProxyDbusClient::kDbusErrorObjectUnknown[] =
  "org.freedesktop.DBus.Error.UnknownObject";

namespace {
template<typename Proxy> bool GetPropertyValueFromProxy(
    Proxy* proxy,
    const std::string& property_name,
    brillo::Any* property_value) {
  CHECK(property_value);
  brillo::VariantDictionary proxy_properties;
  brillo::ErrorPtr error;
  CHECK(proxy->GetProperties(&proxy_properties, &error));
  if (proxy_properties.find(property_name) == proxy_properties.end()) {
    return false;
  }
  *property_value = proxy_properties[property_name];
  return true;
}

template<typename Proxy> void IsProxyPropertyValueIn(
    Proxy* proxy,
    const std::string& property_name,
    const std::vector<brillo::Any>& expected_values,
    base::Time wait_start_time,
    bool* is_success,
    brillo::Any* final_value,
    long* elapsed_time_milliseconds) {
  brillo::Any property_value;
  *is_success = false;
  if ((GetPropertyValueFromProxy<Proxy>(proxy, property_name, &property_value)) &&
      (std::find(expected_values.begin(), expected_values.end(),
                 property_value) != expected_values.end())) {
    *is_success = true;
  }
  if (final_value) {
    *final_value = property_value;
  }
  if (elapsed_time_milliseconds) {
    *elapsed_time_milliseconds =
        (base::Time::Now() - wait_start_time).InMilliseconds();
  }
}

// This is invoked when the dbus detects a change in one of
// the properties of the proxy. We need to check if the property
// we're interested in has reached one of the expected values.
void PropertyChangedSignalCallback(
    const std::string& watched_property_name,
    const std::vector<brillo::Any>& expected_values,
    const std::string& changed_property_name,
    const brillo::Any& new_property_value) {
  if ((watched_property_name == changed_property_name) &&
      (std::find(expected_values.begin(), expected_values.end(),
                 new_property_value) != expected_values.end())) {
    // Unblock the waiting function by stopping the message loop.
    base::MessageLoop::current()->QuitNow();
  }
}

// This is invoked to indicate whether dbus successfully connected our
// signal callback or not.
void PropertyChangedOnConnectedCallback(
    const std::string& /* watched_property_name */,
    const std::string& /* interface */,
    const std::string& /* signal_name */,
    bool success) {
  CHECK(success);
}

template<typename Proxy>
void HelpRegisterPropertyChangedSignalHandler(
    Proxy* proxy,
    dbus::ObjectProxy::OnConnectedCallback on_connected_callback,
    const DbusPropertyChangeCallback& signal_callback) {
  // Re-order |on_connected_callback| and |signal_callback|, to meet
  // the requirements of RegisterPropertyChangedSignalHandler().
  proxy->RegisterPropertyChangedSignalHandler(
      signal_callback, on_connected_callback);
}

template<typename OutValueType, typename ConditionChangeCallbackType>
void WaitForCondition(
    base::Callback<void(base::Time, bool*, OutValueType*, long*)>
        condition_termination_checker,
    base::Callback<ConditionChangeCallbackType> condition_change_callback,
    base::Callback<void(const base::Callback<ConditionChangeCallbackType>&)>
        condition_change_callback_registrar,
    long timeout_milliseconds,
    bool* is_success,
    OutValueType* out_value,
    long* elapsed_time_milliseconds) {
  CHECK(is_success);
  const base::Time wait_start_time(base::Time::Now());
  const base::TimeDelta timeout(
      base::TimeDelta::FromMilliseconds(timeout_milliseconds));
  base::CancelableClosure wait_timeout_callback;
  base::CancelableCallback<ConditionChangeCallbackType> change_callback;

  condition_termination_checker.Run(
      wait_start_time, is_success, out_value, elapsed_time_milliseconds);
  if (*is_success) {
    return;
  }

  wait_timeout_callback.Reset(base::MessageLoop::QuitWhenIdleClosure());
  change_callback.Reset(condition_change_callback);

  condition_change_callback_registrar.Run(change_callback.callback());

  // Add timeout, in case we never hit the expected condition.
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      wait_timeout_callback.callback(),
      timeout);

  // Wait for the condition to occur within |timeout_milliseconds|.
  base::MessageLoop::current()->Run();

  wait_timeout_callback.Cancel();
  change_callback.Cancel();

  // We could have reached here either because we timed out or
  // because we reached the condition.
  condition_termination_checker.Run(
      wait_start_time, is_success, out_value, elapsed_time_milliseconds);
}
} // namespace

ProxyDbusClient::ProxyDbusClient(scoped_refptr<dbus::Bus> bus)
  : dbus_bus_(bus),
    shill_manager_proxy_(dbus_bus_),
    weak_ptr_factory_(this) {
}

void ProxyDbusClient::SetLogging(Technology tech) {
  std::string log_scopes(kCommonLogScopes);
  switch (tech) {
    case TECHNOLOGY_CELLULAR:
      log_scopes += "+cellular";
      break;
    case TECHNOLOGY_ETHERNET:
      log_scopes += "+ethernet";
      break;
    case TECHNOLOGY_VPN:
      log_scopes += "+vpn";
      break;
    case TECHNOLOGY_WIFI:
      log_scopes += "+wifi";
      break;
    case TECHNOLOGY_WIMAX:
      log_scopes += "+wimax";
      break;
  }
  SetLoggingInternal(kLogLevel, log_scopes);
}

std::vector<std::unique_ptr<DeviceProxy>> ProxyDbusClient::GetDeviceProxies() {
  return GetProxies<DeviceProxy>(shill::kDevicesProperty);
}

std::vector<std::unique_ptr<ServiceProxy>> ProxyDbusClient::GetServiceProxies() {
  return GetProxies<ServiceProxy>(shill::kServicesProperty);
}

std::vector<std::unique_ptr<ProfileProxy>> ProxyDbusClient::GetProfileProxies() {
  return GetProxies<ProfileProxy>(shill::kProfilesProperty);
}

std::unique_ptr<DeviceProxy> ProxyDbusClient::GetMatchingDeviceProxy(
    const brillo::VariantDictionary& expected_properties) {
  return GetMatchingProxy<DeviceProxy>(shill::kDevicesProperty, expected_properties);
}

std::unique_ptr<ServiceProxy> ProxyDbusClient::GetMatchingServiceProxy(
    const brillo::VariantDictionary& expected_properties) {
  return GetMatchingProxy<ServiceProxy>(shill::kServicesProperty, expected_properties);
}

std::unique_ptr<ProfileProxy> ProxyDbusClient::GetMatchingProfileProxy(
    const brillo::VariantDictionary& expected_properties) {
  return GetMatchingProxy<ProfileProxy>(shill::kProfilesProperty, expected_properties);
}

bool ProxyDbusClient::GetPropertyValueFromDeviceProxy(
    DeviceProxy* proxy,
    const std::string& property_name,
    brillo::Any* property_value) {
  return GetPropertyValueFromProxy<DeviceProxy>(
      proxy, property_name, property_value);
}

bool ProxyDbusClient::GetPropertyValueFromServiceProxy(
    ServiceProxy* proxy,
    const std::string& property_name,
    brillo::Any* property_value) {
  return GetPropertyValueFromProxy<ServiceProxy>(
      proxy, property_name, property_value);
}

bool ProxyDbusClient::GetPropertyValueFromProfileProxy(
    ProfileProxy* proxy,
    const std::string& property_name,
    brillo::Any* property_value) {
  return GetPropertyValueFromProxy<ProfileProxy>(
      proxy, property_name, property_value);
}

bool ProxyDbusClient::WaitForDeviceProxyPropertyValueIn(
    const dbus::ObjectPath& object_path,
    const std::string& property_name,
    const std::vector<brillo::Any>& expected_values,
    long timeout_milliseconds,
    brillo::Any* final_value,
    long* elapsed_time_milliseconds) {
  return WaitForProxyPropertyValueIn<DeviceProxy>(
      object_path, property_name, expected_values, timeout_milliseconds,
      final_value, elapsed_time_milliseconds);
}

bool ProxyDbusClient::WaitForServiceProxyPropertyValueIn(
    const dbus::ObjectPath& object_path,
    const std::string& property_name,
    const std::vector<brillo::Any>& expected_values,
    long timeout_milliseconds,
    brillo::Any* final_value,
    long* elapsed_time_milliseconds) {
  return WaitForProxyPropertyValueIn<ServiceProxy>(
      object_path, property_name, expected_values, timeout_milliseconds,
      final_value, elapsed_time_milliseconds);
}

bool ProxyDbusClient::WaitForProfileProxyPropertyValueIn(
    const dbus::ObjectPath& object_path,
    const std::string& property_name,
    const std::vector<brillo::Any>& expected_values,
    long timeout_milliseconds,
    brillo::Any* final_value,
    long* elapsed_time_milliseconds) {
  return WaitForProxyPropertyValueIn<ProfileProxy>(
      object_path, property_name, expected_values, timeout_milliseconds,
      final_value, elapsed_time_milliseconds);
}

std::unique_ptr<ServiceProxy> ProxyDbusClient::GetServiceProxy(
    const brillo::VariantDictionary& expected_properties) {
  dbus::ObjectPath service_path;
  brillo::ErrorPtr error;
  if (!shill_manager_proxy_.GetService(
          expected_properties, &service_path, &error)) {
    return nullptr;
  }
  return std::unique_ptr<ServiceProxy>(
      new ServiceProxy(dbus_bus_, service_path));
}

std::unique_ptr<ProfileProxy> ProxyDbusClient::GetActiveProfileProxy() {
  return GetProxyForObjectPath<ProfileProxy>(GetObjectPathForActiveProfile());
}

std::unique_ptr<ServiceProxy> ProxyDbusClient::WaitForMatchingServiceProxy(
    const brillo::VariantDictionary& service_properties,
    const std::string& service_type,
    long timeout_milliseconds,
    int rescan_interval_milliseconds,
    long* elapsed_time_milliseconds) {
  auto condition_termination_checker =
      base::Bind(&ProxyDbusClient::IsMatchingServicePresent,
                 weak_ptr_factory_.GetWeakPtr(),
                 service_properties);
  auto condition_change_callback =
      base::Bind(&ProxyDbusClient::FindServiceOrRestartScan,
                 weak_ptr_factory_.GetWeakPtr(),
                 service_properties,
                 service_type);
  auto condition_change_callback_registrar =
      base::Bind(&ProxyDbusClient::InitiateScanForService,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::TimeDelta::FromMilliseconds(rescan_interval_milliseconds),
                 service_type);

  std::unique_ptr<ServiceProxy> service_proxy;
  bool is_success;
  WaitForCondition(
      condition_termination_checker, condition_change_callback,
      condition_change_callback_registrar,
      timeout_milliseconds, &is_success, &service_proxy, elapsed_time_milliseconds);
  return service_proxy;
}

bool ProxyDbusClient::ConfigureService(
    const brillo::VariantDictionary& config_params) {
  dbus::ObjectPath service_path;
  brillo::ErrorPtr error;
  return shill_manager_proxy_.ConfigureService(
      config_params, &service_path, &error);
}

bool ProxyDbusClient::ConfigureServiceByGuid(
    const std::string& guid,
    const brillo::VariantDictionary& config_params) {
  dbus::ObjectPath service_path;
  brillo::ErrorPtr error;
  brillo::VariantDictionary guid_config_params(config_params);
  guid_config_params[shill::kGuidProperty] = guid;
  return shill_manager_proxy_.ConfigureService(
      guid_config_params, &service_path, &error);
}

bool ProxyDbusClient::ConnectService(
    const dbus::ObjectPath& object_path,
    long timeout_milliseconds) {
  auto proxy = GetProxyForObjectPath<ServiceProxy>(object_path);
  brillo::ErrorPtr error;
  if (!proxy->Connect(&error)) {
    return false;
  }
  const std::vector<brillo::Any> expected_values = {
    brillo::Any(std::string(shill::kStatePortal)),
    brillo::Any(std::string(shill::kStateOnline)) };
  return WaitForProxyPropertyValueIn<ServiceProxy>(
      object_path, shill::kStateProperty, expected_values,
      timeout_milliseconds, nullptr, nullptr);
}

bool ProxyDbusClient::DisconnectService(
    const dbus::ObjectPath& object_path,
    long timeout_milliseconds) {
  auto proxy = GetProxyForObjectPath<ServiceProxy>(object_path);
  brillo::ErrorPtr error;
  if (!proxy->Disconnect(&error)) {
    return false;
  }
  const std::vector<brillo::Any> expected_values = {
    brillo::Any(std::string(shill::kStateIdle)) };
  return WaitForProxyPropertyValueIn<ServiceProxy>(
      object_path, shill::kStateProperty, expected_values,
      timeout_milliseconds, nullptr, nullptr);
}

bool ProxyDbusClient::CreateProfile(const std::string& profile_name) {
  dbus::ObjectPath profile_path;
  brillo::ErrorPtr error;
  return shill_manager_proxy_.CreateProfile(
      profile_name, &profile_path, &error);
}

bool ProxyDbusClient::RemoveProfile(const std::string& profile_name) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.RemoveProfile(profile_name, &error);
}

bool ProxyDbusClient::PushProfile(const std::string& profile_name) {
  dbus::ObjectPath profile_path;
  brillo::ErrorPtr error;
  return shill_manager_proxy_.PushProfile(
      profile_name, &profile_path, &error);
}

bool ProxyDbusClient::PopProfile(const std::string& profile_name) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.PopProfile(profile_name, &error);
}

bool ProxyDbusClient::PopAnyProfile() {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.PopAnyProfile(&error);
}

bool ProxyDbusClient::RequestServiceScan(const std::string& service_type) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.RequestScan(service_type, &error);
}

bool ProxyDbusClient::GetServiceOrder(std::string* order) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.GetServiceOrder(order, &error);
}

bool ProxyDbusClient::SetServiceOrder(const std::string& order) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.SetServiceOrder(order, &error);
}

bool ProxyDbusClient::SetSchedScan(bool enable) {
  brillo::ErrorPtr error;
  return shill_manager_proxy_.SetSchedScan(enable, &error);
}

bool ProxyDbusClient::GetPropertyValueFromManager(
    const std::string& property_name,
    brillo::Any* property_value) {
  return GetPropertyValueFromProxy(
      &shill_manager_proxy_, property_name, property_value);
}

dbus::ObjectPath ProxyDbusClient::GetObjectPathForActiveProfile() {
  brillo::Any property_value;
  if (!GetPropertyValueFromManager(
        shill::kActiveProfileProperty, &property_value)) {
    return dbus::ObjectPath();
  }
  return dbus::ObjectPath(property_value.Get<std::string>());
}

bool ProxyDbusClient::SetLoggingInternal(int level, const std::string& tags) {
  bool is_success = true;
  brillo::ErrorPtr error;
  is_success &= shill_manager_proxy_.SetDebugLevel(level, &error);
  is_success &= shill_manager_proxy_.SetDebugTags(tags, &error);
  return is_success;
}

template<typename Proxy>
std::unique_ptr<Proxy> ProxyDbusClient::GetProxyForObjectPath(
    const dbus::ObjectPath& object_path) {
  return std::unique_ptr<Proxy>(new Proxy(dbus_bus_, object_path));
}

// Templated functions to return the object path property_name based on
template<typename Proxy>
std::vector<std::unique_ptr<Proxy>> ProxyDbusClient::GetProxies(
    const std::string& object_paths_property_name) {
  brillo::Any object_paths;
  if (!GetPropertyValueFromManager(object_paths_property_name, &object_paths)) {
    return std::vector<std::unique_ptr<Proxy>>();
  }
  std::vector<std::unique_ptr<Proxy>> proxies;
  for (const auto& object_path :
       object_paths.Get<std::vector<dbus::ObjectPath>>()) {
    proxies.emplace_back(GetProxyForObjectPath<Proxy>(object_path));
  }
  return proxies;
}

template<typename Proxy>
std::unique_ptr<Proxy> ProxyDbusClient::GetMatchingProxy(
    const std::string& object_paths_property_name,
    const brillo::VariantDictionary& expected_properties) {
  for (auto& proxy : GetProxies<Proxy>(object_paths_property_name)) {
    brillo::VariantDictionary proxy_properties;
    brillo::ErrorPtr error;
    if (!proxy->GetProperties(&proxy_properties, &error)) {
      // Ignore unknown object path errors since we might be using some proxies
      // for objects which may have been destroyed since.
      CHECK(error->GetCode() == kDbusErrorObjectUnknown);
      continue;
    }
    bool all_expected_properties_matched = true;
    for (const auto& expected_property : expected_properties) {
      if (proxy_properties[expected_property.first] != expected_property.second) {
        all_expected_properties_matched = false;
        break;
      }
    }
    if (all_expected_properties_matched) {
      return std::move(proxy);
    }
  }
  return nullptr;
}

template<typename Proxy>
bool ProxyDbusClient::WaitForProxyPropertyValueIn(
    const dbus::ObjectPath& object_path,
    const std::string& property_name,
    const std::vector<brillo::Any>& expected_values,
    long timeout_milliseconds,
    brillo::Any* final_value,
    long* elapsed_time_milliseconds) {
  // Creates a local proxy using |object_path| instead of accepting the proxy
  // from the caller since we cannot deregister the signal property change
  // callback associated.
  auto proxy = GetProxyForObjectPath<Proxy>(object_path);
  auto condition_termination_checker =
      base::Bind(&IsProxyPropertyValueIn<Proxy>,
                 proxy.get(),
                 property_name,
                 expected_values);
  auto condition_change_callback =
      base::Bind(&PropertyChangedSignalCallback,
                 property_name,
                 expected_values);
  auto condition_change_callback_registrar =
      base::Bind(&HelpRegisterPropertyChangedSignalHandler<Proxy>,
                 base::Unretained(proxy.get()),
                 base::Bind(&PropertyChangedOnConnectedCallback,
                            property_name));
  bool is_success;
  WaitForCondition(
      condition_termination_checker, condition_change_callback,
      condition_change_callback_registrar,
      timeout_milliseconds, &is_success, final_value, elapsed_time_milliseconds);
  return is_success;
}

void ProxyDbusClient::IsMatchingServicePresent(
    const brillo::VariantDictionary& service_properties,
    base::Time wait_start_time,
    bool* is_success,
    std::unique_ptr<ServiceProxy>* service_proxy_out,
    long* elapsed_time_milliseconds) {
  auto service_proxy = GetMatchingServiceProxy(service_properties);
  *is_success = false;
  if (service_proxy) {
    *is_success = true;
  }
  if (service_proxy_out) {
    *service_proxy_out = std::move(service_proxy);
  }
  if (elapsed_time_milliseconds) {
    *elapsed_time_milliseconds =
        (base::Time::Now() - wait_start_time).InMilliseconds();
  }
}

void ProxyDbusClient::FindServiceOrRestartScan(
    const brillo::VariantDictionary& service_properties,
    const std::string& service_type) {
  if (GetMatchingServiceProxy(service_properties)) {
    base::MessageLoop::current()->QuitNow();
  } else {
    RestartScanForService(service_type);
  }
}

void ProxyDbusClient::InitiateScanForService(
    base::TimeDelta rescan_interval,
    const std::string& service_type,
    const base::Closure& timer_callback) {
  // Create a new timer instance for repeatedly calling the provided
  // |timer_callback|. |WaitForCondition| will cancel |timer_callback|'s
  // enclosing CancelableCallback when it exits and hence we need to
  // use the same reference when we repeatedly schedule |timer_callback|.
  wait_for_service_timer_.reset(
      new base::Timer(FROM_HERE, rescan_interval, timer_callback, false));
  RestartScanForService(service_type);
}

void ProxyDbusClient::RestartScanForService(
    const std::string& service_type) {
  RequestServiceScan(service_type);
  wait_for_service_timer_->Reset();
}
