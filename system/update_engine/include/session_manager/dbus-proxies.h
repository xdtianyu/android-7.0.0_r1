// Automatic generation of D-Bus interfaces:
//  - org.chromium.SessionManagerInterface
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXIES_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXIES_H
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

// Abstract interface proxy for org::chromium::SessionManagerInterface.
class SessionManagerInterfaceProxyInterface {
 public:
  virtual ~SessionManagerInterfaceProxyInterface() = default;

  virtual bool EmitLoginPromptVisible(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void EmitLoginPromptVisibleAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool EnableChromeTesting(
      bool in_force_relaunch,
      const std::vector<std::string>& in_extra_arguments,
      std::string* out_filepath,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void EnableChromeTestingAsync(
      bool in_force_relaunch,
      const std::vector<std::string>& in_extra_arguments,
      const base::Callback<void(const std::string& /*filepath*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StartSession(
      const std::string& in_email_address,
      const std::string& in_unique_identifier,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StartSessionAsync(
      const std::string& in_email_address,
      const std::string& in_unique_identifier,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StopSession(
      const std::string& in_unique_identifier,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StopSessionAsync(
      const std::string& in_unique_identifier,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StorePolicy(
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StorePolicyAsync(
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RetrievePolicy(
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RetrievePolicyAsync(
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StorePolicyForUser(
      const std::string& in_user_email,
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StorePolicyForUserAsync(
      const std::string& in_user_email,
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RetrievePolicyForUser(
      const std::string& in_user_email,
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RetrievePolicyForUserAsync(
      const std::string& in_user_email,
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StoreDeviceLocalAccountPolicy(
      const std::string& in_account_id,
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StoreDeviceLocalAccountPolicyAsync(
      const std::string& in_account_id,
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RetrieveDeviceLocalAccountPolicy(
      const std::string& in_account_id,
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RetrieveDeviceLocalAccountPolicyAsync(
      const std::string& in_account_id,
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RetrieveSessionState(
      std::string* out_state,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RetrieveSessionStateAsync(
      const base::Callback<void(const std::string& /*state*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RetrieveActiveSessions(
      std::map<std::string, std::string>* out_sessions,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RetrieveActiveSessionsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*sessions*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool HandleSupervisedUserCreationStarting(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void HandleSupervisedUserCreationStartingAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool HandleSupervisedUserCreationFinished(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void HandleSupervisedUserCreationFinishedAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool LockScreen(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void LockScreenAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool HandleLockScreenShown(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void HandleLockScreenShownAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool HandleLockScreenDismissed(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void HandleLockScreenDismissedAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool RestartJob(
      const dbus::FileDescriptor& in_cred_fd,
      const std::vector<std::string>& in_argv,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RestartJobAsync(
      const dbus::FileDescriptor& in_cred_fd,
      const std::vector<std::string>& in_argv,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool StartDeviceWipe(
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void StartDeviceWipeAsync(
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool SetFlagsForUser(
      const std::string& in_user_email,
      const std::vector<std::string>& in_flags,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void SetFlagsForUserAsync(
      const std::string& in_user_email,
      const std::vector<std::string>& in_flags,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool GetServerBackedStateKeys(
      std::vector<std::vector<uint8_t>>* out_state_keys,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void GetServerBackedStateKeysAsync(
      const base::Callback<void(const std::vector<std::vector<uint8_t>>& /*state_keys*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool InitMachineInfo(
      const std::string& in_data,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void InitMachineInfoAsync(
      const std::string& in_data,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RegisterLoginPromptVisibleSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterSessionStateChangedSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterSetOwnerKeyCompleteSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterPropertyChangeCompleteSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterScreenIsLockedSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterScreenIsUnlockedSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::SessionManagerInterface.
class SessionManagerInterfaceProxy final : public SessionManagerInterfaceProxyInterface {
 public:
  SessionManagerInterfaceProxy(const scoped_refptr<dbus::Bus>& bus) :
      bus_{bus},
      dbus_object_proxy_{
          bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~SessionManagerInterfaceProxy() override {
    bus_->RemoveObjectProxy(
        service_name_, object_path_, base::Bind(&base::DoNothing));
  }

  void RegisterLoginPromptVisibleSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "LoginPromptVisible",
        signal_callback,
        on_connected_callback);
  }

  void RegisterSessionStateChangedSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "SessionStateChanged",
        signal_callback,
        on_connected_callback);
  }

  void RegisterSetOwnerKeyCompleteSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "SetOwnerKeyComplete",
        signal_callback,
        on_connected_callback);
  }

  void RegisterPropertyChangeCompleteSignalHandler(
      const base::Callback<void(const std::string&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "PropertyChangeComplete",
        signal_callback,
        on_connected_callback);
  }

  void RegisterScreenIsLockedSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "ScreenIsLocked",
        signal_callback,
        on_connected_callback);
  }

  void RegisterScreenIsUnlockedSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "ScreenIsUnlocked",
        signal_callback,
        on_connected_callback);
  }

  void ReleaseObjectProxy(const base::Closure& callback) {
    bus_->RemoveObjectProxy(service_name_, object_path_, callback);
  }

  const dbus::ObjectPath& GetObjectPath() const {
    return object_path_;
  }

  dbus::ObjectProxy* GetObjectProxy() const { return dbus_object_proxy_; }

  bool EmitLoginPromptVisible(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "EmitLoginPromptVisible",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void EmitLoginPromptVisibleAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "EmitLoginPromptVisible",
        success_callback,
        error_callback);
  }

  bool EnableChromeTesting(
      bool in_force_relaunch,
      const std::vector<std::string>& in_extra_arguments,
      std::string* out_filepath,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "EnableChromeTesting",
        error,
        in_force_relaunch,
        in_extra_arguments);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_filepath);
  }

  void EnableChromeTestingAsync(
      bool in_force_relaunch,
      const std::vector<std::string>& in_extra_arguments,
      const base::Callback<void(const std::string& /*filepath*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "EnableChromeTesting",
        success_callback,
        error_callback,
        in_force_relaunch,
        in_extra_arguments);
  }

  bool StartSession(
      const std::string& in_email_address,
      const std::string& in_unique_identifier,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StartSession",
        error,
        in_email_address,
        in_unique_identifier);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StartSessionAsync(
      const std::string& in_email_address,
      const std::string& in_unique_identifier,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StartSession",
        success_callback,
        error_callback,
        in_email_address,
        in_unique_identifier);
  }

  bool StopSession(
      const std::string& in_unique_identifier,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StopSession",
        error,
        in_unique_identifier);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StopSessionAsync(
      const std::string& in_unique_identifier,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StopSession",
        success_callback,
        error_callback,
        in_unique_identifier);
  }

  bool StorePolicy(
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StorePolicy",
        error,
        in_policy_blob);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StorePolicyAsync(
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StorePolicy",
        success_callback,
        error_callback,
        in_policy_blob);
  }

  bool RetrievePolicy(
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrievePolicy",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_policy_blob);
  }

  void RetrievePolicyAsync(
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrievePolicy",
        success_callback,
        error_callback);
  }

  bool StorePolicyForUser(
      const std::string& in_user_email,
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StorePolicyForUser",
        error,
        in_user_email,
        in_policy_blob);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StorePolicyForUserAsync(
      const std::string& in_user_email,
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StorePolicyForUser",
        success_callback,
        error_callback,
        in_user_email,
        in_policy_blob);
  }

  bool RetrievePolicyForUser(
      const std::string& in_user_email,
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrievePolicyForUser",
        error,
        in_user_email);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_policy_blob);
  }

  void RetrievePolicyForUserAsync(
      const std::string& in_user_email,
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrievePolicyForUser",
        success_callback,
        error_callback,
        in_user_email);
  }

  bool StoreDeviceLocalAccountPolicy(
      const std::string& in_account_id,
      const std::vector<uint8_t>& in_policy_blob,
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StoreDeviceLocalAccountPolicy",
        error,
        in_account_id,
        in_policy_blob);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StoreDeviceLocalAccountPolicyAsync(
      const std::string& in_account_id,
      const std::vector<uint8_t>& in_policy_blob,
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StoreDeviceLocalAccountPolicy",
        success_callback,
        error_callback,
        in_account_id,
        in_policy_blob);
  }

  bool RetrieveDeviceLocalAccountPolicy(
      const std::string& in_account_id,
      std::vector<uint8_t>* out_policy_blob,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveDeviceLocalAccountPolicy",
        error,
        in_account_id);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_policy_blob);
  }

  void RetrieveDeviceLocalAccountPolicyAsync(
      const std::string& in_account_id,
      const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveDeviceLocalAccountPolicy",
        success_callback,
        error_callback,
        in_account_id);
  }

  bool RetrieveSessionState(
      std::string* out_state,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveSessionState",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_state);
  }

  void RetrieveSessionStateAsync(
      const base::Callback<void(const std::string& /*state*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveSessionState",
        success_callback,
        error_callback);
  }

  bool RetrieveActiveSessions(
      std::map<std::string, std::string>* out_sessions,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveActiveSessions",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_sessions);
  }

  void RetrieveActiveSessionsAsync(
      const base::Callback<void(const std::map<std::string, std::string>& /*sessions*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RetrieveActiveSessions",
        success_callback,
        error_callback);
  }

  bool HandleSupervisedUserCreationStarting(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleSupervisedUserCreationStarting",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void HandleSupervisedUserCreationStartingAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleSupervisedUserCreationStarting",
        success_callback,
        error_callback);
  }

  bool HandleSupervisedUserCreationFinished(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleSupervisedUserCreationFinished",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void HandleSupervisedUserCreationFinishedAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleSupervisedUserCreationFinished",
        success_callback,
        error_callback);
  }

  bool LockScreen(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "LockScreen",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void LockScreenAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "LockScreen",
        success_callback,
        error_callback);
  }

  bool HandleLockScreenShown(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleLockScreenShown",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void HandleLockScreenShownAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleLockScreenShown",
        success_callback,
        error_callback);
  }

  bool HandleLockScreenDismissed(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleLockScreenDismissed",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void HandleLockScreenDismissedAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "HandleLockScreenDismissed",
        success_callback,
        error_callback);
  }

  bool RestartJob(
      const dbus::FileDescriptor& in_cred_fd,
      const std::vector<std::string>& in_argv,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RestartJob",
        error,
        in_cred_fd,
        in_argv);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void RestartJobAsync(
      const dbus::FileDescriptor& in_cred_fd,
      const std::vector<std::string>& in_argv,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "RestartJob",
        success_callback,
        error_callback,
        in_cred_fd,
        in_argv);
  }

  bool StartDeviceWipe(
      bool* out_done,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StartDeviceWipe",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_done);
  }

  void StartDeviceWipeAsync(
      const base::Callback<void(bool /*done*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "StartDeviceWipe",
        success_callback,
        error_callback);
  }

  bool SetFlagsForUser(
      const std::string& in_user_email,
      const std::vector<std::string>& in_flags,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "SetFlagsForUser",
        error,
        in_user_email,
        in_flags);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void SetFlagsForUserAsync(
      const std::string& in_user_email,
      const std::vector<std::string>& in_flags,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "SetFlagsForUser",
        success_callback,
        error_callback,
        in_user_email,
        in_flags);
  }

  bool GetServerBackedStateKeys(
      std::vector<std::vector<uint8_t>>* out_state_keys,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "GetServerBackedStateKeys",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_state_keys);
  }

  void GetServerBackedStateKeysAsync(
      const base::Callback<void(const std::vector<std::vector<uint8_t>>& /*state_keys*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "GetServerBackedStateKeys",
        success_callback,
        error_callback);
  }

  bool InitMachineInfo(
      const std::string& in_data,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "InitMachineInfo",
        error,
        in_data);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void InitMachineInfoAsync(
      const std::string& in_data,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.SessionManagerInterface",
        "InitMachineInfo",
        success_callback,
        error_callback,
        in_data);
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.SessionManager"};
  const dbus::ObjectPath object_path_{"/org/chromium/SessionManager"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(SessionManagerInterfaceProxy);
};

}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXIES_H
