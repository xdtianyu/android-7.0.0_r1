// Automatic generation of D-Bus interface mock proxies for:
//  - org.chromium.SessionManagerInterface
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXY_MOCKS_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXY_MOCKS_H
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "session_manager/dbus-proxies.h"

namespace org {
namespace chromium {

// Mock object for SessionManagerInterfaceProxyInterface.
class SessionManagerInterfaceProxyMock : public SessionManagerInterfaceProxyInterface {
 public:
  SessionManagerInterfaceProxyMock() = default;

  MOCK_METHOD2(EmitLoginPromptVisible,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EmitLoginPromptVisibleAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(EnableChromeTesting,
               bool(bool /*in_force_relaunch*/,
                    const std::vector<std::string>& /*in_extra_arguments*/,
                    std::string* /*out_filepath*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(EnableChromeTestingAsync,
               void(bool /*in_force_relaunch*/,
                    const std::vector<std::string>& /*in_extra_arguments*/,
                    const base::Callback<void(const std::string& /*filepath*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StartSession,
               bool(const std::string& /*in_email_address*/,
                    const std::string& /*in_unique_identifier*/,
                    bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StartSessionAsync,
               void(const std::string& /*in_email_address*/,
                    const std::string& /*in_unique_identifier*/,
                    const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(StopSession,
               bool(const std::string& /*in_unique_identifier*/,
                    bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(StopSessionAsync,
               void(const std::string& /*in_unique_identifier*/,
                    const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(StorePolicy,
               bool(const std::vector<uint8_t>& /*in_policy_blob*/,
                    bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(StorePolicyAsync,
               void(const std::vector<uint8_t>& /*in_policy_blob*/,
                    const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrievePolicy,
               bool(std::vector<uint8_t>* /*out_policy_blob*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrievePolicyAsync,
               void(const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StorePolicyForUser,
               bool(const std::string& /*in_user_email*/,
                    const std::vector<uint8_t>& /*in_policy_blob*/,
                    bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StorePolicyForUserAsync,
               void(const std::string& /*in_user_email*/,
                    const std::vector<uint8_t>& /*in_policy_blob*/,
                    const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RetrievePolicyForUser,
               bool(const std::string& /*in_user_email*/,
                    std::vector<uint8_t>* /*out_policy_blob*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RetrievePolicyForUserAsync,
               void(const std::string& /*in_user_email*/,
                    const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StoreDeviceLocalAccountPolicy,
               bool(const std::string& /*in_account_id*/,
                    const std::vector<uint8_t>& /*in_policy_blob*/,
                    bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(StoreDeviceLocalAccountPolicyAsync,
               void(const std::string& /*in_account_id*/,
                    const std::vector<uint8_t>& /*in_policy_blob*/,
                    const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RetrieveDeviceLocalAccountPolicy,
               bool(const std::string& /*in_account_id*/,
                    std::vector<uint8_t>* /*out_policy_blob*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RetrieveDeviceLocalAccountPolicyAsync,
               void(const std::string& /*in_account_id*/,
                    const base::Callback<void(const std::vector<uint8_t>& /*policy_blob*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrieveSessionState,
               bool(std::string* /*out_state*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrieveSessionStateAsync,
               void(const base::Callback<void(const std::string& /*state*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrieveActiveSessions,
               bool(std::map<std::string, std::string>* /*out_sessions*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RetrieveActiveSessionsAsync,
               void(const base::Callback<void(const std::map<std::string, std::string>& /*sessions*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(HandleSupervisedUserCreationStarting,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleSupervisedUserCreationStartingAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(HandleSupervisedUserCreationFinished,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleSupervisedUserCreationFinishedAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(LockScreen,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(LockScreenAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(HandleLockScreenShown,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleLockScreenShownAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(HandleLockScreenDismissed,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleLockScreenDismissedAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RestartJob,
               bool(const dbus::FileDescriptor& /*in_cred_fd*/,
                    const std::vector<std::string>& /*in_argv*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(RestartJobAsync,
               void(const dbus::FileDescriptor& /*in_cred_fd*/,
                    const std::vector<std::string>& /*in_argv*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(StartDeviceWipe,
               bool(bool* /*out_done*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(StartDeviceWipeAsync,
               void(const base::Callback<void(bool /*done*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetFlagsForUser,
               bool(const std::string& /*in_user_email*/,
                    const std::vector<std::string>& /*in_flags*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(SetFlagsForUserAsync,
               void(const std::string& /*in_user_email*/,
                    const std::vector<std::string>& /*in_flags*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetServerBackedStateKeys,
               bool(std::vector<std::vector<uint8_t>>* /*out_state_keys*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetServerBackedStateKeysAsync,
               void(const base::Callback<void(const std::vector<std::vector<uint8_t>>& /*state_keys*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(InitMachineInfo,
               bool(const std::string& /*in_data*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(InitMachineInfoAsync,
               void(const std::string& /*in_data*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RegisterLoginPromptVisibleSignalHandler,
               void(const base::Closure& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterSessionStateChangedSignalHandler,
               void(const base::Callback<void(const std::string&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterSetOwnerKeyCompleteSignalHandler,
               void(const base::Callback<void(const std::string&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterPropertyChangeCompleteSignalHandler,
               void(const base::Callback<void(const std::string&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterScreenIsLockedSignalHandler,
               void(const base::Closure& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterScreenIsUnlockedSignalHandler,
               void(const base::Closure& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionManagerInterfaceProxyMock);
};
}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_VAR_CACHE_PORTAGE_CHROMEOS_BASE_CHROMEOS_LOGIN_OUT_DEFAULT_GEN_INCLUDE_SESSION_MANAGER_DBUS_PROXY_MOCKS_H
