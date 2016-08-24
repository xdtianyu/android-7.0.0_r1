// Automatic generation of D-Bus interface mock proxies for:
//  - org.chromium.PowerManager
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXY_MOCKS_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXY_MOCKS_H
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "power_manager/dbus-proxies.h"

namespace org {
namespace chromium {

// Mock object for PowerManagerProxyInterface.
class PowerManagerProxyMock : public PowerManagerProxyInterface {
 public:
  PowerManagerProxyMock() = default;

  MOCK_METHOD2(RequestShutdown,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RequestShutdownAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RequestRestart,
               bool(int32_t /*in_reason*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RequestRestartAsync,
               void(int32_t /*in_reason*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RequestSuspend,
               bool(uint64_t /*in_external_wakeup_count*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RequestSuspendAsync,
               void(uint64_t /*in_external_wakeup_count*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(DecreaseScreenBrightness,
               bool(bool /*in_allow_off*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(DecreaseScreenBrightnessAsync,
               void(bool /*in_allow_off*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(IncreaseScreenBrightness,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(IncreaseScreenBrightnessAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetScreenBrightnessPercent,
               bool(double* /*out_percent*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetScreenBrightnessPercentAsync,
               void(const base::Callback<void(double /*percent*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetScreenBrightnessPercent,
               bool(double /*in_percent*/,
                    int32_t /*in_style*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(SetScreenBrightnessPercentAsync,
               void(double /*in_percent*/,
                    int32_t /*in_style*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(DecreaseKeyboardBrightness,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(DecreaseKeyboardBrightnessAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(IncreaseKeyboardBrightness,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(IncreaseKeyboardBrightnessAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetPowerSupplyProperties,
               bool(std::vector<uint8_t>* /*out_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetPowerSupplyPropertiesAsync,
               void(const base::Callback<void(const std::vector<uint8_t>& /*serialized_proto*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleVideoActivity,
               bool(bool /*in_fullscreen*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(HandleVideoActivityAsync,
               void(bool /*in_fullscreen*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleUserActivity,
               bool(int32_t /*in_type*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(HandleUserActivityAsync,
               void(int32_t /*in_type*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetIsProjecting,
               bool(bool /*in_is_projecting*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetIsProjectingAsync,
               void(bool /*in_is_projecting*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetPolicy,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetPolicyAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetPowerSource,
               bool(const std::string& /*in_id*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetPowerSourceAsync,
               void(const std::string& /*in_id*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandlePowerButtonAcknowledgment,
               bool(int64_t /*in_timestamp_internal*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(HandlePowerButtonAcknowledgmentAsync,
               void(int64_t /*in_timestamp_internal*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RegisterSuspendDelay,
               bool(const std::vector<uint8_t>& /*in_serialized_request_proto*/,
                    std::vector<uint8_t>* /*out_serialized_reply_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RegisterSuspendDelayAsync,
               void(const std::vector<uint8_t>& /*in_serialized_request_proto*/,
                    const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(UnregisterSuspendDelay,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(UnregisterSuspendDelayAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleSuspendReadiness,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(HandleSuspendReadinessAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RegisterDarkSuspendDelay,
               bool(const std::vector<uint8_t>& /*in_serialized_request_proto*/,
                    std::vector<uint8_t>* /*out_serialized_reply_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RegisterDarkSuspendDelayAsync,
               void(const std::vector<uint8_t>& /*in_serialized_request_proto*/,
                    const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(UnregisterDarkSuspendDelay,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(UnregisterDarkSuspendDelayAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(HandleDarkSuspendReadiness,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(HandleDarkSuspendReadinessAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RecordDarkResumeWakeReason,
               bool(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RecordDarkResumeWakeReasonAsync,
               void(const std::vector<uint8_t>& /*in_serialized_proto*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RegisterBrightnessChangedSignalHandler,
               void(const base::Callback<void(int32_t,
                                              bool)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterKeyboardBrightnessChangedSignalHandler,
               void(const base::Callback<void(int32_t,
                                              bool)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterPeripheralBatteryStatusSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterPowerSupplyPollSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterSuspendImminentSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterSuspendDoneSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterDarkSuspendImminentSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterInputEventSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterIdleActionImminentSignalHandler,
               void(const base::Callback<void(const std::vector<uint8_t>&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterIdleActionDeferredSignalHandler,
               void(const base::Closure& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerManagerProxyMock);
};
}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXY_MOCKS_H
