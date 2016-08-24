// Automatic generation of D-Bus interfaces:
//  - org.chromium.PowerManager
#ifndef ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXIES_H
#define ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXIES_H
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

// Abstract interface proxy for org::chromium::PowerManager.
class PowerManagerProxyInterface {
 public:
  virtual ~PowerManagerProxyInterface() = default;

  virtual bool RequestShutdown(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RequestShutdownAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |reason| arg is a power_manager::RequestRestartReason value.
  virtual bool RequestRestart(
      int32_t in_reason,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |reason| arg is a power_manager::RequestRestartReason value.
  virtual void RequestRestartAsync(
      int32_t in_reason,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |external_wakeup_count| arg is optional, and it will call two
  // different methods in the backend. This can't be expressed in the DBus
  // Introspection XML file.
  virtual bool RequestSuspend(
      uint64_t in_external_wakeup_count,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |external_wakeup_count| arg is optional, and it will call two
  // different methods in the backend. This can't be expressed in the DBus
  // Introspection XML file.
  virtual void RequestSuspendAsync(
      uint64_t in_external_wakeup_count,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool DecreaseScreenBrightness(
      bool in_allow_off,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void DecreaseScreenBrightnessAsync(
      bool in_allow_off,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool IncreaseScreenBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void IncreaseScreenBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool GetScreenBrightnessPercent(
      double* out_percent,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void GetScreenBrightnessPercentAsync(
      const base::Callback<void(double /*percent*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |style| arg must be one of the values:
  //   power_manager::kBrightnessTransitionGradual or
  //   power_manager::kBrightnessTransitionInstant.
  virtual bool SetScreenBrightnessPercent(
      double in_percent,
      int32_t in_style,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |style| arg must be one of the values:
  //   power_manager::kBrightnessTransitionGradual or
  //   power_manager::kBrightnessTransitionInstant.
  virtual void SetScreenBrightnessPercentAsync(
      double in_percent,
      int32_t in_style,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool DecreaseKeyboardBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void DecreaseKeyboardBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool IncreaseKeyboardBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void IncreaseKeyboardBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerSupplyProperties protobuf.
  virtual bool GetPowerSupplyProperties(
      std::vector<uint8_t>* out_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerSupplyProperties protobuf.
  virtual void GetPowerSupplyPropertiesAsync(
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool HandleVideoActivity(
      bool in_fullscreen,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void HandleVideoActivityAsync(
      bool in_fullscreen,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |type| arg is a power_manager::UserActivityType.
  virtual bool HandleUserActivity(
      int32_t in_type,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |type| arg is a power_manager::UserActivityType.
  virtual void HandleUserActivityAsync(
      int32_t in_type,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool SetIsProjecting(
      bool in_is_projecting,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void SetIsProjectingAsync(
      bool in_is_projecting,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerManagementPolicy protobuf.
  virtual bool SetPolicy(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerManagementPolicy protobuf.
  virtual void SetPolicyAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual bool SetPowerSource(
      const std::string& in_id,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void SetPowerSourceAsync(
      const std::string& in_id,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |timestamp_internal| arg is represented as the return value of
  // base::TimeTicks::ToInternalValue().
  virtual bool HandlePowerButtonAcknowledgment(
      int64_t in_timestamp_internal,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |timestamp_internal| arg is represented as the return value of
  // base::TimeTicks::ToInternalValue().
  virtual void HandlePowerButtonAcknowledgmentAsync(
      int64_t in_timestamp_internal,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  virtual bool RegisterSuspendDelay(
      const std::vector<uint8_t>& in_serialized_request_proto,
      std::vector<uint8_t>* out_serialized_reply_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  virtual void RegisterSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_request_proto,
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  virtual bool UnregisterSuspendDelay(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  virtual void UnregisterSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  virtual bool HandleSuspendReadiness(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  virtual void HandleSuspendReadinessAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  virtual bool RegisterDarkSuspendDelay(
      const std::vector<uint8_t>& in_serialized_request_proto,
      std::vector<uint8_t>* out_serialized_reply_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  virtual void RegisterDarkSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_request_proto,
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  virtual bool UnregisterDarkSuspendDelay(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  virtual void UnregisterDarkSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  virtual bool HandleDarkSuspendReadiness(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  virtual void HandleDarkSuspendReadinessAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::DarkResumeWakeReason protobuf.
  virtual bool RecordDarkResumeWakeReason(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  // The |serialized_proto| arg is a serialized
  // power_manager::DarkResumeWakeReason protobuf.
  virtual void RecordDarkResumeWakeReasonAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) = 0;

  virtual void RegisterBrightnessChangedSignalHandler(
      const base::Callback<void(int32_t,
                                bool)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterKeyboardBrightnessChangedSignalHandler(
      const base::Callback<void(int32_t,
                                bool)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterPeripheralBatteryStatusSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterPowerSupplyPollSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterSuspendImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterSuspendDoneSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterDarkSuspendImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterInputEventSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterIdleActionImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

  virtual void RegisterIdleActionDeferredSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface proxy for org::chromium::PowerManager.
class PowerManagerProxy final : public PowerManagerProxyInterface {
 public:
  PowerManagerProxy(const scoped_refptr<dbus::Bus>& bus) :
      bus_{bus},
      dbus_object_proxy_{
          bus_->GetObjectProxy(service_name_, object_path_)} {
  }

  ~PowerManagerProxy() override {
    bus_->RemoveObjectProxy(
        service_name_, object_path_, base::Bind(&base::DoNothing));
  }

  void RegisterBrightnessChangedSignalHandler(
      const base::Callback<void(int32_t,
                                bool)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "BrightnessChanged",
        signal_callback,
        on_connected_callback);
  }

  void RegisterKeyboardBrightnessChangedSignalHandler(
      const base::Callback<void(int32_t,
                                bool)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "KeyboardBrightnessChanged",
        signal_callback,
        on_connected_callback);
  }

  void RegisterPeripheralBatteryStatusSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "PeripheralBatteryStatus",
        signal_callback,
        on_connected_callback);
  }

  void RegisterPowerSupplyPollSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "PowerSupplyPoll",
        signal_callback,
        on_connected_callback);
  }

  void RegisterSuspendImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SuspendImminent",
        signal_callback,
        on_connected_callback);
  }

  void RegisterSuspendDoneSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SuspendDone",
        signal_callback,
        on_connected_callback);
  }

  void RegisterDarkSuspendImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "DarkSuspendImminent",
        signal_callback,
        on_connected_callback);
  }

  void RegisterInputEventSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "InputEvent",
        signal_callback,
        on_connected_callback);
  }

  void RegisterIdleActionImminentSignalHandler(
      const base::Callback<void(const std::vector<uint8_t>&)>& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IdleActionImminent",
        signal_callback,
        on_connected_callback);
  }

  void RegisterIdleActionDeferredSignalHandler(
      const base::Closure& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override {
    brillo::dbus_utils::ConnectToSignal(
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IdleActionDeferred",
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

  bool RequestShutdown(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestShutdown",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void RequestShutdownAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestShutdown",
        success_callback,
        error_callback);
  }

  // The |reason| arg is a power_manager::RequestRestartReason value.
  bool RequestRestart(
      int32_t in_reason,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestRestart",
        error,
        in_reason);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |reason| arg is a power_manager::RequestRestartReason value.
  void RequestRestartAsync(
      int32_t in_reason,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestRestart",
        success_callback,
        error_callback,
        in_reason);
  }

  // The |external_wakeup_count| arg is optional, and it will call two
  // different methods in the backend. This can't be expressed in the DBus
  // Introspection XML file.
  bool RequestSuspend(
      uint64_t in_external_wakeup_count,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestSuspend",
        error,
        in_external_wakeup_count);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |external_wakeup_count| arg is optional, and it will call two
  // different methods in the backend. This can't be expressed in the DBus
  // Introspection XML file.
  void RequestSuspendAsync(
      uint64_t in_external_wakeup_count,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RequestSuspend",
        success_callback,
        error_callback,
        in_external_wakeup_count);
  }

  bool DecreaseScreenBrightness(
      bool in_allow_off,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "DecreaseScreenBrightness",
        error,
        in_allow_off);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void DecreaseScreenBrightnessAsync(
      bool in_allow_off,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "DecreaseScreenBrightness",
        success_callback,
        error_callback,
        in_allow_off);
  }

  bool IncreaseScreenBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IncreaseScreenBrightness",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void IncreaseScreenBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IncreaseScreenBrightness",
        success_callback,
        error_callback);
  }

  bool GetScreenBrightnessPercent(
      double* out_percent,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "GetScreenBrightnessPercent",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_percent);
  }

  void GetScreenBrightnessPercentAsync(
      const base::Callback<void(double /*percent*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "GetScreenBrightnessPercent",
        success_callback,
        error_callback);
  }

  // The |style| arg must be one of the values:
  //   power_manager::kBrightnessTransitionGradual or
  //   power_manager::kBrightnessTransitionInstant.
  bool SetScreenBrightnessPercent(
      double in_percent,
      int32_t in_style,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetScreenBrightnessPercent",
        error,
        in_percent,
        in_style);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |style| arg must be one of the values:
  //   power_manager::kBrightnessTransitionGradual or
  //   power_manager::kBrightnessTransitionInstant.
  void SetScreenBrightnessPercentAsync(
      double in_percent,
      int32_t in_style,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetScreenBrightnessPercent",
        success_callback,
        error_callback,
        in_percent,
        in_style);
  }

  bool DecreaseKeyboardBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "DecreaseKeyboardBrightness",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void DecreaseKeyboardBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "DecreaseKeyboardBrightness",
        success_callback,
        error_callback);
  }

  bool IncreaseKeyboardBrightness(
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IncreaseKeyboardBrightness",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void IncreaseKeyboardBrightnessAsync(
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "IncreaseKeyboardBrightness",
        success_callback,
        error_callback);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerSupplyProperties protobuf.
  bool GetPowerSupplyProperties(
      std::vector<uint8_t>* out_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "GetPowerSupplyProperties",
        error);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_serialized_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerSupplyProperties protobuf.
  void GetPowerSupplyPropertiesAsync(
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "GetPowerSupplyProperties",
        success_callback,
        error_callback);
  }

  bool HandleVideoActivity(
      bool in_fullscreen,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleVideoActivity",
        error,
        in_fullscreen);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void HandleVideoActivityAsync(
      bool in_fullscreen,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleVideoActivity",
        success_callback,
        error_callback,
        in_fullscreen);
  }

  // The |type| arg is a power_manager::UserActivityType.
  bool HandleUserActivity(
      int32_t in_type,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleUserActivity",
        error,
        in_type);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |type| arg is a power_manager::UserActivityType.
  void HandleUserActivityAsync(
      int32_t in_type,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleUserActivity",
        success_callback,
        error_callback,
        in_type);
  }

  bool SetIsProjecting(
      bool in_is_projecting,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetIsProjecting",
        error,
        in_is_projecting);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void SetIsProjectingAsync(
      bool in_is_projecting,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetIsProjecting",
        success_callback,
        error_callback,
        in_is_projecting);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerManagementPolicy protobuf.
  bool SetPolicy(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetPolicy",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::PowerManagementPolicy protobuf.
  void SetPolicyAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetPolicy",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

  bool SetPowerSource(
      const std::string& in_id,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetPowerSource",
        error,
        in_id);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  void SetPowerSourceAsync(
      const std::string& in_id,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "SetPowerSource",
        success_callback,
        error_callback,
        in_id);
  }

  // The |timestamp_internal| arg is represented as the return value of
  // base::TimeTicks::ToInternalValue().
  bool HandlePowerButtonAcknowledgment(
      int64_t in_timestamp_internal,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandlePowerButtonAcknowledgment",
        error,
        in_timestamp_internal);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |timestamp_internal| arg is represented as the return value of
  // base::TimeTicks::ToInternalValue().
  void HandlePowerButtonAcknowledgmentAsync(
      int64_t in_timestamp_internal,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandlePowerButtonAcknowledgment",
        success_callback,
        error_callback,
        in_timestamp_internal);
  }

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  bool RegisterSuspendDelay(
      const std::vector<uint8_t>& in_serialized_request_proto,
      std::vector<uint8_t>* out_serialized_reply_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RegisterSuspendDelay",
        error,
        in_serialized_request_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_serialized_reply_proto);
  }

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  void RegisterSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_request_proto,
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RegisterSuspendDelay",
        success_callback,
        error_callback,
        in_serialized_request_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  bool UnregisterSuspendDelay(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "UnregisterSuspendDelay",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  void UnregisterSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "UnregisterSuspendDelay",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  bool HandleSuspendReadiness(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleSuspendReadiness",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  void HandleSuspendReadinessAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleSuspendReadiness",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  bool RegisterDarkSuspendDelay(
      const std::vector<uint8_t>& in_serialized_request_proto,
      std::vector<uint8_t>* out_serialized_reply_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RegisterDarkSuspendDelay",
        error,
        in_serialized_request_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error, out_serialized_reply_proto);
  }

  // The |serialized_request_proto| arg is a serialized
  // power_manager::RegisterSuspendDelayRequest protobuf.
  // The |serialized_reply_proto| arg is a serialized
  // RegisterSuspendDelayReply protobuf.
  void RegisterDarkSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_request_proto,
      const base::Callback<void(const std::vector<uint8_t>& /*serialized_reply_proto*/)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RegisterDarkSuspendDelay",
        success_callback,
        error_callback,
        in_serialized_request_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  bool UnregisterDarkSuspendDelay(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "UnregisterDarkSuspendDelay",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::UnregisterSuspendDelayRequest protobuf.
  void UnregisterDarkSuspendDelayAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "UnregisterDarkSuspendDelay",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  bool HandleDarkSuspendReadiness(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleDarkSuspendReadiness",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::SuspendReadinessInfo protobuf.
  void HandleDarkSuspendReadinessAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "HandleDarkSuspendReadiness",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::DarkResumeWakeReason protobuf.
  bool RecordDarkResumeWakeReason(
      const std::vector<uint8_t>& in_serialized_proto,
      brillo::ErrorPtr* error,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RecordDarkResumeWakeReason",
        error,
        in_serialized_proto);
    return response && brillo::dbus_utils::ExtractMethodCallResults(
        response.get(), error);
  }

  // The |serialized_proto| arg is a serialized
  // power_manager::DarkResumeWakeReason protobuf.
  void RecordDarkResumeWakeReasonAsync(
      const std::vector<uint8_t>& in_serialized_proto,
      const base::Callback<void()>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback,
      int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT) override {
    brillo::dbus_utils::CallMethodWithTimeout(
        timeout_ms,
        dbus_object_proxy_,
        "org.chromium.PowerManager",
        "RecordDarkResumeWakeReason",
        success_callback,
        error_callback,
        in_serialized_proto);
  }

 private:
  scoped_refptr<dbus::Bus> bus_;
  const std::string service_name_{"org.chromium.PowerManager"};
  const dbus::ObjectPath object_path_{"/org/chromium/PowerManager"};
  dbus::ObjectProxy* dbus_object_proxy_;

  DISALLOW_COPY_AND_ASSIGN(PowerManagerProxy);
};

}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING____________________BUILD_LINK_TMP_PORTAGE_CHROMEOS_BASE_POWER_MANAGER_9999_WORK_BUILD_OUT_DEFAULT_GEN_INCLUDE_POWER_MANAGER_DBUS_PROXIES_H
